/******************************************************************************
 * @file    bldc_motor.h
 * @brief   无刷电机控制模块头文件（六步换相、启动、状态机）
 * @author  cyWu <1917507415@qq.com>
 * @date    2025-11-25
 * @version V1.0.0
 * @history
 *  - V1.0.0, 2025-11-25, cyWu, 首次发布
 ******************************************************************************/
#ifndef __BLDC_MOTOR_H__
#define __BLDC_MOTOR_H__

#include "main.h"
#include "bldc_init.h"
/* ==================== 电机运行状态枚举 ==================== */
typedef enum
{
    BLDC_MOTOR_STOP = 0,    /* 停止状态 */
    BLDC_MOTOR_STARTUP = 1, /* 开环启动阶段（强制定时换相） */
    BLDC_MOTOR_RUNNING = 2  /* 闭环运行阶段（过零点换相） */
} BLDC_Motor_State_t;

/* ==================== 六步换相步骤枚举 ==================== */
/*
 * 相位命名对应：A=U, B=V, C=W
 *
 * ⚠️ 重要说明：过零点检测方向存在两种常见相序
 *
 * 【相序A】- 当前默认配置（bldc_motor.c中BLDC_MOTOR_FORWARD = 1）
 *   STEP_1: A+B- (U+V-) W浮空 上升沿↑
 *   STEP_2: A+C- (U+W-) V浮空 下降沿↓
 *   STEP_3: B+C- (V+W-) U浮空 上升沿↑
 *   STEP_4: B+A- (V+U-) W浮空 下降沿↓
 *   STEP_5: C+A- (W+U-) V浮空 上升沿↑
 *   STEP_6: C+B- (W+V-) U浮空 下降沿↓
 *
 * 【相序B】- 备选配置（bldc_motor.c中BLDC_MOTOR_FORWARD = 0）
 *   STEP_1: A+B- (U+V-) W浮空 下降沿↓
 *   STEP_2: A+C- (U+W-) V浮空 上升沿↑
 *   STEP_3: B+C- (V+W-) U浮空 下降沿↓
 *   STEP_4: B+A- (V+U-) W浮空 上升沿↑
 *   STEP_5: C+A- (W+U-) V浮空 下降沿↓
 *   STEP_6: C+B- (W+V-) U浮空 上升沿↑
 *
 * 如果电机启动后反转，修改 bldc_motor.c 中的 BLDC_MOTOR_FORWARD 宏为 0
 */
typedef enum
{
    BLDC_STEP_1 = 1, /* A+B- (U+V-) W浮空 */
    BLDC_STEP_2 = 2, /* A+C- (U+W-) V浮空 */
    BLDC_STEP_3 = 3, /* B+C- (V+W-) U浮空 */
    BLDC_STEP_4 = 4, /* B+A- (V+U-) W浮空 */
    BLDC_STEP_5 = 5, /* C+A- (W+U-) V浮空 */
    BLDC_STEP_6 = 6  /* C+B- (W+V-) U浮空 */
} BLDC_Motor_Step_t;

/* ==================== 预定位参数 ==================== */
typedef struct
{
    BLDC_Motor_Step_t align_step; /* 预定位相位（步骤） */
    uint16_t align_pwm_ccr;       /* 预定位PWM CCR值 */
    uint8_t align_time_samples;   /* 预定位持续时间（采样周期数） */
} BLDC_Motor_AlignConfig_t;

/* ==================== 开环启动参数 ==================== */
typedef struct
{
    uint32_t comm_delay;       /* 启动换相间隔（采样周期数），例如100×50us=5ms */
    uint16_t openloop_time_ms; /* 开环运行时间（ms），超过此时间强制进入闭环 */
    uint16_t startup_pwm_ccr;  /* 启动阶段PWM CCR计数值（0-1199），更精细的控制 */
    uint16_t pwm_step_ccr;     /* 每次换相增加PWM CCR值 */
    uint16_t pwm_max_ccr;      /* 开环阶段PWM最大CCR值 */
} BLDC_Motor_OpenLoopConfig_t;

/* ==================== PI控制器参数 ==================== */
/**
 * @brief   PI控制器参数结构体
 * @note    用于速度闭环控制，控制PWM占空比
 */
typedef struct
{
    float kp;             /* 比例系数 */
    float ki;             /* 积分系数 */
    float integral_limit; /* 积分限幅（防止积分饱和） */
    float output_limit;   /* 输出限幅（PWM调整量限制，CCR单位） */
} BLDC_PI_Param_t;

