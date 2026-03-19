/******************************************************************************
 * @file    bldc_comp.c
 * @brief   比较器过零检测与30°延时换相（50us采样，软件滤波）
 * @author  cyWu
 * @date    2025-11-25
 * @version V2.0.0
 * @history
 *  - V2.0.0, 2025-11-25, cyWu, 过零检测+30°补偿换相
 *
 * @重点功能
 *  - 50us 周期采样（可通过 BLDC_SAMPLE_PERIOD_US 调整）
 *  - 过零后按 30° 电角度延时触发换相
 *
 * @使用步骤（摘要）
 *  1) BLDC_COMP_Init()：初始化比较器与采样计数
 *  2) 50us定时中断调用 BLDC_COMP_SampleAndFilter()
 *  3) 换相前调用 BLDC_COMP_SelectPhase() 设置当前浮空检测相
 *  4) 主循环轮询 BLDC_COMP_IsCommutationReady()，就绪后执行 Motor_Commutate()
 *  5) 可选：BLDC_COMP_GetSpeed() 获取当前转速（RPM）
 ******************************************************************************/
#include "bldc_comp.h"
#include "bldc_init.h"
#include "bldc_motor.h"
uint32_t s_delay_30_degree_time = 0;

/* ==================== 说明 ==================== */
/**
 * @brief   采样周期和极对数配置说明
 *
 * @note    采样周期（BLDC_SAMPLE_PERIOD_US）和极对数（BLDC_POLE_PAIRS）
 *          已在 bldc_init.h 中统一配置，更换电机时只需修改：
 *          - BLDC_POLE_PAIRS：电机极对数（例如10极电机=5极对）
 *          - BLDC_SAMPLE_PERIOD_US：采样周期（建议50us）
 *
 *          转速转换系数会在 bldc_app_init() 中自动计算：
 *          speed_factor = 60,000,000 / (BLDC_SAMPLE_PERIOD_US × 6 × BLDC_POLE_PAIRS)
 */

/* ==================== 30度延时滤波配置 ==================== */
/**
 * @brief   30度延时时间滤波系数（一阶低通滤波）
 *
 * @note    滤波公式：filtered = α * new + (1-α) * old
 *          α = DELAY_FILTER_ALPHA / 256
 *
 *          滤波系数选择：
 *          - 64  (0.25) = 强滤波，响应慢，适合转速稳定场景
 *          - 128 (0.50) = 平衡滤波，推荐默认值 ✅
 *          - 192 (0.75) = 弱滤波，响应快，适合转速快速变化场景
 *          - 256 (1.00) = 无滤波，直接使用原始值
 *
 *          作用：平滑30度延时时间，避免转速波动或噪声导致换相抖动
 */
#define DELAY_FILTER_ALPHA 128 /* 滤波系数：128/256=0.5（推荐值）*/

/* 当前检测相（浮空相） */
static BLDC_Phase_t s_currentPhase;

BLDC_ZeroCross_t s_zeroCross = {0};

/* 全局采样计数器（用于时间计算） */
static uint32_t s_sampleCount = 0; /* 采样次数，每次定时器中断递增1次（当前50us） */

/* 当前档位范围（用于动态调整比较器参数）0=低速，1=中速，2=高速 */
static uint8_t s_currentSpeedRange = 0; /* 默认低速 */

/**
 * @brief   初始化过零检测变量（硬件已在 bldc_init.c 配好）
 * @param   无
 * @return  无
 * @note    仅重置状态和计数器，硬件初始化请调用 BLDC_COMP_CoreInit()
 */
void BLDC_COMP_Init(void)
{
    s_currentPhase = BLDC_PHASE_U;                     /* 默认检测U相 */
    s_zeroCross.last_level = BLDC_COMP_ReadOutput();   /* 当前比较器电平作为初值 */
    s_zeroCross.delay_30_degree_time = 0;              /* 30°延时清零 */
    s_zeroCross.zero_detected = 0;                     /* 过零标志清零 */
    s_zeroCross.commutation_ready = 0;                 /* 换相准备标志清零 */
    s_zeroCross.stable_comm_count = 0;                 /* 稳定换相计数清零 */
    s_zeroCross.last_commutation_time = 0;             /* 上次换相时刻清零 */
    s_zeroCross.filter_level = s_zeroCross.last_level; /* 候选电平初始化为当前电平 */
    s_zeroCross.filter_count = 0;                      /* 连续计数清零 */
    s_sampleCount = 0;                                 /* 采样计数清零 */
}

