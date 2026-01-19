/******************************************************************************
 * @file    bldc_comp.h
 * @brief   无刷电机比较器过零点检测与30°换相模块头文件
 * @author  cyWu
 * @date    2025-11-25
 * @version V2.0.0
 ******************************************************************************/
#ifndef __BLDC_COMP_H__
#define __BLDC_COMP_H__

#include "main.h"

/* ==================== 相位枚举定义 ==================== */
/* 相位命名：U=A相, V=B相, W=C相 */
typedef enum
{
    BLDC_PHASE_U = 0, /* U相（对应A相）*/
    BLDC_PHASE_V = 1, /* V相（对应B相）*/
    BLDC_PHASE_W = 2  /* W相（对应C相）*/
} BLDC_Phase_t;

/* 过零点检测相关变量 */
typedef struct
{
    uint8_t zero_detected;           /* 过零点检测标志：1=检测到，0=未检测到 */
    uint8_t last_level;              /* 上次比较器输出电平 */
    uint32_t delay_30_degree_time;   /* 30°电角度延时时间 */
    uint32_t zero_detect_time;       /* 过零点检测时刻 */
    uint32_t last_commutation_time;  /* 上次换相时刻（采样计数器） */
    uint32_t blank_time_end;         /* 过零检测屏蔽期结束时刻（采样计数器） */
    uint8_t commutation_ready;       /* 换相准备标志：1=可以换相，0=等待中 */
    uint8_t stable_comm_count;       /* 闭环稳定换相计数（用于滤波深度切换） */
} BLDC_ZeroCross_t;

/* ==================== 函数声明 ==================== */

/**
 * @brief   初始化比较器（用于过零点检测）
 * @param   无
 * @return  无
 */
void BLDC_COMP_Init(void);

/**
 * @brief   切换比较器检测相（根据换相步骤选择浮空相）
 * @param   phase 要检测的相（浮空相）
 * @return  无
 */
void BLDC_COMP_SelectPhase(BLDC_Phase_t phase);

/**
 * @brief   检查是否检测到过零点（开环专用，不等30度延时）
 * @param   无
 * @return  1=检测到过零点，0=未检测到
 * @note    用于开环启动阶段，只要检测到过零点就返回真
 */
uint8_t BLDC_COMP_IsZeroDetected(void);

/**
 * @brief   检查是否可以换相（过零点+30°延时）
 * @param   无
 * @return  1=可以换相，0=等待中
 * @note    用于闭环运行阶段，需要在主循环中周期性调用此函数
 */
uint8_t BLDC_COMP_IsCommutationReady(void);

/**
 * @brief   获取当前电机速度（基于换相周期）
 * @param   无
 * @return  速度（RPM），0表示无效
 * @note    使用换相周期计算转速，与速度控制函数使用相同的数据源和公式
 *          转速(RPM) = speed_factor / 换相周期（采样数）
 *          
 *          其中 speed_factor = 60,000,000 / (采样周期us × 6 × 极对数)
 *          会在 bldc_app_init() 中根据 BLDC_POLE_PAIRS 自动计算
 */
uint16_t BLDC_COMP_GetSpeed(void);

/**
 * @brief   过零点采样和滤波（定时器中断调用）
 * @param   无
 * @return  无
 * @note    ⚠️ 此函数需要在定时器中断中调用（当前周期：50us）
 */
void BLDC_COMP_SampleAndFilter(void);

/**
 * @brief   复位过零点检测状态
 * @param   无
 * @return  无
 * @note    切换相位后或启动时调用，清除历史过零点数据
 */
void BLDC_COMP_ResetZeroCross(void);

/**
 * @brief   获取采样计数器（高精度时间基准）
 * @param   无
 * @return  采样计数器值（每50us递增1）
 * @note    可用于高精度时间测量，精度为50us
 */
uint32_t BLDC_COMP_GetSampleCount(void);

/**
 * @brief   更新换相时刻（用于30°延时计算）
 * @param   commutation_time 当前换相时刻（采样计数器值）
 * @return  无
 * @note    ⚠️ 此函数必须在每次换相时调用，用于记录换相时刻
 *          换相时刻到过零点的时间间隔 = 30°电角度
 */
void BLDC_COMP_UpdateCommutationTime(uint32_t commutation_time);

#endif /* __BLDC_COMP_H__ */