/* ==================== 速度环可配置参数（PI控制版本） ==================== */
typedef struct
{
    uint16_t target_rpm;  /* 目标转速（RPM） */
    uint16_t tolerance;   /* 速度误差容差（采样点），在此范围内不调整 */
    float oc_limit_a;     /* 过流阈值（安培），0表示使用默认限值 */
    BLDC_PI_Param_t pi;   /* PI控制器参数 */
} BLDC_SpeedProfile_t;

/* ==================== 转速区间定义（用于100档位分段配置）==================== */
/**
 * @brief   转速区间枚举
 * @note    将100个档位分成3个转速区间，每个区间使用相同的控制参数
 *          - 低速区间：档位1-22
 *          - 中速区间：档位23-70
 *          - 高速区间：档位71-100
 */
typedef enum
{
    BLDC_SPEED_RANGE_LOW = 0,  /* 低速区间：档位1-22 */
    BLDC_SPEED_RANGE_MID = 1,  /* 中速区间：档位23-70 */
    BLDC_SPEED_RANGE_HIGH = 2  /* 高速区间：档位71-100 */
} BLDC_SpeedRange_t;

/* ==================== PWM限制配置结构体 ==================== */
/**
 * @brief   PWM限制配置结构体
 */
typedef struct
{
    uint16_t pwm_min_ccr; /* 最小PWM CCR值 */
    uint16_t pwm_max_ccr; /* 最大PWM CCR值 */
} BLDC_Motor_PwmLimitConfig_t;

/* ==================== 保护参数配置结构体 ==================== */
/**
 * @brief   保护参数配置结构体（强制换相与堵转保护）
 */
typedef struct
{
    uint8_t comm_timeout_factor; /* 超时换相系数：目标换相周期的倍数 */
    uint16_t force_total_limit;  /* 累计强制换相上限→停机 */
} BLDC_Motor_ProtectionConfig_t;

/* ==================== 电流保护配置结构体 ==================== */
/**
 * @brief   电流保护配置结构体
 */
typedef struct
{
    uint16_t oc_trip_duration_ms; /* 过流判定时间：持续超过阈值即停机（ms）*/
    float shunt_res_ohm;          /* 采样电阻（Ω）*/
    float sense_gain;             /* 电流放大倍数（无放大=1.0）*/
} BLDC_Motor_CurrentProtectConfig_t;

/* ==================== 速度控制配置结构体 ==================== */
/**
 * @brief   速度控制配置结构体
 */
typedef struct
{
    uint8_t speed_adjust_period; /* 速度调整周期（ms）*/
    uint16_t min_speed_rpm;      /* 最低转速（RPM）*/
    uint16_t max_speed_rpm;      /* 最高转速（RPM）*/
} BLDC_Motor_SpeedControlConfig_t;

/* ==================== 电机控制总配置结构体 ==================== */
/**
 * @brief   电机控制总配置结构体
 * @note    整合所有电机相关配置，便于统一管理和动态修改
 */
typedef struct
{
    uint8_t forward;                            /* 换相方向：1=正向，0=反向 */
    BLDC_Motor_AlignConfig_t align;             /* 预定位参数 */
    BLDC_Motor_OpenLoopConfig_t openloop;       /* 开环启动参数 */
    BLDC_Motor_PwmLimitConfig_t pwm_limit;      /* PWM限制参数 */
    BLDC_Motor_ProtectionConfig_t protection;   /* 保护参数 */
    BLDC_Motor_CurrentProtectConfig_t current;  /* 电流保护参数 */
    BLDC_Motor_SpeedControlConfig_t speed_ctrl; /* 速度控制参数 */
    const BLDC_SpeedProfile_t *speed_profiles;  /* 速度环档位配置表指针 */
    uint8_t speed_profile_count;                /* 速度环配置表元素个数 */
    uint32_t speed_factor;                      /* 转速转换系数（在初始化时根据极对数和采样周期计算） */
} BLDC_Motor_Config_t;

/* ==================== 函数声明 ==================== */

/**
 * @brief   初始化电机控制模块
 * @param   无
 * @return  无
 * @note    初始化比较器、PWM、状态变量等
 */
void BLDC_Motor_Init(void);

/**
 * @brief   启动电机（开环启动）
 * @param   无
 * @return  无
 * @note    切换到开环启动状态，开始强制换相
 */
void BLDC_Motor_Start(void);

