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
// 霍尔传感器检测引脚
#define HALL_SENSOR_PIN               GPIO_PIN_1
#define HALL_SENSOR_GPIO_PORT         GPIOB
#define HALL_SENSOR_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE();
/*******************************/

/**********LED GPIO*************/
// 主机充电指示灯引脚
#define HOST_CHARGE_LED_PIN               GPIO_PIN_7
#define HOST_CHARGE_LED_GPIO_PORT         GPIOA
#define HOST_CHARGE_LED_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE();

// 遥控器充电指示灯引脚
#define REMOTE_CHARGE_LED_PIN               GPIO_PIN_6
#define REMOTE_CHARGE_LED_GPIO_PORT         GPIOA
#define REMOTE_CHARGE_LED_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE();
/*******************************/

/*********COMM GPIO**********/
// 主机发送引脚
#define HOST_TX_PIN               GPIO_PIN_0
#define HOST_TX_GPIO_PORT         GPIOB
#define HOST_TX_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()

// 遥控器接收引脚
#define REMOTE_RX_PIN               GPIO_PIN_6
#define REMOTE_RX_GPIO_PORT         GPIOB
#define REMOTE_RX_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()

/*******************************/

void Error_Handler(uint8_t *file, uint32_t line);
uint32_t HAL_GetTickDiff(uint32_t meiosis);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT Puya *****END OF FILE******************/
