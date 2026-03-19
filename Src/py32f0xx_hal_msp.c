/**
 ******************************************************************************
 * @file    py32f0xx_hal_msp.c
 * @author  MCU Application Team
 * @brief   This file provides code for the MSP Initialization
 *          and de-Initialization codes.
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
DMA_HandleTypeDef HdmaCh1;

/* Private function prototypes -----------------------------------------------*/
/* External functions --------------------------------------------------------*/

/**
 * @brief Initialize global MSP
 */
void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
    /* Set LPTIM interrupt priority */
    HAL_NVIC_SetPriority(LPTIM1_IRQn, 0x01, 0);
    /* Enable LPTIM global interrupt */
    HAL_NVIC_EnableIRQ(LPTIM1_IRQn);
}

/**
 * @brief Initialize ADC-related
 */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_SYSCFG_CLK_ENABLE(); /* Enable SYSCFG clock */
    __HAL_RCC_DMA_CLK_ENABLE();    /* Enable DMA clock */

    ADC_BAT_GPIO_CLK_ENABLE();
    ADC_BLDC_GPIO_CLK_ENABLE();

    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    GPIO_InitStruct.Pin = ADC_BAT_PIN;
    HAL_GPIO_Init(ADC_BAT_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = ADC_BLDC_PIN;
    HAL_GPIO_Init(ADC_BLDC_GPIO_PORT, &GPIO_InitStruct);

    HAL_SYSCFG_DMA_Req(0); /* 将DMA1映射设置为ADC */

    /* ---------------------- */
    /* DMA 配置      */
    /* ---------------------- */
    HdmaCh1.Instance = DMA1_Channel1;                       /* 选择 DMA1 通道 1 */
    HdmaCh1.Init.Direction = DMA_PERIPH_TO_MEMORY;          /* 外设 → 内存 */
    HdmaCh1.Init.PeriphInc = DMA_PINC_DISABLE;              /* 外设地址不变 */
    HdmaCh1.Init.MemInc = DMA_MINC_ENABLE;                  /* 使能内存地址自增 */
    HdmaCh1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD; /* 外设数据宽度 16-bit */
    HdmaCh1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;    /* 内存数据宽度 16-bit */
    HdmaCh1.Init.Mode = DMA_CIRCULAR;                       /* 采用循环模式 */
    HdmaCh1.Init.Priority = DMA_PRIORITY_VERY_HIGH;         /* DMA 通道优先级 */

    HAL_DMA_DeInit(&HdmaCh1);                 /* Deinitialize DMA channel 1 */
    HAL_DMA_Init(&HdmaCh1);                   /* Initialize DMA channel 1 */
    __HAL_LINKDMA(hadc, DMA_Handle, HdmaCh1); /* 连接 DMA 句柄到 ADC */
}

/**
 * @brief Initialize TIM-related MSP
 */
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        /* Enable TIM1 clock */
        __HAL_RCC_TIM1_CLK_ENABLE();

        HAL_NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
    }
}

/**
 * @brief Initialize COMP-related MSP (时钟配置)
 * @note  比较器GPIO配置已移至bldc_init.c的BLDC_COMP_GPIO_Init()函数
 *        这里只负责使能比较器时钟
 */
void HAL_COMP_MspInit(COMP_HandleTypeDef *hcomp)
{
    if (hcomp->Instance == COMP2)
    {
        /* 使能COMP2时钟 */
        __HAL_RCC_COMP2_CLK_ENABLE();

        /* GPIO初始化已在bldc_init.c的BLDC_COMP_GPIO_Init()中完成 */
        HAL_NVIC_SetPriority(ADC_COMP_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(ADC_COMP_IRQn);

        // 过零点输出
        GPIO_InitTypeDef GPIO_InitStruct={0};
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_12;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        GPIO_InitStruct.Alternate = GPIO_AF7_COMP2;

        HAL_GPIO_Init(GPIOA,  &GPIO_InitStruct);
    }
}

/************************ (C) COPYRIGHT Puya *****END OF FILE******************/
