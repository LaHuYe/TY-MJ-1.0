/******************************************************************************
 * @file    bldc_app.h
 * @brief   BLDC电机应用层（档位控制、转速管理）
 * @author  cyWu <1917507415@qq.com>
 * @date    2025-11-25
 * @version V1.0.0
 * @history
 *  - V1.0.0, 2025-11-25, cyWu, 首次发布
 ******************************************************************************/

#ifndef __BLDC_APP_H
#define __BLDC_APP_H

#include "main.h"
#include <stdbool.h>
#include "bldc_motor.h"  /* 包含BLDC电机底层控制模块，提供BLDC_Gear_t定义 */

/* ==================== 初始化接口 ==================== */
/**
 * @brief   初始化BLDC应用层
 * @param   无
 * @return  无
 * @note    必须在系统初始化时调用，会自动调用BLDC_Motor_Init()
 */
void bldc_app_init(void);

/* ==================== 档位控制接口 ==================== */
/**
 * @brief   设置BLDC电机档位
 * @param   gear 目标档位（BLDC_GEAR_OFF ~ BLDC_GEAR_100）
 * @return  无
 * @note    - BLDC_GEAR_OFF (0)：停止电机
 *          - BLDC_GEAR_1 ~ BLDC_GEAR_100 (1-100)：启动电机并设置对应转速
 *          - 转速计算公式：RPM = 500 + (gear - 1) * 23
 *          - 档位1：500 RPM，档位100：2777 RPM
 *          - 档位切换是平滑的，不会造成电机抖动
 */
void bldc_set_gear(BLDC_Gear_t gear);

/**
 * @brief   获取当前BLDC电机档位
 * @param   无
 * @return  当前档位
 */
BLDC_Gear_t bldc_get_gear(void);

/**
 * @brief   增加档位（+1档）
 * @param   无
 * @return  无
 * @note    如果已经是最高档（BLDC_GEAR_100），则保持不变
 */
void bldc_gear_increase(void);

/**
 * @brief   减少档位（-1档）
 * @param   无
 * @return  无
 * @note    如果已经是OFF档，则保持不变
 */
void bldc_gear_decrease(void);

/* ==================== 方向控制接口 ==================== */
/**
 * @brief   设置BLDC电机方向
 * @param   direction 方向（1=正向，0=反向）
 * @return  无
 */
void bldc_set_direction(uint8_t direction);

/* ==================== 状态查询接口 ==================== */
/**
 * @brief   查询BLDC电机是否正在运行
 * @param   无
 * @return  true-电机运行中，false-电机已停止
 */
bool bldc_is_running(void);

/**
 * @brief   获取当前电机转速（RPM）
 * @param   无
 * @return  转速（RPM），0表示停止或无效
 */
uint16_t bldc_get_speed_rpm(void);

/**
 * @brief   获取堵转状态
 * @param   无
 * @return  true=已堵转停机，false=正常
 */
bool bldc_is_stalled(void);

/**
 * @brief   清除堵转标志
 * @param   无
 * @return  无
 * @note    用于LED显示后手动清除标志
 */
void bldc_clear_stalled_flag(void);

/**
 * @brief   强制停止BLDC电机
 * @param   无
 * @return  无
 * @note    立即停止电机，档位自动设置为BLDC_GEAR_OFF
 */
void bldc_force_stop(void);

/**
 * @brief   BLDC应用层处理函数
 * @param   无
 * @return  无
 * @note    - 需要在主循环中周期性调用（建议1ms调用一次）
 *          - 负责调用底层BLDC_Motor_Handle()
 */
void bldc_app_handle(void);

#endif /* __BLDC_APP_H */

