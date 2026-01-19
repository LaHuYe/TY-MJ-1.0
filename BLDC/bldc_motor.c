/******************************************************************************
 * @file    bldc_motor.c
 * @brief   BLDC电机控制模块（六步换相 + 闭环速度控制）
 * @author  cyWu <1917507415@qq.com>
 * @date    2025-11-25
 * @version V2.0.0 - 高性能换相优化版本
 *
 * @note    功能特性：
 *          - 六步换相控制（开环启动 + 闭环运行）
 *          - 基于过零点检测（ZCD）的无传感器控制
 *          - 档位控制（5档可调）
 *          - 自动速度闭环（5ms调整周期）
 *          - 支持正反转（BLDC_MOTOR_FORWARD 宏配置）
 *          - ⚡ 高性能换相优化（直接寄存器操作，换相耗时 < 1us）
 *
 *          电机参数：5极对，最高3000 RPM
 *          硬件架构：上桥PMOS（PWM控制），下桥NMOS（GPIO控制）
 *
 *          如果电机反转，修改 BLDC_MOTOR_FORWARD 为 0
 *
 * @performance 性能优化说明（V2.0.0新增）：
 *          所有换相操作统一使用 Motor_SetPhase() 函数：
 *          1. 直接操作TIM->CCR寄存器，替代HAL函数调用
 *          2. 使用GPIO BSRR寄存器原子操作，一次写入完成所有GPIO设置
 *          3. 换相耗时从 ~10us 降低到 < 1us（提升10倍以上）⚡
 *          4. 提高过零点检测精度，改善高速运行稳定性
 *
 *          所有换相操作统一使用 Motor_SetPhase() 函数，内部自动根据状态选择PWM值
 ******************************************************************************/
#include "bldc_motor.h"
#include "adc.h"
#include "bldc_comp.h"
#include "common.h"
int32_t test = 0;
uint32_t test_pwm = 0;
/* ==================== 私有变量结构体定义 ==================== */

/**
 * @brief   电机状态结构体
 */
typedef struct
{
    BLDC_Motor_State_t state;       /* 电机运行状态 */
    BLDC_Motor_Step_t current_step; /* 当前换相步骤 */
    uint32_t startup_comm_count;    /* 启动阶段换相计数器 */
} BLDC_Motor_StateData_t;

/**
 * @brief   开环启动结构体
 */
typedef struct
{
    uint16_t current_startup_pwm; /* 当前启动PWM CCR（动态调整）*/
    uint32_t align_end_time;      /* 预定位结束时刻（采样计数器）*/
    uint8_t align_completed;      /* 预定位完成标志 */
    uint32_t startup_begin_time;  /* 开环启动开始时刻 */
} BLDC_Motor_StartupData_t;

/**
 * @brief   闭环速度控制结构体
 */
typedef struct
{
    uint32_t last_comm_period;       /* 上次换相周期（单位：采样次数×50us）*/
    uint16_t target_speed;           /* 目标转速（单位：RPM）*/
    BLDC_Gear_t current_gear;        /* 当前档位（用于过流阈值选择）*/
    uint32_t last_speed_adjust_time; /* 上次速度调整时间（采样计数器）*/
    uint32_t last_comm_sample_time;  /* 上次换相时刻（采样计数器）*/
    uint16_t running_pwm_ccr;        /* 运行时PWM CCR值（闭环自动调整）*/
} BLDC_Motor_SpeedControlData_t;

/**
 * @brief   PWM调整计数结构体
 */
typedef struct
{
    uint16_t increase_count_high; /* 加速时大偏差计数（偏差>high_threshold）*/
    uint16_t increase_count_mid;  /* 加速时中偏差计数（偏差>mid_threshold且<=high_threshold）*/
    uint16_t increase_count_low;  /* 加速时小偏差计数（偏差<=mid_threshold）*/
    uint16_t decrease_count_high; /* 减速时大偏差计数（偏差>high_threshold）*/
    uint16_t decrease_count_mid;  /* 减速时中偏差计数（偏差>mid_threshold且<=high_threshold）*/
    uint16_t decrease_count_low;  /* 减速时小偏差计数（偏差<=mid_threshold）*/
} BLDC_Motor_PwmAdjustCount_t;

/**
 * @brief   强制换相统计结构体
 */
typedef struct
{
    uint16_t forced_comm_total; /* 累计强制换相计数 */
    uint16_t zero_cross_count;  /* 过零计数（用于累计强制换相计数衰减）*/
} BLDC_Motor_ForceCommData_t;

/**
 * @brief   电流保护结构体
 */
typedef struct
{
    uint8_t current_limit_active;     /* 电流限制激活标志 */
    uint32_t current_high_start_time; /* 过流开始时刻（用于判定）*/
} BLDC_Motor_CurrentProtectData_t;

/**
 * @brief   电机运行时数据（整合所有运行时变量）
 */
typedef struct
{
    BLDC_Motor_StateData_t state;            /* 电机状态 */
    BLDC_Motor_StartupData_t startup;        /* 开环启动数据 */
    BLDC_Motor_SpeedControlData_t speed;     /* 闭环速度控制数据 */
    BLDC_Motor_PwmAdjustCount_t pwm_count;   /* PWM调整计数 */
    BLDC_Motor_ForceCommData_t force_comm;   /* 强制换相统计 */
    BLDC_Motor_CurrentProtectData_t current; /* 电流保护数据 */
    uint8_t stalled_flag;                    /* 堵转标志：1=已堵转停机，0=正常 */
    CircularQueue current_filter_queue;      /* 电流滤波队列 */
} BLDC_Motor_RuntimeData_t;

/* ==================== 私有变量 ==================== */

