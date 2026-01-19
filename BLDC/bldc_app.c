/******************************************************************************
 * @file    bldc_app.c
 * @brief   BLDC电机应用层（档位控制、转速管理）
 * @author  cyWu <1917507415@qq.com>
 * @date    2025-11-25
 * @version V1.0.0
 * @history
 *  - V1.0.0, 2025-11-25, cyWu, 首次发布
 ******************************************************************************/

#include "bldc_app.h"
#include "bldc_comp.h"
#include "bldc_motor.h"
#include "log.h"

/* ==================== 私有变量 ==================== */
static BLDC_Gear_t s_currentGear = BLDC_GEAR_OFF; /* 当前档位 */

/* ==================== 转速区间速度环配置（100档位分段配置）==================== */
/**
 * @brief   转速区间配置表
 * @note    将100个档位分成3个转速区间，每个区间使用相同的控制参数：
 *          - 低速区间（1-44档）：500-1500 RPM
 *          - 中速区间（45-88档）：1500-2500 RPM
 *          - 高速区间（89-100档）：2500-2800 RPM
 *          
 *          转速计算公式：RPM = 500 + (gear - 1) * 23
 */
static const BLDC_SpeedProfile_t s_speedRangeProfiles[3] = {
    /* 低速区间：500-1500 RPM (档位1-44) */
    [BLDC_SPEED_RANGE_LOW] = {
        .target_rpm = 0,    /* 动态计算，此处不使用 */
        .tolerance = 2,     /* 速度误差容差（采样点） */
        .oc_limit_a = 3.0f, /* 过流阈值（安培） */
        .inc = {
            .high_threshold = 20, /* 大偏差阈值（采样点） */
            .mid_threshold = 5,   /* 中偏差阈值（采样点） */
            .high_step = 50,      /* 大偏差时的调整步长（CCR） */
            .mid_step = 10,        /* 中偏差时的调整步长（CCR） */
            .low_step = 1,        /* 小偏差时的调整步长（CCR） */
            .high_count = 0,      /* 大调整立即执行 */
            .mid_count = 1,       /* 中调整计数到1 */
            .low_count = 5        /* 小调整计数到5 */
        },
        .dec = {
            .high_threshold = 20, /* 大偏差阈值（采样点） */
            .mid_threshold = 5,   /* 中偏差阈值（采样点） */
            .high_step = 50,      /* 大偏差时的调整步长（CCR） */
            .mid_step = 20,       /* 中偏差时的调整步长（CCR） */
            .low_step = 1,        /* 小偏差时的调整步长（CCR） */
            .high_count = 0,      /* 大调整立即执行 */
            .mid_count = 0,       /* 中调整立即执行 */
            .low_count = 7        /* 小调整计数到7 */
        }
    },

    /* 中速区间：1500-2500 RPM (档位45-88) */
    [BLDC_SPEED_RANGE_MID] = {
        .target_rpm = 0,    /* 动态计算，此处不使用 */
        .tolerance = 2,     /* 速度误差容差（采样点） */
        .oc_limit_a = 4.0f, /* 过流阈值（安培） */
        .inc = {
            .high_threshold = 8,  /* 大偏差阈值（采样点） */
            .mid_threshold = 5,   /* 中偏差阈值（采样点） */
            .high_step = 40,      /* 大偏差时的调整步长（CCR） */
            .mid_step = 10,       /* 中偏差时的调整步长（CCR） */
            .low_step = 1,        /* 小偏差时的调整步长（CCR） */
            .high_count = 0,      /* 大调整立即执行 */
            .mid_count = 1,       /* 中调整计数到1 */
            .low_count = 5        /* 小调整计数到5 */
        },
        .dec = {
            .high_threshold = 8,  /* 大偏差阈值（采样点） */
            .mid_threshold = 5,   /* 中偏差阈值（采样点） */
            .high_step = 20,      /* 大偏差时的调整步长（CCR） */
            .mid_step = 10,       /* 中偏差时的调整步长（CCR） */
            .low_step = 1,        /* 小偏差时的调整步长（CCR） */
            .high_count = 0,      /* 大调整立即执行 */
            .mid_count = 1,       /* 中调整计数到1 */
            .low_count = 3        /* 小调整计数到3 */
        }
    },

    /* 高速区间：2500-2800 RPM (档位89-100) */
    [BLDC_SPEED_RANGE_HIGH] = {
        .target_rpm = 0,    /* 动态计算，此处不使用 */
        .tolerance = 1,     /* 速度误差容差（采样点） */
        .oc_limit_a = 5.0f, /* 过流阈值（安培） */
        .inc = {
            .high_threshold = 6,  /* 大偏差阈值（采样点） */
            .mid_threshold = 4,   /* 中偏差阈值（采样点） */
            .high_step = 3,       /* 大偏差时的调整步长（CCR） */
            .mid_step = 3,        /* 中偏差时的调整步长（CCR） */
            .low_step = 1,        /* 小偏差时的调整步长（CCR） */
            .high_count = 1,      /* 大调整计数到1 */
            .mid_count = 2,       /* 中调整计数到2 */
            .low_count = 5        /* 小调整计数到5 */
        },
        .dec = {
            .high_threshold = 10, /* 大偏差阈值（采样点） */
            .mid_threshold = 6,   /* 中偏差阈值（采样点） */
            .high_step = 50,      /* 大偏差时的调整步长（CCR） */
            .mid_step = 50,       /* 中偏差时的调整步长（CCR） */
            .low_step = 1,        /* 小偏差时的调整步长（CCR） */
            .high_count = 0,      /* 大调整立即执行 */
            .mid_count = 1,       /* 中调整计数到1 */
            .low_count = 1        /* 小调整计数到1 */
        }
    }
};

