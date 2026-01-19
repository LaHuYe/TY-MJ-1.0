#include "lptim.h"

LPTIM_HandleTypeDef LPTIMConf = {0};

void lptim_clock_config(void)
{
    RCC_OscInitTypeDef OSCINIT = {0};
    RCC_PeriphCLKInitTypeDef LPTIM_RCC = {0};

    /* LSI clock configuration */
    OSCINIT.OscillatorType = RCC_OSCILLATORTYPE_LSI; /* Set the oscillator type to LSI */
    OSCINIT.LSIState = RCC_LSI_ON;                   /* Enable LSI */
    OSCINIT.PLL.PLLState = RCC_PLL_NONE;
    /* Clock initialization */
    if (HAL_RCC_OscConfig(&OSCINIT) != HAL_OK)
    {
        logAssert();
        Error_Handler(__FILE__, __LINE__);
    }

    /* LPTIM clock configuration */
    LPTIM_RCC.PeriphClockSelection = RCC_PERIPHCLK_LPTIM;   /* Select peripheral clock: LPTIM */
    LPTIM_RCC.LptimClockSelection = RCC_LPTIMCLKSOURCE_LSI; /* Select LPTIM clock source: LSI */
    /* Peripheral clock initialization */
    if (HAL_RCCEx_PeriphCLKConfig(&LPTIM_RCC) != HAL_OK)
    {
        logAssert();
        Error_Handler(__FILE__, __LINE__);
    }

    /* Enable LPTIM clock */
    __HAL_RCC_LPTIM_CLK_ENABLE();
}

void lptim_init(void)
{
    lptim_clock_config();
    /* LPTIM configuration */
    LPTIMConf.Instance = LPTIM;                         /* LPTIM */
    LPTIMConf.Init.Prescaler = LPTIM_PRESCALER_DIV128;  /* Prescaler: 128 */
    LPTIMConf.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE; /* Immediate update mode */
    /* Initialize LPTIM */
    if (HAL_LPTIM_Init(&LPTIMConf) != HAL_OK)
    {
        logAssert();
        Error_Handler(__FILE__, __LINE__);
    }
}

/**
 * @brief   Microsecond delay function
 * @param   nus ：delay time in microseconds
 * @retval  None
 */
static void delay_nops(uint32_t nus)
{
    __IO uint32_t Delay = 1 + nus * (SystemCoreClock / 24U) / 1000000U;
    do
    {
        __NOP();
    } while (Delay--);
}

void lptim_start(void)
{
    /* Enable autoreload interrupt */
    __HAL_LPTIM_ENABLE_IT(&LPTIMConf, LPTIM_IT_ARRM);

    /* Enable LPTIM */
    __HAL_LPTIM_ENABLE(&LPTIMConf);

    /* Load autoreload value */
    __HAL_LPTIM_AUTORELOAD_SET(&LPTIMConf, 128); // 32768/128/128=2=500ms

    /* Delay 120us */
    delay_nops(120);

    /* Start single count mode */
    __HAL_LPTIM_START_SINGLE(&LPTIMConf);
}