/**
 * @brief   停止电机
 * @param   无
 * @return  无
 * @note    关闭所有PWM输出，复位状态
 */
void BLDC_Motor_Stop(void);

/**
 * @brief   电机控制主循环处理函数
 * @param   无
 * @return  无
 * @note    需要在主循环中周期性调用（例如1ms）
 *          处理启动、换相、状态切换等逻辑
 */
void BLDC_Motor_Handle(void);

/**
 * @brief   获取当前电机运行状态
 * @param   无
 * @return  当前状态（停止/启动/运行）
 */
BLDC_Motor_State_t BLDC_Motor_GetState(void);

/**
 * @brief   获取当前六步换相步骤
 * @param   无
 * @return  当前步骤（1-6）
 */
BLDC_Motor_Step_t BLDC_Motor_GetStep(void);

/**
 * @brief   获取上次换相周期
 * @param   无
 * @return  上次换相周期（单位：采样次数×50us）
 * @note    用于外部模块（如比较器模块）获取换相周期信息
 */
uint32_t BLDC_Motor_GetLastCommPeriod(void);

/**
 * @brief   获取堵转标志
 * @param   无
 * @return  1=已堵转停机，0=正常
 */
uint8_t BLDC_Motor_IsStalledFlag(void);

/**
 * @brief   清除堵转标志
 * @param   无
 * @return  无
 * @note    用于用户确认堵转状态后手动清除标志
 */
void BLDC_Motor_ClearStalledFlag(void);

/**
 * @brief   设置电机总配置（包含正反转、启动参数、开环参数等）
 * @param   config 电机配置结构体指针
 * @return  无
 * @note    可以动态修改电机运行方向、启动参数等
 */
void BLDC_Motor_SetConfig(const BLDC_Motor_Config_t *config);

/**
 * @brief   获取当前电机配置
 * @param   config 用于返回配置的结构体指针
 * @return  无
 */
void BLDC_Motor_GetConfig(BLDC_Motor_Config_t *config);

/**
 * @brief   设置电机运行速度（根据档位）
 * @param   gear 目标档位（BLDC_GEAR_1 ~ BLDC_GEAR_100，或BLDC_GEAR_OFF）
 * @return  无
 * @note    - 档位越高，转速越快
 *          - 转速计算公式：RPM = 500 + (gear - 1) * 23
 *          - 档位1：500 RPM，档位100：2777 RPM
 *          - 可以在电机运行时动态调整
 *          - 此函数只影响闭环运行阶段，不影响启动阶段
 *          - 根据档位自动设置初始PWM和目标换相周期
 */
void BLDC_Motor_SetSpeed(BLDC_Gear_t gear);

/**
 * @brief   设置电机方向
 * @param   direction 方向（1=正向，0=反向）
 * @return  无
 * @note    只能在电机停止时设置方向
 */
void BLDC_Motor_SetDirection(uint8_t direction);

/**
 * @brief   获取当前电机方向
 * @param   无
 * @return  方向（1=正向，0=反向）
 */
uint8_t BLDC_Motor_GetDirection(void);

/**
 * @brief   电流过流保护（按档位阈值 + 持续时间判定）
 * @param   无
 * @return  无
 */
void Motor_CurrentLimitHandle(void);

/**
 * @brief   获取转速转换系数
 * @param   无
 * @return  转速转换系数（根据极对数和采样周期计算）
 * @note    用于外部模块（如比较器模块）进行转速计算
 *          转速(RPM) = speed_factor / 换相周期（采样数）
 */
uint32_t BLDC_Motor_GetSpeedFactor(void);

/**
 * @brief   根据档位计算目标转速（RPM）
 * @param   gear 档位（1-100）
 * @return  目标转速（RPM），gear=0时返回0
 * @note    转速计算公式：RPM = 500 + (gear - 1) * 23
 *          - 档位1：500 RPM
 *          - 档位100：500 + 99*23 = 2777 RPM
 */
uint16_t BLDC_Motor_CalcRpmFromGear(BLDC_Gear_t gear);

/**
 * @brief   根据档位选择转速区间
 * @param   gear 档位（1-100）
 * @return  转速区间枚举值
 * @note    - 档位1-22：低速区间
 *          - 档位23-70：中速区间
 *          - 档位71-100：高速区间
 */
BLDC_SpeedRange_t BLDC_Motor_GetSpeedRange(BLDC_Gear_t gear);

#endif /* __BLDC_MOTOR_H__ */
