/******************************************************************************
 * @file    encoder.h
 * @brief   编码器模块驱动
 * @author  cyWu <1917507415@qq.com>
 * @date    2024-12-19
 * @version V1.0.0
 * @history
 *  - V1.0.0, 2024-12-19, cyWu, 首次发布
 ******************************************************************************/

#ifndef __ENCODER_H
#define __ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/
/**
 * @brief 编码器旋转方向枚举
 */
typedef enum
{
    ENCODER_MODE_NONE = 0,      /**< 空模式，无旋转 */
    ENCODER_MODE_CW,            /**< 顺时针旋转 */
    ENCODER_MODE_CCW            /**< 逆时针旋转 */
} encoder_mode_t;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief  编码器初始化函数
 * @param  None
 * @return None
 * @note   初始化编码器A和B引脚，配置编码器A为外部中断（下降沿触发）
 */
void encoder_init(void);

/**
 * @brief  编码器中断处理函数（供外部中断服务程序调用）
 * @param  None
 * @return None
 * @note   在外部中断服务程序中调用此函数来处理编码器旋转检测
 */
void encoder_irq_handler(void);

/**
 * @brief  获取编码器旋转模式
 * @param  None
 * @return encoder_mode_t 编码器模式（空模式/顺时针/逆时针）
 * @note   获取后会清除当前模式，返回空模式
 */
encoder_mode_t encoder_get_mode(void);

/**
 * @brief  清除编码器模式
 * @param  None
 * @return None
 * @note   将编码器模式重置为空模式
 */
void encoder_clear_mode(void);

#ifdef __cplusplus
}
#endif

#endif /* __ENCODER_H */

/************************ (C) COPYRIGHT Puya *****END OF FILE******************/
