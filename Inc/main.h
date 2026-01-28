/**
 ******************************************************************************
 * @file    main.h
 * @author  MCU Application Team
 * @brief   Header for main.c file.
 *          This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2023 Puya Semiconductor Co.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by Puya under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "log.h"
#include "py32f030xx_Start_Kit.h"
#include "py32f0xx_hal.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Exported variables prototypes ---------------------------------------------*/
extern uint8_t lptim_flag;
/* Exported functions prototypes ---------------------------------------------*/

/***************************** KEY GPIO *****************************/
// 电源按键引脚（低电平表示电源按键按下）
#define KEY_POWER_PIN               GPIO_PIN_15
#define KEY_POWER_GPIO_Port         GPIOA
#define KEY_POWER_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE();

// 充电引脚（高电平1s表示充电插入）
#define CHARGE_PIN               GPIO_PIN_0
#define CHARGE_GPIO_PORT         GPIOA
#define CHARGE_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE();

// 充电满引脚（低电平表示充电满，高电平表示充电未满，异常则闪烁）
#define FULL_CHARGE_PIN               GPIO_PIN_1
#define FULL_CHARGE_GPIO_PORT         GPIOF
#define FULL_CHARGE_GPIO_CLK_ENABLE() __HAL_RCC_GPIOF_CLK_ENABLE();
/********************************************************************/

/***************************** LED GPIO *****************************/
#define USER_RED_PIN               GPIO_PIN_4
#define USER_RED_GPIO_PORT         GPIOF
#define USER_RED_GPIO_CLK_ENABLE() __HAL_RCC_GPIOF_CLK_ENABLE();

#define USER_GREEN_PIN               GPIO_PIN_8
#define USER_GREEN_GPIO_PORT         GPIOB
#define USER_GREEN_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE();
/********************************************************************/

/***************************** ADC GPIO *****************************/

// 电池检测ADC引脚
#define ADC_BAT_PIN               GPIO_PIN_1
#define ADC_BAT_GPIO_PORT         GPIOA
#define ADC_BAT_CH                ADC_CHANNEL_1
#define ADC_BAT_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE();

// BLDC电流检测ADC引脚
#define ADC_BLDC_PIN               GPIO_PIN_2
#define ADC_BLDC_GPIO_PORT         GPIOA
#define ADC_BLDC_CH                ADC_CHANNEL_2
#define ADC_BLDC_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE();

/********************************************************************/

/***************************** CONTROL GPIO *****************************/
// 充电芯片使能引脚
#define CHARGE_EN_GPIO              GPIOB
#define CHARGE_EN_PIN               GPIO_PIN_7
#define CHARGE_EN_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE();

// BLDC供电使能引脚
#define BLDC_POWER_EN_GPIO              GPIOF
#define BLDC_POWER_EN_PIN               GPIO_PIN_0
#define BLDC_POWER_EN_GPIO_CLK_ENABLE() __HAL_RCC_GPIOF_CLK_ENABLE();

/********************************************************************/

/***************************** BLDC GPIO *****************************/

/*============================上桥臂============================*/
// BLDC U相上桥臂引脚
#define BLDC_U_UP_GPIO_PORT         GPIOA
#define BLDC_U_UP_PIN               GPIO_PIN_8
#define BLDC_U_UP_AF                GPIO_AF2_TIM1
#define BLDC_U_UP_CH                TIM_CHANNEL_1
#define BLDC_U_UP_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE();

// BLDC V相上桥臂引脚
#define BLDC_V_UP_GPIO_PORT         GPIOA
#define BLDC_V_UP_PIN               GPIO_PIN_9
#define BLDC_V_UP_AF                GPIO_AF2_TIM1
#define BLDC_V_UP_CH                TIM_CHANNEL_2
#define BLDC_V_UP_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE();

// BLDC W相上桥臂引脚
#define BLDC_W_UP_GPIO_PORT         GPIOA
#define BLDC_W_UP_PIN               GPIO_PIN_10
#define BLDC_W_UP_AF                GPIO_AF2_TIM1
#define BLDC_W_UP_CH                TIM_CHANNEL_3
#define BLDC_W_UP_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE();

/*============================下桥臂============================*/
// BLDC U相下桥臂引脚
#define BLDC_U_DOWN_GPIO_PORT         GPIOA
#define BLDC_U_DOWN_PIN               GPIO_PIN_7
#define BLDC_U_DOWN_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE();

// BLDC V相下桥臂引脚
#define BLDC_V_DOWN_GPIO_PORT         GPIOB
#define BLDC_V_DOWN_PIN               GPIO_PIN_0
#define BLDC_V_DOWN_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE();

// BLDC W相下桥臂引脚
#define BLDC_W_DOWN_GPIO_PORT         GPIOB
#define BLDC_W_DOWN_PIN               GPIO_PIN_1
#define BLDC_W_DOWN_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE();

/*============================比较器============================*/
// BLDC U相比较器引脚
#define BLDC_U_COMP_GPIO_PORT         GPIOB
#define BLDC_U_COMP_PIN               GPIO_PIN_4
#define BLDC_U_COMP_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE();

// BLDC V相比较器引脚
#define BLDC_V_COMP_GPIO_PORT         GPIOB
#define BLDC_V_COMP_PIN               GPIO_PIN_6
#define BLDC_V_COMP_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE();

// BLDC W相比较器引脚
#define BLDC_W_COMP_GPIO_PORT         GPIOF
#define BLDC_W_COMP_PIN               GPIO_PIN_3
#define BLDC_W_COMP_GPIO_CLK_ENABLE() __HAL_RCC_GPIOF_CLK_ENABLE();

// BLDC 中性点比较器引脚
#define BLDC_COMP_NEUTRAL_GPIO_PORT         GPIOB
#define BLDC_COMP_NEUTRAL_PIN               GPIO_PIN_3
#define BLDC_COMP_NEUTRAL_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE();
/********************************************************************/

typedef enum
{
    STATUS_SUCCESS,
    STATUS_ERROR,
    STATUS_OVERFLOW,
    STATUS_WAIT,
    STATUS_TIMEOUT,
    // 可以继续添加其他状态
} User_StatusTypeDef;

/* Exported functions prototypes ---------------------------------------------*/
/**
 * @brief   错误处理函数
 * @param   *file：文件名，line：行号
 * @return  None
 */
void Error_Handler(uint8_t *file, uint32_t line);

/**
 * @brief   计算时间差
 * @param   meiosis：需要比较的时间戳，用于计算时间差
 * @retval  时间差
 */
uint32_t HAL_GetTickDiff(uint32_t meiosis);
void APP_DelayNops(uint32_t Nops);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT Puya *****END OF FILE******************/
