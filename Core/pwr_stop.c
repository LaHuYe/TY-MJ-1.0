#include "pwr_stop.h"
#include "application.h"
#include "cmt2300a.h"
#include "cmt_spi3.h"
#include "radio.h"

extern ADC_HandleTypeDef AdcHandle;
extern DMA_HandleTypeDef HdmaCh1;
extern TIM_HandleTypeDef Tim1Handle, Tim16Handle, Tim3Handle, Tim14Handle;

static void pwr_stop_Init(void)
{
    // 初始化KEY GPIO
    KEY_POWER_GPIO_CLK_ENABLE();
    CHARGE_GPIO_CLK_ENABLE();
    FULL_CHARGE_GPIO_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 电源按键输入
    GPIO_InitStruct.Pin = KEY_POWER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(KEY_POWER_GPIO_Port, &GPIO_InitStruct);

    // 充电按键输入
    GPIO_InitStruct.Pin = CHARGE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(CHARGE_GPIO_PORT, &GPIO_InitStruct);

    // 充电满按键输入
    GPIO_InitStruct.Pin = FULL_CHARGE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(FULL_CHARGE_GPIO_PORT, &GPIO_InitStruct);

    HAL_NVIC_EnableIRQ(EXTI0_1_IRQn); /* Enable EXTI interrupt */
    HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);

    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn); /* Enable EXTI interrupt */
    HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
}

void DeInit_Peripherals(void)
{
    // 睡眠前要复位一下ADC和DMA
    __HAL_RCC_ADC_FORCE_RESET();
    __HAL_RCC_ADC_RELEASE_RESET();
    __HAL_RCC_ADC_CLK_ENABLE(); /* Enable ADC clock */

    __HAL_RCC_DMA_FORCE_RESET();
    __HAL_RCC_DMA_RELEASE_RESET();

    HAL_ADC_DeInit(&AdcHandle);
    HAL_ADC_Stop_DMA(&AdcHandle);
    HAL_DMA_DeInit(&HdmaCh1);

    HAL_TIM_Base_Stop_IT(&Tim1Handle);
    __HAL_TIM_DISABLE_IT(&Tim1Handle, TIM_IT_UPDATE);
    HAL_TIM_Base_DeInit(&Tim1Handle);

    HAL_TIM_Base_Stop_IT(&Tim3Handle);
    __HAL_TIM_DISABLE_IT(&Tim3Handle, TIM_IT_UPDATE);
    HAL_TIM_PWM_DeInit(&Tim3Handle);

    HAL_TIM_Base_Stop_IT(&Tim14Handle);
    __HAL_TIM_DISABLE_IT(&Tim14Handle, TIM_IT_UPDATE);
    HAL_TIM_Base_DeInit(&Tim14Handle);

    HAL_TIM_Base_Stop_IT(&Tim16Handle);
    __HAL_TIM_DISABLE_IT(&Tim16Handle, TIM_IT_UPDATE);
    HAL_TIM_Base_DeInit(&Tim16Handle);

    __HAL_RCC_TIM1_CLK_DISABLE();  /* Disable TIM1 clock */
    __HAL_RCC_TIM3_CLK_DISABLE();  /* Disable TIM1 clock */
    __HAL_RCC_TIM16_CLK_DISABLE(); /* Disable TIM16 clock */
    __HAL_RCC_ADC_FORCE_RESET();
    __HAL_RCC_ADC_CLK_DISABLE(); /* Disable ADC clock */

    __HAL_RCC_FLASH_CLK_DISABLE(); /* Disable FLASH clock */
    __HAL_RCC_SRAM_CLK_DISABLE();
    __HAL_RCC_CRC_CLK_DISABLE();
    __HAL_RCC_LPTIM_CLK_DISABLE();
    __HAL_RCC_COMP2_CLK_DISABLE();
    // __HAL_RCC_COMP2_FORCE_RESET();

    __HAL_RCC_USART1_CLK_DISABLE();

    __HAL_RCC_GPIOA_CLK_DISABLE();
    __HAL_RCC_GPIOB_CLK_DISABLE();
    __HAL_RCC_GPIOF_CLK_DISABLE();

    HAL_NVIC_DisableIRQ(USART1_IRQn);
    HAL_NVIC_DisableIRQ(EXTI0_1_IRQn);
    HAL_NVIC_DisableIRQ(EXTI4_15_IRQn);
}