/* 电机总配置（包含正反转、启动参数、开环参数等）*/
BLDC_Motor_Config_t s_motorConfig = {
    .forward = 1, /* 默认正向换相：1=正向，0=反向 */
    .align = {
        .align_step = BLDC_STEP_1, /* 预定位相位（步骤1）*/
        .align_pwm_ccr = 250,      /* 预定位PWM CCR=250（约21%）*/
        .align_time_samples = 3,   /* 预定位持续时间：3个采样周期（3×50us=150us）*/
    },
    .openloop = {
        .comm_delay = 100,      /* 100×50us=5ms起始换相间隔（慢启动，可靠）*/
        .openloop_time_ms = 90, /* 开环运行500ms后强制进入闭环 */
        .startup_pwm_ccr = 300, /* 启动阶段起始PWM CCR=120（约10%占空比，逐渐增加）*/
        .pwm_step_ccr = 2,      /* 每次换相增加PWM CCR=2（精细控制）*/
        .pwm_max_ccr = 500      /* 开环阶段PWM最大CCR=500（约42%占空比）*/
    },
    .pwm_limit = {
        .pwm_min_ccr = 50,  /* 最小PWM CCR=50 (约4%) */
        .pwm_max_ccr = 1080 /* 最大PWM CCR=1080 (90%) */
    },
    .protection = {
        .comm_timeout_factor = 4, /* 超时系数：目标周期的4倍 */
        .force_total_limit = 50   /* 累计强制换相上限→停机 */
    },
    .current = {
        .oc_trip_duration_ms = 50, /* 过流判定时间：持续超过阈值50ms即停机 */
        .shunt_res_ohm = 0.02f,    /* 采样电阻=0.02Ω */
        .sense_gain = 1.0f         /* 电流放大倍数（无放大=1.0）*/
    },
    .speed_ctrl = {
        .speed_adjust_period = 2, /* 调整周期=2ms（给电机足够的响应时间）*/
        .min_speed_rpm = 500,     /* 最低转速：500 RPM */
        .max_speed_rpm = 4000     /* 最高转速：4000 RPM */
    },
    .speed_factor = 40000 /* 默认转速转换系数（会在bldc_app_init中根据极对数重新计算）*/
};

/* 电机运行时数据（整合所有运行时变量）*/
BLDC_Motor_RuntimeData_t s_motor;

/* ==================== 私有函数声明 ==================== */
static void Motor_SetPhase(BLDC_Motor_Step_t step);
static void Motor_Commutate(void);
static void Motor_StartupHandle(void);
static void Motor_RunningHandle(void);
static void Motor_SpeedControl(void);

/* ==================== 复位辅助函数 ==================== */
/**
 * @brief   统一复位所有变量
 * @param   reset_mode 复位模式：
 *                     0 = 完全复位（停止时使用，复位所有变量）
 *                     1 = 启动复位（启动时使用，部分复位，保留target_speed和last_comm_sample_time）
 *                     2 = 初始化复位（初始化时使用，复位所有变量）
 * @return  无
 */
static void Motor_ResetAllVars(uint8_t reset_mode)
{
    /* ========== 复位保护变量（所有模式都需要） ========== */
    s_motor.force_comm.forced_comm_total = 0;    /* 清零累计强制换相次数 */
    s_motor.force_comm.zero_cross_count = 0;     /* 清零过零点计数 */
    s_motor.current.current_limit_active = 0;    /* 关闭过流限制标志 */
    s_motor.current.current_high_start_time = 0; /* 清零过流开始时间 */
    s_motor.stalled_flag = 0;                    /* 清除堵转标志 */

    /* ========== 复位速度控制变量 ========== */
    s_motor.speed.last_comm_period = 0;       /* 清零上一换相周期 */
    s_motor.speed.last_speed_adjust_time = 0; /* 清零上次速度调整时间 */

    if (reset_mode == 0 || reset_mode == 2) /* 完全复位或初始化复位 */
    {
        s_motor.speed.target_speed = 0;                                     /* 清除目标速度 */
        s_motor.speed.last_comm_sample_time = 0;                            /* 清零上次换相采样点 */
        s_motor.speed.running_pwm_ccr = s_motorConfig.openloop.pwm_max_ccr; /* 恢复默认PWM占空 */
    }
    /* reset_mode == 1 (启动复位) 时，不复位target_speed和last_comm_sample_time，由启动函数设置 */

    /* ========== 复位启动相关变量 ========== */
    s_motor.state.startup_comm_count = 0;    /* 启动换相计数清零 */
    s_motor.startup.current_startup_pwm = 0; /* 启动PWM清零 */
    s_motor.startup.align_end_time = 0;      /* 预定位结束时间清零 */
    s_motor.startup.align_completed = 0;     /* 预定位完成标志清零 */
    s_motor.startup.startup_begin_time = 0;  /* 启动开始时间清零 */

    /* ========== 初始化复位模式：额外复位状态变量 ========== */
    if (reset_mode == 2)
    {
        s_motor.state.state = BLDC_MOTOR_STOP;    /* 初始状态设为停止 */
        s_motor.state.current_step = BLDC_STEP_1; /* 初始换相步进设为步进1 */
    }
}

/**
 * @brief   初始化电机控制模块
 * @param   无
 * @return  无
 */
void BLDC_Motor_Init(void)
{
    /* 调用硬件初始化（GPIO、PWM、比较器） */
    BLDC_Hardware_Init();

    /* 初始化比较器运行时变量 */
    BLDC_COMP_Init();

    /* 统一复位所有变量（初始化模式） */
    Motor_ResetAllVars(2);

    /* 初始化电流移动平均滤波队列 */
    initQueue(&s_motor.current_filter_queue);

    /* 设置默认PWM CCR值（复位函数已设置，这里确保正确） */
    s_motor.speed.running_pwm_ccr = s_motorConfig.openloop.pwm_max_ccr; /* 运行PWM恢复默认 */
}

/**
 * @brief   启动电机（开环启动）
 * @param   无
 * @return  无
 * @note    启动流程：
 *          1. 预定位阶段：固定相位保持一段时间，让转子对齐
 *          2. 开环换相阶段：逐渐加速，PWM逐渐增大
 *          3. 检测到过零点后进入闭环运行
 */
void BLDC_Motor_Start(void)
{
    /* 统一复位所有变量（启动模式） */
    Motor_ResetAllVars(1);

    /* 切换到开环启动状态 */
    s_motor.state.state = BLDC_MOTOR_STARTUP; /* 状态切换为启动 */

    /* 设置启动相关参数（复位后设置） */
    s_motor.state.startup_comm_count = 0;                                                                 /* 启动换相计数清零 */
    s_motor.speed.last_comm_sample_time = BLDC_COMP_GetSampleCount();                                     /* 记录当前采样时间基准 */
    s_motor.startup.current_startup_pwm = s_motorConfig.openloop.startup_pwm_ccr;                         /* 启动PWM设为配置值 */
    s_motor.startup.align_end_time = BLDC_COMP_GetSampleCount() + s_motorConfig.align.align_time_samples; /* 计算预定位结束点 */
    s_motor.startup.align_completed = 0;                                                                  /* 预定位标志清零 */
    s_motor.startup.startup_begin_time = HAL_GetTick();                                                   /* 记录启动开始时间 */

    /* 复位过零点检测 */
    BLDC_COMP_ResetZeroCross();

    /* 预定位阶段：设置预定位相位 */
    s_motor.state.current_step = s_motorConfig.align.align_step; /* 当前步进设为预定位相位 */
    Motor_SetPhase(s_motor.state.current_step);
}

