/**
 ******************************************************************************
 * @file    main.c
 * @author  MCU Application Team
 * @brief   Main program body
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
#include "application.h"

/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private user code ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void APP_SystemClockConfig(void);
uint8_t lptim_flag = 0;

/**
 * @brief  Main program.
 * @retval int
 */
int main(void)
{
    /* Reset of all peripherals, Initializes the Systick. */
    HAL_Init();
    /* Configure the system clock */
    APP_SystemClockConfig();
    HAL_Delay(1000); // 防止SWD被初始化

    app_Init();

    app_lication();
}

/**
 * @brief  System Clock Configuration
 * @param  None
 * @retval None
 */
static void APP_SystemClockConfig(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Oscillator Configuration */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE; /* Select oscillators HSE, HSI, LSI, LSE */
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;                                                                                              /* Enable HSI */
    RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;                                                                                              /* HSI not divided */
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_24MHz;                                                                     /* Configure HSI clock as 8MHz */
    RCC_OscInitStruct.HSEState = RCC_HSE_OFF;                                                                                             /* Disable HSE */
    /*RCC_OscInitStruct.HSEFreq = RCC_HSE_16_32MHz;*/
    RCC_OscInitStruct.LSIState = RCC_LSI_OFF; /* Disable LSI */
    RCC_OscInitStruct.LSEState = RCC_LSE_OFF; /* Disable LSE */
    /*RCC_OscInitStruct.LSEDriver = RCC_LSEDRIVE_MEDIUM;*/
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;         /* Enable PLL */
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI; /* Select PLL source as HSI */
    /* Configure oscillators */
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }

    /* Clock source configuration */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1; /* RCC system clock types */
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;                                      /* SYSCLK source selection as PLL */
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;                                             /* AHB clock not divided */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;                                              /* APB clock not divided */
    /* Configure clock source */
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }
}

/**
 * @brief   Period elapsed callback in non blocking mode
 * @param   htim：TIM handle
 * @retval  None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    static uint8_t time_50us_count = 0;
    if (htim->Instance == TIM1) // 50us定时（20kHz，用于BLDC过零点采样）
    {
        /* ✅ 中断中只做快速采样和滤波，不做复杂逻辑处理 */
        BLDC_COMP_SampleAndFilter();
        bldc_app_handle(); // ✅ BLDC电机处理（必须高频调用，替代中断方式）

        addr_rx_decode(); // 地址接收解码

        time_50us_count++;
        // 1ms定时
        if (time_50us_count % 20 == 0)
        {

            time_50us_count = 0;
            keyCheckProcess();
        }
    }
}
/**
 * @brief   LPTIM autoreload match interrupt callback function
 * @param   None
 * @retval  None
 */
void HAL_LPTIM_AutoReloadMatchCallback(LPTIM_HandleTypeDef *hlptim)
{
    lptim_flag = 1;
}

/**
 * @brief   计算时间差
 * @param   meiosis：需要比较的时间戳，用于计算时间差
 * @retval  时间差
 */
uint32_t HAL_GetTickDiff(uint32_t meiosis)
{
    uint32_t temp = HAL_GetTick();
    if (temp >= meiosis)
    {
        temp = temp - meiosis;
    }
    else
    {
        temp = 0xFFFFFFFFU - meiosis + temp;
    }
    return temp;
}

/**
 * @brief   错误处理函数
 * @param   *file：文件名，line：行号
 * @return  None
 */
void Error_Handler(uint8_t *file, uint32_t line)
{
    while (1)
    {
        appPrintf(LOG_ERROR, "%s %d\r\n", file, line);
    }
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       for example: printf("Wrong parameters value: file %s on line %d\r\n", file, line)  */
    /* Infinite loop */
    while (1)
    {
    }
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT Puya *****END OF FILE******************/