/* ==================== 初始化接口 ==================== */
/**
 * @brief   初始化BLDC应用层
 * @param   无
 * @return  无
 */
void bldc_app_init(void)
{
    /* 初始化底层电机控制模块 */
    BLDC_Motor_Init();

    /* ========== 计算转速转换系数（根据极对数和采样周期） ========== */
    /**
     * @brief   转速转换系数计算公式
     * @note    转速(RPM) = speed_factor / 换相周期（采样数）
     *
     *          推导过程：
     *          1. 每个电气周期 = 6步换相
     *          2. 机械转速(RPM) = (电气频率 × 60秒) / 极对数
     *          3. 换相周期(us) = 换相周期(采样数) × 采样周期(us)
     *          4. 电气频率(Hz) = 1,000,000us / (换相周期(us) × 6步)
     *          5. 因此：RPM = (1,000,000 × 60) / (换相周期(采样数) × 采样周期(us) × 6步 × 极对数)
     *                      = 60,000,000 / (采样周期(us) × 6 × 极对数) / 换相周期(采样数)
     *
     *          示例：
     *          - 采样周期=50us，极对数=5：speed_factor = 60,000,000 / (50×6×5) = 40,000
     *          - 采样周期=50us，极对数=2：speed_factor = 60,000,000 / (50×6×2) = 100,000
     *          - 采样周期=100us，极对数=5：speed_factor = 60,000,000 / (100×6×5) = 20,000
     */
    uint32_t speed_factor = 60000000UL / (BLDC_SAMPLE_PERIOD_US * 6 * BLDC_POLE_PAIRS);

    /* 配置电机总参数（包含正反转、启动参数、开环参数、速度环配置） */
    BLDC_Motor_Config_t motor_cfg = {
        .forward = 1, /* 1=正向，0=反向，可在运行时动态修改 */
        .align = {
            .align_step = BLDC_STEP_1, /* 预定位相位 */
            .align_pwm_ccr = 250,      /* 预定位PWM CCR=250 */
            .align_time_samples = 100,   /* 预定位持续时间：3个采样周期 */
        },
        .openloop = {
            .comm_delay = 110,      /* 启动换相间隔：110×50us=5.5ms */
            .openloop_time_ms = 500, /* 开环运行200ms后强制进入闭环 */
            .startup_pwm_ccr = 300, /* 启动PWM CCR=300（约25%）*/
            .pwm_step_ccr = 2,      /* 每次换相增加PWM CCR=2 */
            .pwm_max_ccr = 400,     /* 开环阶段PWM最大CCR=500 */
        },
        .pwm_limit = {
            .pwm_min_ccr = 50,  /* 最小PWM CCR=50 (约4%) */
            .pwm_max_ccr = 1080 /* 最大PWM CCR=1080 (90%) */
        },
        .protection = {
            .comm_timeout_factor = 8, /* 超时系数：目标周期的4倍 */
            .force_total_limit = 30   /* 累计强制换相上限→停机 */
        },
        .current = {
            .oc_trip_duration_ms = 200, /* 过流判定时间：持续超过阈值200ms即停机 */
            .shunt_res_ohm = 0.05f,     /* 采样电阻=0.05Ω */
            .sense_gain = 1.0f          /* 电流放大倍数（无放大=1.0）*/
        },
        .speed_ctrl = {
            .speed_adjust_period = 2, /* 调整周期=2ms（给电机足够的响应时间）*/
            .min_speed_rpm = 500,     /* 最低转速：500 RPM */
            .max_speed_rpm = 2800     /* 最高转速：2800 RPM（档位100：500+99*23=2777） */
        },
        .speed_profiles = s_speedRangeProfiles,  /* 速度环区间配置表（3个区间） */
        .speed_profile_count = 3,                /* 区间配置表元素个数（3个区间） */
        .speed_factor = speed_factor             /* 转速转换系数（预计算） */
    };
    BLDC_Motor_SetConfig(&motor_cfg);

    /* 初始化档位 */
    s_currentGear = BLDC_GEAR_OFF;

    Log("BLDC app initialized\r\n");
}