/**
 * @brief   停止电机
 * @param   无
 * @return  无
 */
void BLDC_Motor_Stop(void)
{
    /* 关闭所有PWM和GPIO输出 */
    BLDC_Stop_All_Output();

    /* 统一复位所有变量（完全复位模式） */
    Motor_ResetAllVars(0);

    /* 切换到停止状态 */
    s_motor.state.state = BLDC_MOTOR_STOP;

    /* 复位过零点检测 */
    BLDC_COMP_ResetZeroCross();
}

/**
 * @brief   主循环处理（需周期调用）
 */
void BLDC_Motor_Handle(void)
{
    switch (s_motor.state.state)
    {
    case BLDC_MOTOR_STOP:
        /* 停止状态，什么都不做 */
        break;

    case BLDC_MOTOR_STARTUP:
        /* 开环启动阶段：强制定时换相 */
        Motor_StartupHandle();
        break;

    case BLDC_MOTOR_RUNNING:
        /* 闭环运行阶段：过零点换相 */
        Motor_RunningHandle();
        /* 闭环速度控制*/
        Motor_SpeedControl();
        break;
    }
}

/**
 * @brief   开环启动处理函数
 * @param   无
 * @return  无
 * @note    启动分两阶段：
 *          阶段1-预定位：固定相位保持一段时间，让转子对齐到初始位置
 *          阶段2-开环加速：固定时间间隔换相，时间逐渐缩短，PWM逐渐增大
 *          阶段3-检测到过零点后切换闭环
 *          时间精度：50us（使用采样计数器，避免ms转换精度损失）
 */
static void Motor_StartupHandle(void)
{
    uint32_t current_time_ms = HAL_GetTick();
    uint32_t current_sample_time = BLDC_COMP_GetSampleCount(); /* 50us精度计数器 */

    /* ========== 阶段1：预定位阶段 ========== */
    if (!s_motor.startup.align_completed)
    {
        /* 检查预定位时间是否到达（使用50us精度采样计数器）*/
        if (current_sample_time >= s_motor.startup.align_end_time)
        {
            /* 预定位完成，切换到第一个换相步骤 */
            s_motor.startup.align_completed = 1;
            /* 预定位结束，稳定计数清零 */

            /* 执行第一次换相（从预定位相位切换到步骤1）*/
            if (s_motorConfig.forward)
            {
                /* 正向：预定位相位的下一步 */
                s_motor.state.current_step = (BLDC_Motor_Step_t)(s_motorConfig.align.align_step + 1);
                if (s_motor.state.current_step > BLDC_STEP_6)
                {
                    s_motor.state.current_step = BLDC_STEP_1;
                }
            }
            else
            {
                /* 反向：预定位相位的上一步 */
                s_motor.state.current_step = (BLDC_Motor_Step_t)(s_motorConfig.align.align_step - 1);
                if (s_motor.state.current_step < BLDC_STEP_1)
                {
                    s_motor.state.current_step = BLDC_STEP_6;
                }
            }
            /* 预定位完成后第一次换相，使用启动PWM值 */
            Motor_SetPhase(s_motor.state.current_step);

            /* 重置时间基准（开始计时下一次换相）*/
            s_motor.speed.last_comm_sample_time = current_sample_time;
        }
        return; /* 预定位期间不执行换相 */
    }

    /* ========== 阶段2：开环过零点换相阶段（过零点+30°延时 + PWM逐渐增加）========== */

    /* ========== 时间检测：开环运行时间达到设定值后强制进入闭环 ========== */
    uint32_t openloop_elapsed_time = current_time_ms - s_motor.startup.startup_begin_time;
    if (openloop_elapsed_time >= s_motorConfig.openloop.openloop_time_ms)
    {
        /* 开环运行时间到达，强制切换到闭环 */
        s_motor.state.state = BLDC_MOTOR_RUNNING;
        s_motor.speed.last_comm_sample_time = BLDC_COMP_GetSampleCount(); /* 初始化换相时间基准 */

        /* ⚡ 关键：使用实际换相周期作为闭环初始周期，实现平滑过渡 */
        if (s_motor.speed.last_comm_period == 0)
        {
            s_motor.speed.last_comm_period = s_motorConfig.openloop.comm_delay; /* 如果没有实际周期，使用配置值 */
        }

        /* 限制初始周期范围，避免过大或过小 */
        if (s_motor.speed.last_comm_period < 3) /* 最小周期：3个采样周期（150us）→ 最高约13000 RPM */
        {
            s_motor.speed.last_comm_period = 3;
        }
        if (s_motor.speed.last_comm_period > 400) /* 最大周期：400个采样周期（20ms）→ 最低100 RPM */
        {
            s_motor.speed.last_comm_period = 400;
        }
        return; /* 立即返回，下次进入Motor_RunningHandle */
    }

    /* ========== 过零点检测+30°延时换相（与闭环相同的换相方式）========== */
    if (BLDC_COMP_IsCommutationReady())
    {
        /* 检测到过零点+30°延时，执行换相 */
        s_motor.state.startup_comm_count++;
        Motor_Commutate();

        /* ========== 开环PWM逐渐增加（每次换相增加扭矩）========== */
        if (s_motor.startup.current_startup_pwm < s_motorConfig.openloop.pwm_max_ccr)
        {
            s_motor.startup.current_startup_pwm += s_motorConfig.openloop.pwm_step_ccr;
            if (s_motor.startup.current_startup_pwm > s_motorConfig.openloop.pwm_max_ccr)
            {
                s_motor.startup.current_startup_pwm = s_motorConfig.openloop.pwm_max_ccr; /* 限制最大值 */
            }
        }
    }
    else
    {
        /* ========== 开环超时保护：如果长时间没有过零点，强制换相 ========== */
        /* 使用配置的启动换相间隔作为超时阈值 */
        uint32_t timeout_samples = s_motorConfig.openloop.comm_delay; /* 超时=配置值 */

        if ((current_sample_time - s_motor.speed.last_comm_sample_time) >= timeout_samples)
        {

            /* 超时强制换相（开环初期可能还没有稳定的过零点）*/
            s_motor.state.startup_comm_count++;
            Motor_Commutate();
        }
    }
}

/**
 * @brief   闭环运行处理函数
 * @param   无
 * @return  无
 * @note    基于过零点+30°延时进行换相，并进行闭环速度控制
 *          增加强制换相、失锁判断、堵转停机等保护功能
 */
