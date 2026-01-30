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
static void APP_OptionConfig(void);

/**
 * @brief  Main program.
 * @retval int
 */
 /**
  * @brief  Delayed by NOPS
  * @param  None
  * @retval None
  */
void APP_DelayNops(uint32_t Nops)			//1ms:	4788
{
  for(uint32_t i=0; i<Nops;i++)
  {
    __NOP();
  }
}

 
int main(void)
{
    HAL_Init();
    APP_SystemClockConfig();
    // 读取OPTION flash 中若PCO非GPIO则写 OPTION flash 配置PCO为GPIO
    if (READ_BIT(FLASH->OPTR, OB_USER_SWD_NRST_MODE) != OB_SWD_PB6_GPIO_PC0)
    {
        APP_OptionConfig();
    }
    HAL_Delay(1000); // 防止SWD被初始化
    app_Init();
    app_lication();
}

/**
 * @brief   Period elapsed callback in non blocking mode
 * @param   htim：TIM handle
 * @retval  None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM14) // 100us定时
    {
        addr_tx_process(); // 地址发送处理
        static uint8_t tick_count = 0;
        tick_count++;
        if (tick_count >= 10)
        {
            tick_count = 0;
            keyCheckProcess(); // 按键处理
        }
    }
}

/**
 * @brief EXTI line detection callback.
 * @param GPIO_Pin: Specifies the pins connected EXTI line
 * @retval None
 * @note   当编码器A引脚(PC1)触发外部中断时，调用编码器中断处理函数
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    /* 检查是否为编码器A引脚触发的中断 */
    if (GPIO_Pin == ENCODER_A_PIN)
    {
        /* 调用编码器中断处理函数 */
        encoder_irq_handler();
    }
}

// 计算时间差
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

static void APP_OptionConfig(void)
{
    FLASH_OBProgramInitTypeDef OBInitCfg = {0};

    HAL_FLASH_Unlock();    /* Unlock Flash */
    HAL_FLASH_OB_Unlock(); /* Unlock Option */

    OBInitCfg.OptionType = OPTIONBYTE_USER;
    OBInitCfg.USERType = OB_USER_BOR_EN | OB_USER_BOR_LEV | OB_USER_IWDG_SW | OB_USER_IWDG_STOP | OB_USER_SWD_NRST_MODE;

    OBInitCfg.USERConfig = OB_BOR_DISABLE | OB_BOR_LEVEL_3p1_3p2 | OB_IWDG_SW | OB_IWDG_STOP_ACTIVE | OB_SWD_PB6_GPIO_PC0;

    /* Option Program */
    HAL_FLASH_OBProgram(&OBInitCfg);

    HAL_FLASH_Lock();    /* Lock Flash */
    HAL_FLASH_OB_Lock(); /* Lock Option */

    /* Option Launch */
    HAL_FLASH_OB_Launch();
}

/**
 * @brief  Clock configuration function.
 * @param  None
 * @retval None
 */
static void APP_SystemClockConfig(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Oscillator configuration */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE; /* Select oscillator HSE, HSI, LSI, LSE */
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;                                                                                              /* Enable HSI */
    RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;                                                                                              /* HSI 1 frequency division */
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_24MHz;                                                                     /* Configure the HSI clock to 24MHz */
    RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS_DISABLE;                                                                                  /* Close HSE bypass */
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;                                                                                              /* Close LSI */
    /*RCC_OscInitStruct.LSICalibrationValue = RCC_LSICALIBRATION_32768Hz;*/
    RCC_OscInitStruct.LSEState = RCC_LSE_OFF; /* Close LSE */
    /*RCC_OscInitStruct.LSEDriver = RCC_LSEDRIVE_MEDIUM;*/
    /* Configure oscillator */
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }

    /* Clock source configuration */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1; /* Choose to configure clock HCLK, SYSCLK, PCLK1 */
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSISYS;                                      /* Select HSISYS as the system clock */
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;                                             /* AHB clock 1 division */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;                                              /* APB clock 1 division */
    /* Configure clock source */
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }
}

/***********************************************************************************************************************
 * Function Name: Error_Handler
 * @brief       错误处理函数
 * @param       *file：文件名，line：行号
 * @return      None
 ***********************************************************************************************************************/
void Error_Handler(uint8_t *file, uint32_t line)
{
    while (1)
    {
        printf("%s %d\r\n", file, line);
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
    /* Users can add their own printing information as needed,
       for example: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* Infinite loop */
    while (1)
    {
    }
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT Puya *****END OF FILE******************/