/* ==================== 档位控制接口 ==================== */
/**
 * @brief   设置BLDC电机档位
 * @param   gear 目标档位
 * @return  无
 */
void bldc_set_gear(BLDC_Gear_t gear)
{
    /* 边界检查 */
    if (gear >= BLDC_GEAR_MAX)
    {
        Log("BLDC invalid gear: %d\r\n", gear);
        return;
    }

    /* 更新档位 */
    s_currentGear = gear;
    Log("BLDC gear set to: %d\r\n", gear);

    /* 根据档位执行相应操作 */
    if (gear == BLDC_GEAR_OFF)
    {
        /* 关闭电机 */
        BLDC_Motor_Stop();
        Log("BLDC motor stopped\r\n");
    }
    else
    {
        /* 直接设置档位，底层自动处理PWM占空比和目标转速 */
        BLDC_Motor_SetSpeed(gear);
        Log("BLDC speed set to gear: %d\r\n", gear);

        /* 如果电机未运行，启动电机 */
        if (BLDC_Motor_GetState() == BLDC_MOTOR_STOP)
        {
            BLDC_Motor_Start();
            Log("BLDC motor started\r\n");
        }
    }
}

/**
 * @brief   获取当前BLDC电机档位
 * @param   无
 * @return  当前档位
 */
BLDC_Gear_t bldc_get_gear(void)
{
    return s_currentGear;
}

/**
 * @brief   增加档位（+1档）
 * @param   无
 * @return  无
 */
void bldc_gear_increase(void)
{
    if (s_currentGear < BLDC_GEAR_MAX - 1)
    {
        bldc_set_gear((BLDC_Gear_t)(s_currentGear + 1));
    }
    else
    {
        Log("BLDC already at max gear\r\n");
    }
}

/**
 * @brief   减少档位（-1档）
 * @param   无
 * @return  无
 */
void bldc_gear_decrease(void)
{
    if (s_currentGear > BLDC_GEAR_OFF)
    {
        bldc_set_gear((BLDC_Gear_t)(s_currentGear - 1));
    }
    else
    {
        Log("BLDC already at OFF\r\n");
    }
}

/* ==================== 方向控制接口 ==================== */
/**
 * @brief   设置BLDC电机方向
 * @param   direction 方向（1=正向，0=反向）
 * @return  无
 */
void bldc_set_direction(uint8_t direction)
{
    BLDC_Motor_SetDirection(direction);
}

/**
 * @brief   获取当前BLDC电机方向
 * @param   无
 * @return  方向（1=正向，0=反向）
 */
uint8_t bldc_get_direction(void)
{
    return BLDC_Motor_GetDirection();
}

/* ==================== 状态查询接口 ==================== */
/**
 * @brief   查询BLDC电机是否正在运行
 * @param   无
 * @return  true-电机运行中，false-电机已停止
 */
bool bldc_is_running(void)
{
    BLDC_Motor_State_t state = BLDC_Motor_GetState();
    return (state != BLDC_MOTOR_STOP);
}

/**
 * @brief   获取当前电机转速（RPM）
 * @param   无
 * @return  转速（RPM），0表示停止或无效
 */
uint16_t bldc_get_speed_rpm(void)
{
    return BLDC_COMP_GetSpeed();
}

/**
 * @brief   获取堵转状态
 * @param   无
 * @return  true=已堵转停机，false=正常
 */
bool bldc_is_stalled(void)
{
    return (BLDC_Motor_IsStalledFlag() != 0);
}

/**
 * @brief   清除堵转标志
 * @param   无
 * @return  无
 * @note    用于LED显示后手动清除标志
 */
void bldc_clear_stalled_flag(void)
{
    BLDC_Motor_ClearStalledFlag();
    Log("BLDC stalled flag cleared\r\n");
}

/**
 * @brief   强制停止BLDC电机
 * @param   无
 * @return  无
 */
void bldc_force_stop(void)
{
    BLDC_Motor_Stop();
    s_currentGear = BLDC_GEAR_OFF;
    Log("BLDC motor force stopped\r\n");
}

/**
 * @brief   BLDC应用层处理函数
 * @param   无
 * @return  无
 */
void bldc_app_handle(void)
{
    /* 调用底层电机控制处理函数 */
    BLDC_Motor_Handle();
}