static void Motor_RunningHandle(void)
{
    uint32_t now = BLDC_COMP_GetSampleCount();

    /* ========== 检查累计强制换相次数（堵转停机保护）========== */
    if (s_motor.force_comm.forced_comm_total >= s_motorConfig.protection.force_total_limit)
    {
        /* 累计强制换相次数过多，判定长期堵转，停机保护 */
        BLDC_Motor_Stop();
        s_motor.stalled_flag = 1; /* 设置堵转标志 */
        return;
    }

    /* ========== 正常过零换相 ========== */
    if (BLDC_COMP_IsCommutationReady())
    {

        /* 累计强制换相计数自然衰减（每6次过零减1，最小为0）*/
        /* 6次过零 = 1个完整电气周期（360°电角度）*/
        s_motor.force_comm.zero_cross_count++;
        if (s_motor.force_comm.zero_cross_count >= 20)
        {
            s_motor.force_comm.zero_cross_count = 0; /* 重置计数器 */
            if (s_motor.force_comm.forced_comm_total > 0)
            {
                s_motor.force_comm.forced_comm_total--; /* 每6次过零才减1 */
            }
        }
        Motor_Commutate();
    }
    else
    {
        /* ========== 换相超时强制换相 ========== */
        /* 计算超时阈值：使用上次实际换相周期 */
        uint32_t timeout_samples = s_motor.speed.last_comm_period * s_motorConfig.protection.comm_timeout_factor;

        /* 检查是否超时 */
        if ((now - s_motor.speed.last_comm_sample_time) >= timeout_samples)
        {
            /* 超时，执行强制换相 */
            s_motor.force_comm.forced_comm_total++; /* 累计强制换相计数 */
            Motor_Commutate();

            /* 连续强制换相次数过多，判定闭环失锁，切回开环重新启动 */
            s_motor.state.state = BLDC_MOTOR_STARTUP;
            s_motor.state.startup_comm_count = 0;
            s_motor.startup.startup_begin_time = HAL_GetTick(); /* 重新记录启动时间 */
            s_motor.force_comm.zero_cross_count = 0;            /* 清零过零计数 */

            /* 重新初始化开环过零点换相参数（回到起始值）*/
            s_motor.startup.current_startup_pwm = s_motorConfig.openloop.startup_pwm_ccr;

            /* 复位PWM到启动CCR值 */
            s_motor.speed.running_pwm_ccr = s_motorConfig.openloop.startup_pwm_ccr;

            /* 复位过零点检测状态 */
            BLDC_COMP_ResetZeroCross();

            return;
        }
    }
}

/**
 * @brief   获取目标转速对应的期望换相周期
 * @param   target_speed 目标转速（RPM）
 * @return  期望的换相周期（采样点数）
 * @note    公式：期望周期 = speed_factor / 目标转速
 *          用于速度环控制，直接比较实际周期与期望周期
 */
uint32_t BLDC_COMP_GetTargetPeriod(uint16_t target_speed)
{
    if (target_speed == 0)
    {
        return 0; /* 目标转速为0，返回0 */
    }

    /* 计算期望换相周期：speed_factor / 目标转速 */
    uint32_t target_period = s_motorConfig.speed_factor / target_speed;

    /* 限制在合理范围（3-400个采样点）*/
    if (target_period < 3)
    {
        target_period = 3; /* 最小周期：3个采样点（150us）*/
    }
    else if (target_period > 400)
    {
        target_period = 400; /* 最大周期：400个采样点（20ms）*/
    }

    return target_period;
}

/**
 * @brief   闭环速度控制函数（基于换相周期的直接控制 + 移动平均滤波优化）
 * @param   无
 * @return  无
 * @note    根据实际换相周期与目标周期的差值，自动调整PWM的CCR值。
 *          策略概览：
 *          - 调整周期：s_motorConfig.speed_ctrl.speed_adjust_period ms（默认1ms，对应20个采样点）
 *          - 容差：±3个采样点内不调整（PERIOD_TOLERANCE）
 *          - 加速：
 *              >10采样点：计数5次后执行一次快速增加（PWM_INCREASE_STEP_FAST）
 *              >5 采样点：计数50次后执行一次正常增加（PWM_INCREASE_STEP_NORMAL）
 *              ≤5采样点：立即精细增加（PWM_INCREASE_STEP_SLOW）
 *          - 减速：
 *              >10采样点：立即快速减少（PWM_DECREASE_STEP_FAST）
 *              >5 采样点：立即正常减少（PWM_DECREASE_STEP_NORMAL）
 *              ≤5采样点：计数50次后执行一次精细减少（PWM_DECREASE_STEP_SLOW）
 *          - 处于限流状态时，仅做小幅降PWM并跳过速度调节
 */