/**
 * @brief   切换比较器检测相（高性能版本，直接寄存器操作）
 * @param   phase 要检测的相（浮空相）
 * @return  无
 * @note    每次换相后，需要切换比较器检测当前的浮空相
 */
void BLDC_COMP_SelectPhase(BLDC_Phase_t phase)
{
    if (phase == s_currentPhase)
    {
        return; /* 相同则不需要切换 */
    }

    /* 调用硬件层接口完成输入切换 */
    BLDC_COMP_SetInputPlus(phase);
    s_currentPhase = phase;

    /* 复位过零点检测状态 */
    s_zeroCross.zero_detected = 0;
    s_zeroCross.commutation_ready = 0;
}

/**
 * @brief   50us定时器回调：过零点采样+30°换相定时
 * @note    放在定时器中断调用（当前50us周期），流程按当前代码：
 *          1) 每次中断采样一次比较器输出（无多次一致滤波，直接用当前值）；
 *          2) 检测到电平翻转（上升/下降）时：
 *             - 第一次仅记录时间，不触发换相（建立基准）；
 *             - 之后计算“过零→上次换相”的间隔，得到30°延时并一阶滤波；
 *             - 记录过零时刻，置位 zero_detected，等待30°计时后换相；
 *          3) 六步换相检测边沿：步骤1/3/5看上升沿，步骤2/4/6看下降沿。
 */
void BLDC_COMP_SampleAndFilter(void)
{
    s_sampleCount++; /* 全局采样计数器递增 */

    /* ========== 动态滤波阈值：转速越高阈值越小 ========== */
    /* 阈值 = 换相周期 / 8，范围限制在 [2, 15]
     * 转速低/开环时阈值大（保守，抗干扰强）；转速高时阈值小（响应快，不漏过零点） */
    uint8_t filter_threshold;
    {
        uint32_t comm_period = BLDC_Motor_GetLastCommPeriod();
        if (comm_period == 0)
        {
            filter_threshold = 20; /* 无换相数据（开环期间）使用最大值 */
        }
        else
        {
            filter_threshold = (uint8_t)(comm_period >> 2)+(comm_period >> 4); /* 除以8 */
            if (filter_threshold < 4)  { filter_threshold = 4;  } /* 最小2，防止完全不滤波 */
            if (filter_threshold > 15) { filter_threshold = 15; } /* 最大15 */
        }
    }

    /* ========== 连续一致滤波：防止毛刺误触发 ========== */
    /* 每次采样读取一次比较器，连续 filter_threshold 次读到相同值且与上次确认电平不同，才判定为有效边沿 */
    uint8_t current_level = BLDC_COMP_ReadOutput();

    if (current_level == s_zeroCross.filter_level)
    {
        /* 与候选电平一致，累加计数 */
        if (s_zeroCross.filter_count < filter_threshold)
        {
            s_zeroCross.filter_count++;
        }
    }
    else
    {
        /* 候选电平变化，重新开始计数 */
        s_zeroCross.filter_level = current_level;
        s_zeroCross.filter_count = 1;
    }

    /* 连续计数达到阈值且与上次确认电平不同 → 判定为有效边沿 */
    if (s_zeroCross.filter_count < filter_threshold || s_zeroCross.filter_level == s_zeroCross.last_level)
    {
        return; /* 未达到滤波阈值或电平未变化，继续等待 */
    }

    /* 确认边沿，更新已确认电平 */
    s_zeroCross.last_level = s_zeroCross.filter_level;

    /* ========== 方法1：使用"过零点时刻 - 换相时刻"计算30°（推荐）========== */
    /* 原理：从换相点到过零点的时间间隔 = 30°电角度 */
    if (s_zeroCross.last_commutation_time > 0)
    {
        /* 计算30°延时时间 = 当前过零点时刻 - 上次换相时刻 */
        /* 根据档位动态选择右移位数：低档右移5位（除以32），中高档右移2位（除以4） */
        uint32_t delay_30_degree_raw;
        if (s_currentSpeedRange == 0) /* 低速区间 */
        {
            delay_30_degree_raw = ((s_sampleCount - s_zeroCross.last_commutation_time)>>3); /* 低档：右移5位 */
        }
        else /* 中速或高速区间 */
        {
            delay_30_degree_raw = ((s_sampleCount - s_zeroCross.last_commutation_time)>>3); /* 中高档：右移2位 */
        }

        /* 限制原始值最小值，防止极端值进入滤波器 */
        // if (delay_30_degree_raw > 100)
        // {
        //     delay_30_degree_raw = 100; /* 最小值=100×50us=2.5ms */
        // }
        if (delay_30_degree_raw < 1)
        {
            delay_30_degree_raw = 1; /* 最小值=3×50us=0.15ms */
        }

        /* ========== 一阶低通滤波（平滑延时时间，提高稳定性）========== */
        /* 滤波公式：filtered = α * new + (1-α) * old */
        /* 使用定点数运算：α = DELAY_FILTER_ALPHA / 256 */

        if (s_zeroCross.delay_30_degree_time == 0)
        {
            /* 首次计算，直接使用原始值（避免除0或无效滤波）*/
            s_zeroCross.delay_30_degree_time = 500;
        }
        else
        {
            /* 一阶滤波：平滑过渡，抑制转速波动和噪声干扰 */
            s_zeroCross.delay_30_degree_time =
                (s_zeroCross.delay_30_degree_time * (256 - DELAY_FILTER_ALPHA) +
                 delay_30_degree_raw * DELAY_FILTER_ALPHA) >>
                8;
            s_delay_30_degree_time = s_zeroCross.delay_30_degree_time;
        }

        /* ========== 触发换相标志 ========== */
        /* 只有计算出30°延时后，才触发换相流程 */
        s_zeroCross.zero_detect_time = s_sampleCount; /* 记录过零点时刻（用于30°延时计算） */
        s_zeroCross.zero_detected = 1;                /* 设置过零点检测标志 */
        s_zeroCross.commutation_ready = 0;            /* 等待30°延时 */
    }
}

