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
#include "py32f002bxx_Start_Kit.h"
#include "py32f0xx_hal.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Exported variables prototypes ---------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/

/**********KEY GPIO*************/
// 按键检测引脚
#define KEY_PIN               GPIO_PIN_3
#define KEY_GPIO_PORT         GPIOA
#define KEY_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE();

// 充电器插入引脚
#define POWER_INSERT_PIN               GPIO_PIN_7
#define POWER_INSERT_GPIO_PORT         GPIOA
#define POWER_INSERT_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE();

// 充满引脚(充满时，为低电平，其他状态为高阻态)
#define FULL_PIN               GPIO_PIN_5
#define FULL_GPIO_PORT         GPIOA
#define FULL_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE();
/*******************************/

/**********LED GPIO*************/
// 绿色指示灯引脚
#define GREEN_LED_PIN               GPIO_PIN_2
#define GREEN_LED_GPIO_PORT         GPIOA
#define GREEN_LED_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE();

// 红色指示灯引脚
#define RED_LED_PIN               GPIO_PIN_6
#define RED_LED_GPIO_PORT         GPIOB
#define RED_LED_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE();
/*******************************/

/**********ENCODER GPIO*************/
// 编码器A引脚
#define ENCODER_A_PIN               GPIO_PIN_1
#define ENCODER_A_GPIO_PORT         GPIOC
#define ENCODER_A_GPIO_CLK_ENABLE() __HAL_RCC_GPIOC_CLK_ENABLE();

// 编码器B引脚
#define ENCODER_B_PIN               GPIO_PIN_7
#define ENCODER_B_GPIO_PORT         GPIOB
#define ENCODER_B_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE();
/*******************************/

/**********ADC GPIO*************/
// 电池电压引脚
#define ADC_BAT_PIN               GPIO_PIN_6
#define ADC_BAT_GPIO_PORT         GPIOA
#define ADC_BAT_CH                ADC_CHANNEL_3
#define ADC_BAT_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE();

/*******************************/

/*********COMM GPIO**********/
// 地址发送引脚
#define ADDR_TX_PIN               GPIO_PIN_0
#define ADDR_TX_GPIO_PORT         GPIOB
#define ADDR_TX_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()
/*******************************/

/**************RF GPIO***************/
#define RF_CSB_PIN               GPIO_PIN_3
#define RF_CSB_GPIO_PORT         GPIOB
#define RF_CSB_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()

#define RF_FCSB_PIN               GPIO_PIN_5
#define RF_FCSB_GPIO_PORT         GPIOB
#define RF_FCSB_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()

#define RF_SCLK_PIN               GPIO_PIN_2
#define RF_SCLK_GPIO_PORT         GPIOB
#define RF_SCLK_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()

#define RF_SDIO_PIN               GPIO_PIN_4
#define RF_SDIO_GPIO_PORT         GPIOB
#define RF_SDIO_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()

#define RF_GPIO1_PIN               GPIO_PIN_1
#define RF_GPIO1_GPIO_PORT         GPIOB
#define RF_GPIO1_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()

/**********************************/


void Error_Handler(uint8_t *file, uint32_t line);
uint32_t HAL_GetTickDiff(uint32_t meiosis);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT Puya *****END OF FILE******************/