static void Motor_SpeedControl(void)
{
    uint32_t current_sample_time = BLDC_COMP_GetSampleCount();

    /* 计算调整间隔：s_motorConfig.speed_ctrl.speed_adjust_period(ms) × 20(采样点/ms) = 采样点数 */
    /* 注：50us采样周期，1ms=20个采样点，确保调整频率适中 */
    uint32_t adjust_interval = s_motorConfig.speed_ctrl.speed_adjust_period * 20;

    /* 速度调整周期控制（避免频繁调整）*/
    if ((current_sample_time - s_motor.speed.last_speed_adjust_time) < adjust_interval)
    {
        return; /* 还在调整周期内，本次不调整 */
    }
    s_motor.speed.last_speed_adjust_time = current_sample_time;

    /* 检查目标转速有效性 */
    if (s_motor.speed.target_speed == 0)
    {
        return;
    }

    /* 获取当前实际换相周期和目标期望周期 */
    if (s_motor.speed.last_comm_period == 0 || s_motor.speed.last_comm_period > 400)
    {
        return; /* 周期无效，不调整（防止除零错误和异常值） */
    }

    /* 计算目标期望换相周期（40,000/目标转速） */
    uint32_t target_period = BLDC_COMP_GetTargetPeriod(s_motor.speed.target_speed);
    if (target_period == 0)
    {
        return;
    }

    /* 计算周期偏差（实际周期 - 目标周期） */
    /* 注意：周期越大表示转速越低，周期越小表示转速越高 */
    int32_t period_error = (int32_t)s_motor.speed.last_comm_period - (int32_t)target_period;

    /* 若处于限流状态，优先降PWM并跳过速度调节，待电流恢复后再进入正常调节 */
    if (s_motor.current.current_limit_active == 1)
    {
        if (s_motor.speed.running_pwm_ccr > s_motorConfig.pwm_limit.pwm_min_ccr)
        {
            uint16_t dec = 3; /* 限流降幅：每周期减1个CCR计数，平滑降低占空比 */
            s_motor.speed.running_pwm_ccr = (s_motor.speed.running_pwm_ccr > dec) ? (s_motor.speed.running_pwm_ccr - dec) : s_motorConfig.pwm_limit.pwm_min_ccr;
            Motor_SetPhase(s_motor.state.current_step); /* 立即应用降幅 */
        }
        return;
    }

    /* 读取当前档位配置，若未注册则直接返回 */
    if (s_motorConfig.speed_profiles == NULL)
    {
        return; /* 没有配置，直接返回 */
    }

    /* 根据档位选择转速区间配置 */
    BLDC_SpeedRange_t speed_range = BLDC_Motor_GetSpeedRange(s_motor.speed.current_gear);
    if (speed_range >= 3) /* 区间枚举值范围：0-2 */
    {
        return; /* 区间无效，直接返回 */
    }
    const BLDC_SpeedProfile_t *profile = &s_motorConfig.speed_profiles[speed_range];

    uint16_t tolerance = profile->tolerance;
    const BLDC_AdjustParam_t *inc = &profile->inc;
    const BLDC_AdjustParam_t *dec = &profile->dec;

    /* 在容差范围内不调整（避免频繁抖动） */
    if (period_error > -(int32_t)tolerance && period_error < (int32_t)tolerance)
    {
        return;
    }
    test = period_error;
    /* ========== PWM调整逻辑（带计数延迟，防抖，可配置） ========== */
    if (period_error > 0)
    {
        /* 实际周期 > 目标周期 → 转速偏低 → 增加CCR（加速） */
        if (!s_motor.current.current_limit_active)
        {
            int32_t abs_error = period_error;

            if (abs_error > inc->high_threshold)
            {
                /* 大偏差：计数到 high_count 后快加；high_count=0 表示立即 */
                s_motor.pwm_count.increase_count_high++;
                s_motor.pwm_count.increase_count_mid = 0;
                s_motor.pwm_count.increase_count_low = 0;

                if (inc->high_count == 0 || s_motor.pwm_count.increase_count_high >= inc->high_count)
                {
                    s_motor.pwm_count.increase_count_high = 0;
                    if (s_motor.speed.running_pwm_ccr < s_motorConfig.pwm_limit.pwm_max_ccr)
                    {
                        s_motor.speed.running_pwm_ccr += inc->high_step;
                        if (s_motor.speed.running_pwm_ccr > s_motorConfig.pwm_limit.pwm_max_ccr)
                        {
                            s_motor.speed.running_pwm_ccr = s_motorConfig.pwm_limit.pwm_max_ccr;
                        }
                        Motor_SetPhase(s_motor.state.current_step);
                    }
                }
            }
            else if (abs_error > inc->mid_threshold)
            {
                /* 中偏差：计数到 mid_count 后正常加 */
                s_motor.pwm_count.increase_count_mid++;
                s_motor.pwm_count.increase_count_high = 0;
                s_motor.pwm_count.increase_count_low = 0;

                if (inc->mid_count == 0 || s_motor.pwm_count.increase_count_mid >= inc->mid_count)
                {
                    s_motor.pwm_count.increase_count_mid = 0;
                    if (s_motor.speed.running_pwm_ccr < s_motorConfig.pwm_limit.pwm_max_ccr)
                    {
                        s_motor.speed.running_pwm_ccr += inc->mid_step;
                        if (s_motor.speed.running_pwm_ccr > s_motorConfig.pwm_limit.pwm_max_ccr)
                        {
                            s_motor.speed.running_pwm_ccr = s_motorConfig.pwm_limit.pwm_max_ccr;
                        }
                        Motor_SetPhase(s_motor.state.current_step);
                    }
                }
            }
            else
            {
                /* 小偏差：计数到 low_count 后微加 */
                s_motor.pwm_count.increase_count_high = 0;
                s_motor.pwm_count.increase_count_mid = 0;
                s_motor.pwm_count.increase_count_low++;

                if (inc->low_count == 0 || s_motor.pwm_count.increase_count_low >= inc->low_count)
                {
                    s_motor.pwm_count.increase_count_low = 0;
                    if (s_motor.speed.running_pwm_ccr < s_motorConfig.pwm_limit.pwm_max_ccr)
                    {
                        s_motor.speed.running_pwm_ccr += inc->low_step;
                        if (s_motor.speed.running_pwm_ccr > s_motorConfig.pwm_limit.pwm_max_ccr)
                        {
                            s_motor.speed.running_pwm_ccr = s_motorConfig.pwm_limit.pwm_max_ccr;
                        }
                        Motor_SetPhase(s_motor.state.current_step);
                    }
                }
            }
        }
    }
    else
    {
        /* 实际周期 < 目标周期 → 转速偏高 → 减少CCR（减速） */
        int32_t abs_error = -period_error;

        if (abs_error > dec->high_threshold)
        {
            /* 大偏差：计数到 high_count 后快减；high_count=0 表示立即 */
            s_motor.pwm_count.decrease_count_high++;
            s_motor.pwm_count.decrease_count_mid = 0;
            s_motor.pwm_count.decrease_count_low = 0;
            s_motor.pwm_count.increase_count_high = 0;

            if (dec->high_count == 0 || s_motor.pwm_count.decrease_count_high >= dec->high_count)
            {
                s_motor.pwm_count.decrease_count_high = 0;
                if (s_motor.speed.running_pwm_ccr > s_motorConfig.pwm_limit.pwm_min_ccr)
                {
                    s_motor.speed.running_pwm_ccr -= dec->high_step;
                    if (s_motor.speed.running_pwm_ccr < s_motorConfig.pwm_limit.pwm_min_ccr)
                    {
                        s_motor.speed.running_pwm_ccr = s_motorConfig.pwm_limit.pwm_min_ccr;
                    }
                    Motor_SetPhase(s_motor.state.current_step);
                }
            }
        }
        else if (abs_error > dec->mid_threshold)
        {
            /* 中偏差：计数到 mid_count 后正常减 */
            s_motor.pwm_count.decrease_count_low = 0;
            s_motor.pwm_count.decrease_count_high = 0;
            s_motor.pwm_count.increase_count_high = 0;

            if (dec->mid_count == 0 || ++s_motor.pwm_count.decrease_count_mid >= dec->mid_count)
            {
                s_motor.pwm_count.decrease_count_mid = 0;
                if (s_motor.speed.running_pwm_ccr > s_motorConfig.pwm_limit.pwm_min_ccr)
                {
                    s_motor.speed.running_pwm_ccr -= dec->mid_step;
                    if (s_motor.speed.running_pwm_ccr < s_motorConfig.pwm_limit.pwm_min_ccr)
                    {
                        s_motor.speed.running_pwm_ccr = s_motorConfig.pwm_limit.pwm_min_ccr;
                    }
                    Motor_SetPhase(s_motor.state.current_step);
                }
            }
        }
        else
        {
            /* 小偏差：计数到 low_count 后微减 */
            s_motor.pwm_count.decrease_count_mid = 0;
            s_motor.pwm_count.decrease_count_high = 0;
            s_motor.pwm_count.increase_count_high = 0;

            s_motor.pwm_count.decrease_count_low++;
            if (dec->low_count == 0 || s_motor.pwm_count.decrease_count_low >= dec->low_count)
            {
                s_motor.pwm_count.decrease_count_low = 0;
                if (s_motor.speed.running_pwm_ccr > s_motorConfig.pwm_limit.pwm_min_ccr)
                {
                    s_motor.speed.running_pwm_ccr -= dec->low_step;
                    if (s_motor.speed.running_pwm_ccr < s_motorConfig.pwm_limit.pwm_min_ccr)
                    {
                        s_motor.speed.running_pwm_ccr = s_motorConfig.pwm_limit.pwm_min_ccr;
                    }
                    Motor_SetPhase(s_motor.state.current_step);
                }
            }
        }
    }
}