/**
 * @brief   比较器中断回调函数（本模块不使用，仅保留接口）
 * @param   hcompHandle 比较器句柄
 * @return  无
 * @note    定时器采样方案不使用比较器中断，此函数为空实现
 */
void HAL_COMP_TriggerCallback(COMP_HandleTypeDef *hcompHandle)
{
    /* 定时器采样方案不使用此中断 */
}

/**
 * @brief   检查是否检测到过零点（开环专用，不等30度延时）
 * @param   无
 * @return  1=检测到过零点，0=未检测到
 * @note    用于开环启动阶段，只要检测到过零点就返回真
 *          不需要等待30度延时，因为开环是固定时间换相
 *          调用后会清除 zero_detected 标志，避免重复计数
 */
uint8_t BLDC_COMP_IsZeroDetected(void)
{
    if (s_zeroCross.zero_detected)
    {
        s_zeroCross.zero_detected = 0; /* 清除标志，避免重复计数 */
        return 1;
    }
    return 0;
}

/**
 * @brief   检查是否可以换相（过零点+30°延时）
 * @param   无
 * @return  1=可以换相，0=等待中
 * @note    用于闭环运行阶段，需要在主循环中周期性调用此函数
 */
uint8_t BLDC_COMP_IsCommutationReady(void)
{
    if (s_zeroCross.zero_detected && !s_zeroCross.commutation_ready)
    {
        /* 计算从过零点到现在的采样次数差 */
        uint32_t elapsed_samples = s_sampleCount - s_zeroCross.zero_detect_time;

        /* 检查是否达到30°延时 */
        if (elapsed_samples >= s_zeroCross.delay_30_degree_time)
        {
            s_zeroCross.commutation_ready = 1; /* 可以换相 */
            s_zeroCross.zero_detected = 0;     /* 复位标志，等待下次过零点 */
            return 1;
        }
    }

    return 0;
}

/**
 * @brief   获取当前电机转速（RPM）
 * @param   无
 * @return  当前转速（RPM），0表示无效
 * @note    使用换相周期计算转速，与速度控制函数使用相同的数据源和公式
 *          转速(RPM) = speed_factor / 换相周期（采样数）
 */