void Set_GPIO_LowPower(void)
{
    // 设置全部IO浮空输入
    GPIO_InitTypeDef GPIO_InitStruct;
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;     /* GPIO mode set to falling edge interrupt */
    GPIO_InitStruct.Pull = GPIO_NOPULL;          /* Pull-up */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; /* High-speed */
    GPIO_InitStruct.Pin = GPIO_PIN_All;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;     /* GPIO mode set to falling edge interrupt */
    GPIO_InitStruct.Pull = GPIO_NOPULL;          /* Pull-up */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; /* High-speed */
    GPIO_InitStruct.Pin = GPIO_PIN_All;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    __HAL_RCC_GPIOF_CLK_ENABLE();
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;     /* GPIO mode set to falling edge interrupt */
    GPIO_InitStruct.Pull = GPIO_NOPULL;          /* Pull-up */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; /* High-speed */
    GPIO_InitStruct.Pin = GPIO_PIN_All;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
}

static void pwr_wakeUp_Init(void)
{
    DEBUG_USART_Config(); // 将串口配置成日志口
    reset_decode_parameters(); // 重置解码参数
    // 外设初始化
    adc_Init();        // 初始化ADC
    All_Tim_Init();    // 初始化定时器（TIM1, TIM3, TIM16）
    user_led_init();   // 初始化LED
    power_gpio_init(); // 初始化电源控制
    cmt_spi3_init();   // 433IO口初始化
    RF_Init();         // 433配置与寄存器初始化
}

static void sleep_confing(void)
{
    DeInit_Peripherals(); // 关外设关中断
    Set_GPIO_LowPower();  // 所有GPIO浮空输入
    pwr_stop_Init();      // 按键中断唤醒初始化
    // lptim_init();         // lptim初始化
    // lptim_start();        // Lptim开始
}

static void wakeUp_confing(void)
{
    pwr_wakeUp_Init(); // GPIO初始化
}

void LowPower_SystemClockConfig(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Initialize CPU, AHB, and APB bus clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1; /* RCC system clock types */
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;                                         /* SYSCLK source is HSI */
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;                                             /* AHB clock not divided */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;                                              /* APB clock not divided */

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) /* Initialize RCC system clock */
    {
        Error_Handler(__FILE__, __LINE__);
    }

    /* Oscillator Configuration */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE; /* Select oscillators HSE, HSI, LSI, LSE */
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;                                                                                              /* Enable HSI */
    RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;                                                                                              /* HSI not divided */
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_8MHz;                                                                      /* Configure HSI clock as 8MHz */
    RCC_OscInitStruct.HSEState = RCC_HSE_OFF;                                                                                             /* Disable HSE */
    /*RCC_OscInitStruct.HSEFreq = RCC_HSE_16_32MHz;*/
    RCC_OscInitStruct.LSIState = RCC_LSI_OFF; /* Disable LSI */
    RCC_OscInitStruct.LSEState = RCC_LSE_OFF; /* Disable LSE */
    /*RCC_OscInitStruct.LSEDriver = RCC_LSEDRIVE_MEDIUM;*/
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF; /* Enable PLL */
    // RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI; /* Select PLL source as HSI */
    /* Configure oscillators */
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }
}

void Norm_SystemClockConfig(void)
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

// 进入休眠模式
void mcu_enter_sleep(void)
{
    PWR_StopModeConfigTypeDef PwrStopModeConf = {0};
    // 关闭PLL倍频
    LowPower_SystemClockConfig();
    HAL_Delay(20);
    // 休眠配置
    sleep_confing();
    /* Suspend Systick interrupt */
    HAL_SuspendTick();
    /* VCORE = 1.0V  when enter stop mode */
    PwrStopModeConf.LPVoltSelection = PWR_STOPMOD_LPR_VOLT_SCALE2;
    PwrStopModeConf.FlashDelay = PWR_WAKEUP_FLASH_DELAY_5US;
    PwrStopModeConf.WakeUpHsiEnableTime = PWR_WAKEUP_HSIEN_AFTER_MR;
    PwrStopModeConf.RegulatorSwitchDelay = PWR_WAKEUP_LPR_TO_MR_DELAY_2US;
    PwrStopModeConf.SramRetentionVolt = PWR_SRAM_RETENTION_VOLT_VOS;
    HAL_PWR_ConfigStopMode(&PwrStopModeConf);
    /* Enter STOP mode */
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    // 打开PLL倍频
    Norm_SystemClockConfig();
    /* Resume the SysTick interrupt */
    HAL_ResumeTick();
    // 禁用LPTIM
    // __HAL_LPTIM_DISABLE(&LPTIMConf);
    // HAL_Delay(20);
    // 唤醒配置
    wakeUp_confing();
}