/**
 * @brief   执行换相操作（切换到下一步）
 * @param   无
 * @return  无
 * @note    六步换相，根据 BLDC_MOTOR_FORWARD 宏决定正向或反向换相
 *          - 正向：步骤 1→2→3→4→5→6→1...
 *          - 反向：步骤 6→5→4→3→2→1→6...
 */
static void Motor_Commutate(void)
{
    /* 记录换相周期（用于闭环速度控制）*/
    uint32_t current_sample_time = BLDC_COMP_GetSampleCount();
    if (s_motor.speed.last_comm_sample_time > 0)
    {
        /* 换相周期 = 当前时刻 - 上次换相时刻（单位：采样次数） */
        s_motor.speed.last_comm_period = current_sample_time - s_motor.speed.last_comm_sample_time;

        // 限制最大值660转
        // if (s_motor.speed.last_comm_period > 60) // 60个采样点=3ms
        // {
        //     s_motor.speed.last_comm_period = 60;
        // }
        // 限制最小值4000转
        if (s_motor.speed.last_comm_period < 5) // 5个采样点=0.25ms
        {
            s_motor.speed.last_comm_period = 5;
        }
    }
    s_motor.speed.last_comm_sample_time = current_sample_time;

    /* ⚠️ 更新比较器模块的换相时刻（用于30°延时计算） */
    /* 原理：从换相点到过零点 = 30°电角度 */
    BLDC_COMP_UpdateCommutationTime(current_sample_time);

    /* 根据配置的换相方向执行换相 */
    if (s_motorConfig.forward)
    {
        /* 正向换相：步骤递增 */
        s_motor.state.current_step++;
        if (s_motor.state.current_step > BLDC_STEP_6)
        {
            s_motor.state.current_step = BLDC_STEP_1;
        }
    }
    else
    {
        /* 反向换相：步骤递减 */
        s_motor.state.current_step--;
        if (s_motor.state.current_step < BLDC_STEP_1)
        {
            s_motor.state.current_step = BLDC_STEP_6;
        }
    }
    /* ========== 根据运行阶段选择换相方式 ========== */
    if (s_motor.state.state == BLDC_MOTOR_RUNNING)
    {
        /* 闭环运行阶段：使用高性能快速换相（直接寄存器操作）⚡ */
        Motor_SetPhase(s_motor.state.current_step);
    }
    else
    {
        /* 开环启动阶段：使用标准换相函数（内部自动判断PWM值） */
        Motor_SetPhase(s_motor.state.current_step);
    }
}

/**
 * @brief   六步换相（直接寄存器操作，优化换相速度）
 * @param   step 换相步骤（1-6）
 * @return  无
 * @note    性能优化说明：
 *          1. 直接操作定时器CCR寄存器，避免HAL函数调用开销
 *          2. 直接操作GPIO BSRR寄存器，一次写入完成置位/复位
 *          3. 比较器切换优化：直接修改寄存器，避免停止/重启
 *          4. 内部根据电机状态自动选择PWM值：
 *             - 预定位阶段：使用预定位PWM值
 *             - 开环启动阶段：使用当前启动PWM值
 *             - 闭环运行阶段：使用运行PWM值
 */
static void Motor_SetPhase(BLDC_Motor_Step_t step)
{
    /* 根据电机状态自动选择PWM CCR值 */
    uint16_t pwm_ccr;
    if (s_motor.state.state == BLDC_MOTOR_STARTUP && !s_motor.startup.align_completed)
    {
        /* 预定位阶段：使用预定位PWM值 */
        pwm_ccr = s_motorConfig.align.align_pwm_ccr;
    }
    else if (s_motor.state.state == BLDC_MOTOR_STARTUP)
    {
        /* 开环启动阶段：使用当前启动PWM值 */
        pwm_ccr = s_motor.startup.current_startup_pwm;
    }
    else
    {
        /* 闭环运行阶段：使用运行PWM值 */
        pwm_ccr = s_motor.speed.running_pwm_ccr;
    }
    test_pwm = pwm_ccr;
    /* ========== 六步换相逻辑（调用硬件初始化模块函数）========== */
    /* 性能优化点：
     * 1. 使用硬件初始化模块的寄存器宏，直接操作寄存器
     * 2. 先设置PWM和GPIO，最后切换比较器（减少比较器噪声）
     */

    switch (step)
    {
    case BLDC_STEP_1:                        /* U+V- W浮空（检测W相） */
        BLDC_SetPWM_CCR(pwm_ccr, 0, 0);      /* U相PWM，V相0，W相0 */
        BLDC_SetLowSide_GPIO(0, 1, 0);       /* U相下桥0，V相下桥1，W相下桥0 */
        BLDC_COMP_SelectPhase(BLDC_PHASE_W); /* 切换比较器到W相 */
        break;

    case BLDC_STEP_2:                        /* U+W- V浮空（检测V相） */
        BLDC_SetPWM_CCR(pwm_ccr, 0, 0);      /* U相PWM，V相0，W相0 */
        BLDC_SetLowSide_GPIO(0, 0, 1);       /* U相下桥0，V相下桥0，W相下桥1 */
        BLDC_COMP_SelectPhase(BLDC_PHASE_V); /* 切换比较器到V相 */
        break;

    case BLDC_STEP_3:                        /* V+W- U浮空（检测U相） */
        BLDC_SetPWM_CCR(0, pwm_ccr, 0);      /* U相0，V相PWM，W相0 */
        BLDC_SetLowSide_GPIO(0, 0, 1);       /* U相下桥0，V相下桥0，W相下桥1 */
        BLDC_COMP_SelectPhase(BLDC_PHASE_U); /* 切换比较器到U相 */
        break;

    case BLDC_STEP_4:                        /* V+U- W浮空（检测W相） */
        BLDC_SetPWM_CCR(0, pwm_ccr, 0);      /* U相0，V相PWM，W相0 */
        BLDC_SetLowSide_GPIO(1, 0, 0);       /* U相下桥1，V相下桥0，W相下桥0 */
        BLDC_COMP_SelectPhase(BLDC_PHASE_W); /* 切换比较器到W相 */
        break;

    case BLDC_STEP_5:                        /* W+U- V浮空（检测V相） */
        BLDC_SetPWM_CCR(0, 0, pwm_ccr);      /* U相0，V相0，W相PWM */
        BLDC_SetLowSide_GPIO(1, 0, 0);       /* U相下桥1，V相下桥0，W相下桥0 */
        BLDC_COMP_SelectPhase(BLDC_PHASE_V); /* 切换比较器到V相 */
        break;

    case BLDC_STEP_6:                        /* W+V- U浮空（检测U相） */
        BLDC_SetPWM_CCR(0, 0, pwm_ccr);      /* U相0，V相0，W相PWM */
        BLDC_SetLowSide_GPIO(0, 1, 0);       /* U相下桥0，V相下桥1，W相下桥0 */
        BLDC_COMP_SelectPhase(BLDC_PHASE_U); /* 切换比较器到U相 */
        break;
    }
}