uint16_t BLDC_COMP_GetSpeed(void)
{
    /* 获取换相周期 */
    uint32_t last_comm_period = BLDC_Motor_GetLastCommPeriod();

    /* 使用换相周期计算转速 */
    if (last_comm_period == 0 || last_comm_period > 400)
    {
        return 0; /* 无效或转速太低 */
    }

    /* 获取转速转换系数（根据极对数和采样周期计算） */
    uint32_t speed_factor = BLDC_Motor_GetSpeedFactor();

    /* 转速计算：speed_factor / 换相周期（采样数）= RPM */
    /* 公式推导：60,000,000us / (换相周期us × 6 × 极对数) */
    /*          = 60,000,000 / (采样周期us × 6 × 极对数) / 换相周期（采样数） */
    /*          = speed_factor / last_comm_period */
    uint32_t speed = speed_factor / last_comm_period;

    /* 限制在合理范围 */
    if (speed > 4000) /* 最大转速限制 */
    {
        speed = 4000;
    }

    return (uint16_t)speed;
}

/**
 * @brief   复位过零点检测状态
 * @param   无
 * @return  无
 * @note    切换相位后或启动时调用，清除历史过零点数据
 */
void BLDC_COMP_ResetZeroCross(void)
{
    s_zeroCross.zero_detected = 0;
    s_zeroCross.commutation_ready = 0;
    s_zeroCross.delay_30_degree_time = 0;
    s_zeroCross.last_commutation_time = 0;
    s_zeroCross.stable_comm_count = 0;                 /* 复位闭环稳定计数器 */
    s_zeroCross.filter_level = BLDC_COMP_ReadOutput(); /* 候选电平重置为当前电平 */
    s_zeroCross.filter_count = 0;                      /* 连续计数清零 */
}

/**
 * @brief   获取采样计数器（高精度时间基准）
 * @param   无
 * @return  采样计数器值（每50us递增1）
 * @note    精度为50us，可用于高精度时间测量
 */
uint32_t BLDC_COMP_GetSampleCount(void)
{
    return s_sampleCount;
}

/**
 * @brief   更新换相时刻（用于30°延时计算）
 * @param   commutation_time 当前换相时刻（采样计数器值）
 * @return  无
 * @note    ⚠️ 此函数必须在每次换相时调用，用于记录换相时刻
 *          原理：从换相点到过零点的时间间隔 = 30°电角度
 *          因此 30°延时时间 = 过零点时刻 - 换相时刻
 */
void BLDC_COMP_UpdateCommutationTime(uint32_t commutation_time)
{
    s_zeroCross.last_commutation_time = commutation_time;

    /* 换相后重置滤波状态，让过零检测从当前电平重新开始计数，避免旧候选值污染 */
    s_zeroCross.filter_level = BLDC_COMP_ReadOutput();
    s_zeroCross.filter_count = 0;

    /* 闭环运行阶段：递增稳定换相计数（用于滤波深度切换）*/
    if (BLDC_Motor_GetState() == BLDC_MOTOR_RUNNING)
    {
        if (s_zeroCross.stable_comm_count < 255) /* 防止溢出（uint8_t） */
        {
            s_zeroCross.stable_comm_count++;
        }
    }
}

/**
 * @brief   更新比较器参数（根据档位动态调整）
 * @param   speed_range 当前速度区间（0=低速，1=中速，2=高速）
 * @return  无
 * @note    根据档位动态调整比较器数字滤波器和30度延时计算方式
 *          - 低档（1-25）：滤波器50000，延时右移5位（除以32）
 *          - 中高档（26-100）：滤波器12000，延时右移2位（除以4）
 */
void BLDC_COMP_UpdateSpeedRange(uint8_t speed_range)
{
    /* 检查是否需要更新参数 */
    if (s_currentSpeedRange == speed_range)
    {
        return; /* 相同区间，无需更新 */
    }

    /* 更新当前档位范围 */
    s_currentSpeedRange = speed_range;

    /* 停止比较器 */
    HAL_COMP_Stop(&hcomp);

    /* 根据档位范围调整数字滤波器 */
    if (speed_range == 0) /* 低速区间 */
    {
        /* 低档：滤波器设为50000 */
        hcomp.Init.DigitalFilter = 0;
    }
    else /* 中速或高速区间 */
    {
        /* 中高档：滤波器设为12000 */
        hcomp.Init.DigitalFilter = 0;
    }

    /* 重新初始化比较器 */
    HAL_COMP_Init(&hcomp);

    /* 启动比较器 */
    HAL_COMP_Start(&hcomp);
}