/**
 * @brief   设置电机方向
 * @param   direction 方向（1=正向，0=反向）
 * @return  无
 * @note    只能在电机停止时设置方向
 */
void BLDC_Motor_SetDirection(uint8_t direction)
{
    if (s_motor.state.state != BLDC_MOTOR_STOP)
    {
        return;
    }
    s_motorConfig.forward = direction;
}

/**
 * @brief   获取当前电机方向
 * @param   无
 * @return  方向（1=正向，0=反向）
 */
uint8_t BLDC_Motor_GetDirection(void)
{
    return s_motorConfig.forward;
}

/**
 * @brief   获取当前电机电流
 * @param   无
 * @return  电流值（A）
 * @note    通过ADC_BLDC_CH读取电流采样值并换算为电流
 */
static float Motor_GetCurrent(void)
{
    uint32_t raw = adc_get_value(ADC_BLDC_CH);
    float v_sense = (float)raw * (VREF_V / (float)ADC_RESOLUTION);
    float current_a = v_sense / (s_motorConfig.current.shunt_res_ohm * s_motorConfig.current.sense_gain);
    return current_a;
}

/**
 * @brief   获取滤波后的电机电流（移动平均）
 * @param   无
 * @return  电流值（A）
 * @note    使用CURRENT_FILTER_QUEUE_SIZE点移动平均滤波，平滑波动
 */
static float Motor_GetFilteredCurrent(void)
{
    float raw_current = Motor_GetCurrent();

    /* 将原始电流值入队（转换为mA整数，便于队列存储） */
    uint16_t current_ma = (uint16_t)(raw_current * 1000.0f);
    enqueue(&s_motor.current_filter_queue, current_ma);

    /* 获取移动平均值并还原为A */
    uint16_t avg_ma = movingAverage(&s_motor.current_filter_queue);
    return ((float)avg_ma) / 1000.0f;
}

/**
 * @brief   电流过流保护（按档位阈值 + 持续时间判定）
 * @param   无
 * @return  无
 * @note    逻辑：
 *          - 根据当前档位选择过流阈值（GEAR_OVER_CURRENT_MAP）
 *          - 电流持续超过阈值 s_motorConfig.current.oc_trip_duration_ms（2s）→ 立即停机
 *          - 使用 HAL_GetTickDiff 计算持续时间，防止计时溢出问题
 *          - 保留 s_motor.current.current_limit_active 但不再做限流降速，直接停机
 */
void Motor_CurrentLimitHandle(void)
{
    /* 使用滤波后的电流，降低瞬态尖峰误触发概率 */
    float current = Motor_GetFilteredCurrent();

    /* 若已停机或档位无效，直接返回 */
    if (s_motor.state.state == BLDC_MOTOR_STOP)
    {
        s_motor.current.current_high_start_time = 0;
        s_motor.current.current_limit_active = 0;
        return;
    }

    /* 选择当前档位对应转速区间的过流阈值 */
    float oc_limit = 0.0f;
    if (s_motorConfig.speed_profiles != NULL)
    {
        /* 根据档位选择转速区间配置 */
        BLDC_SpeedRange_t speed_range = BLDC_Motor_GetSpeedRange(s_motor.speed.current_gear);
        if (speed_range < 3) /* 区间枚举值范围：0-2 */
        {
            const BLDC_SpeedProfile_t *profile = &s_motorConfig.speed_profiles[speed_range];
            oc_limit = profile->oc_limit_a;
        }
        else
        {
            /* 区间无效，直接返回（不进行过流保护）*/
            return;
        }
    }
    else
    {
        /* 没有配置，直接返回（不进行过流保护）*/
        return;
    }

    /* 检查是否持续超阈值 */
    if (current >= oc_limit)
    {
        if (s_motor.current.current_high_start_time == 0)
        {
            s_motor.current.current_high_start_time = HAL_GetTick();
        }

        if (HAL_GetTickDiff(s_motor.current.current_high_start_time) >= s_motorConfig.current.oc_trip_duration_ms)
        {
            /* 过流保护：直接停机 */
            BLDC_Motor_Stop();
            s_motor.stalled_flag = 1; /* 设置堵转标志 */
            return;
        }
    }
    else
    {
        /* 滤波处理，防止误触发 */
        s_motor.current.current_high_start_time = 0;
        s_motor.current.current_limit_active = 0;
    }
}

/**
 * @brief   获取当前电机运行状态
 * @param   无
 * @return  当前状态
 */
BLDC_Motor_State_t BLDC_Motor_GetState(void)
{
    return s_motor.state.state;
}

/**
 * @brief   获取当前六步换相步骤
 * @param   无
 * @return  当前步骤（1-6）
 */
BLDC_Motor_Step_t BLDC_Motor_GetStep(void)
{
    return s_motor.state.current_step;
}

/**
 * @brief   获取上次换相周期
 * @param   无
 * @return  上次换相周期（单位：采样次数×50us）
 * @note    用于外部模块（如比较器模块）获取换相周期信息
 */
uint32_t BLDC_Motor_GetLastCommPeriod(void)
{
    return s_motor.speed.last_comm_period;
}

/**
 * @brief   获取堵转标志
 * @param   无
 * @return  1=已堵转停机，0=正常
 */
uint8_t BLDC_Motor_IsStalledFlag(void)
{
    return s_motor.stalled_flag;
}

/**
 * @brief   清除堵转标志
 * @param   无
 * @return  无
 * @note    用于用户确认堵转状态后手动清除标志
 */
void BLDC_Motor_ClearStalledFlag(void)
{
    s_motor.stalled_flag = 0;
}

/**
 * @brief   获取转速转换系数
 * @param   无
 * @return  转速转换系数（根据极对数和采样周期计算）
 * @note    用于外部模块（如比较器模块）进行转速计算
 *          转速(RPM) = speed_factor / 换相周期（采样数）
 */
uint32_t BLDC_Motor_GetSpeedFactor(void)
{
    return s_motorConfig.speed_factor;
}

/**
 * @brief   根据档位计算目标转速（RPM）
 * @param   gear 档位（1-100）
 * @return  目标转速（RPM），gear=0时返回0
 * @note    转速计算公式：RPM = 500 + (gear - 1) * 23
 *          - 档位1：500 RPM
 *          - 档位100：500 + 99*23 = 2777 RPM
 */
uint16_t BLDC_Motor_CalcRpmFromGear(BLDC_Gear_t gear)
{
    if (gear == BLDC_GEAR_OFF || gear == 0)
    {
        return 0;
    }
    if (gear >= BLDC_GEAR_MAX)
    {
        gear = BLDC_GEAR_100; /* 限制最大档位 */
    }
    /* 转速计算公式：RPM = 500 + (gear - 1) * 23 */
    return (uint16_t)(500 + (gear - 1) * 23);
}

/**
 * @brief   根据档位选择转速区间
 * @param   gear 档位（1-100）
 * @return  转速区间枚举值
 * @note    - 档位1-44：低速区间（500-1500 RPM）
 *          - 档位45-88：中速区间（1500-2500 RPM）
 *          - 档位89-100：高速区间（2500-2800 RPM）
 */
BLDC_SpeedRange_t BLDC_Motor_GetSpeedRange(BLDC_Gear_t gear)
{
    if (gear == BLDC_GEAR_OFF || gear == 0)
    {
        return BLDC_SPEED_RANGE_LOW; /* 默认返回低速区间 */
    }
    if (gear >= BLDC_GEAR_MAX)
    {
        gear = BLDC_GEAR_100; /* 限制最大档位 */
    }

    /* 根据档位范围选择区间 */
    if (gear <= 44)
    {
        return BLDC_SPEED_RANGE_LOW;  /* 低速区间：1-44档 */
    }
    else if (gear <= 88)
    {
        return BLDC_SPEED_RANGE_MID;  /* 中速区间：45-88档 */
    }
    else
    {
        return BLDC_SPEED_RANGE_HIGH; /* 高速区间：89-100档 */
    }
}

/**
 * @brief   设置启动配置参数
 * @param   config 启动配置结构体指针
 * @return  无
 */

/**
 * @brief   设置电机总配置（包含正反转、启动参数、开环参数等）
 * @param   config 电机配置结构体指针
 * @return  无
 * @note    可以动态修改电机运行方向、启动参数等
 */
void BLDC_Motor_SetConfig(const BLDC_Motor_Config_t *config)
{
    if (config != NULL)
    {
        s_motorConfig.forward = config->forward;
        s_motorConfig.align = config->align;
        s_motorConfig.openloop = config->openloop;
        s_motorConfig.pwm_limit = config->pwm_limit;
        s_motorConfig.protection = config->protection;
        s_motorConfig.current = config->current;
        s_motorConfig.speed_ctrl = config->speed_ctrl;
        s_motorConfig.speed_profiles = config->speed_profiles;
        s_motorConfig.speed_profile_count = config->speed_profile_count;
        s_motorConfig.speed_factor = config->speed_factor;
    }
}

/**
 * @brief   获取当前电机配置
 * @param   config 用于返回配置的结构体指针
 * @return  无
 */
void BLDC_Motor_GetConfig(BLDC_Motor_Config_t *config)
{
    if (config != NULL)
    {
        *config = s_motorConfig;
    }
}

/**
 * @brief   设置电机运行速度（根据档位）
 * @param   gear 目标档位（BLDC_GEAR_1 ~ BLDC_GEAR_100，或BLDC_GEAR_OFF）
 * @return  无
 * @note    - 档位越高，转速越快
 *          - 转速计算公式：RPM = 500 + (gear - 1) * 23
 *          - 可以在电机运行时动态调整
 *          - 此函数只影响闭环运行阶段，不影响启动阶段
 *          - 闭环模式下，目标换相周期决定最终转速，初始PWM会被自动调整
 */
void BLDC_Motor_SetSpeed(BLDC_Gear_t gear)
{
    /* 边界检查 */
    if (gear >= BLDC_GEAR_MAX)
    {
        return;
    }

    /* 记录当前档位（用于过流阈值选择和区间选择） */
    s_motor.speed.current_gear = gear;

    /* 如果是OFF档，停止电机 */
    if (gear == BLDC_GEAR_OFF)
    {
        BLDC_Motor_Stop();
        return;
    }

    /* 动态计算目标转速：RPM = 500 + (gear - 1) * 23 */
    s_motor.speed.target_speed = BLDC_Motor_CalcRpmFromGear(gear);

    /* 安全限制（防止计算错误）*/
    if (s_motor.speed.target_speed > s_motorConfig.speed_ctrl.max_speed_rpm)
    {
        s_motor.speed.target_speed = s_motorConfig.speed_ctrl.max_speed_rpm;
    }
    if (s_motor.speed.target_speed < s_motorConfig.speed_ctrl.min_speed_rpm)
    {
        s_motor.speed.target_speed = s_motorConfig.speed_ctrl.min_speed_rpm;
    }

    /* 更换档位时清除计数器，避免上一档残留影响 */
    s_motor.pwm_count.increase_count_high = 0;
    s_motor.pwm_count.increase_count_mid = 0;
    s_motor.pwm_count.increase_count_low = 0;
    s_motor.pwm_count.decrease_count_high = 0;
    s_motor.pwm_count.decrease_count_mid = 0;
    s_motor.pwm_count.decrease_count_low = 0;

    /* 如果电机正在运行，立即更新PWM */
    if (s_motor.state.state == BLDC_MOTOR_RUNNING)
    {
        Motor_SetPhase(s_motor.state.current_step);
    }
}
