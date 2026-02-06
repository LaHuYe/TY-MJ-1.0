#include "pwr_stop.h"
#include "application.h"
// #include "lptim.h"

extern uint8_t lptim_flag;
extern ADC_HandleTypeDef AdcHandle;
extern TIM_HandleTypeDef Tim14Handle, Tim1Handle;

static void pwr_stop_Init(void)
{
    // 初始化KEY GPIO
    KEY_GPIO_CLK_ENABLE();
    POWER_INSERT_GPIO_CLK_ENABLE();
    FULL_GPIO_CLK_ENABLE();

    /* 使能编码器A和B的GPIO时钟 */
    ENCODER_A_GPIO_CLK_ENABLE();
    ENCODER_B_GPIO_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = KEY_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(KEY_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = POWER_INSERT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(POWER_INSERT_GPIO_PORT, &GPIO_InitStruct);

    /* 配置编码器A引脚为外部中断模式（边沿触发） */
    GPIO_InitStruct.Pin = ENCODER_A_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING; /* 边沿中断 */
    GPIO_InitStruct.Pull = GPIO_PULLUP;                 /* 上拉输入，确保空闲时为高电平 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;       /* 高速模式，提高响应速度 */
    HAL_GPIO_Init(ENCODER_A_GPIO_PORT, &GPIO_InitStruct);

    /* 配置编码器A的外部中断优先级并使能中断 */
    HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

    HAL_NVIC_SetPriority(EXTI2_3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);

    HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
}

void DeInit_Peripherals(void)
{
    // 睡眠前要复位一下ADC和DMA
    __HAL_RCC_ADC_FORCE_RESET();
    __HAL_RCC_ADC_RELEASE_RESET();
    // __HAL_RCC_ADC_CLK_ENABLE(); /* Enable ADC clock */

    // HAL_ADC_DeInit(&AdcHandle);

    HAL_TIM_Base_Stop_IT(&Tim14Handle);
    __HAL_TIM_DISABLE_IT(&Tim14Handle, TIM_IT_UPDATE);
    HAL_TIM_Base_DeInit(&Tim14Handle);
}

void Set_GPIO_LowPower(void)
{
    // 设置全部IO浮空输入
    GPIO_InitTypeDef GPIO_InitStruct;
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;     /* GPIO mode set to falling edge interrupt */
    GPIO_InitStruct.Pull = GPIO_NOPULL;          /* Pull-up */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; /* High-speed */
    GPIO_InitStruct.Pin = (GPIO_PIN_All);        // 休眠唤醒后初始化I2C不成功，所以在休眠前干脆不浮空I2C引脚
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;     /* GPIO mode set to falling edge interrupt */
    GPIO_InitStruct.Pull = GPIO_NOPULL;          /* Pull-up */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; /* High-speed */
    GPIO_InitStruct.Pin = (GPIO_PIN_All);        // vcc使能脚和充电使能脚不复位，在休眠时也需要进行充电
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;     /* GPIO mode set to falling edge interrupt */
    GPIO_InitStruct.Pull = GPIO_NOPULL;          /* Pull-up */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; /* High-speed */
    GPIO_InitStruct.Pin = (GPIO_PIN_All);        // 加热脚不复位，在休眠时也需要进行加热
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

static void pwr_wakeUp_Init(void)
{
    adc_Init();             // ADC 初始化
    All_Tim_Init();         // 定时器初始化
    user_key_wakeup_Init(); // 按键唤醒初始化
    user_led_init();        // LED初始化
    encoder_init();         // 编码器初始化
    addr_tx_init();         // 地址发送初始化
    // DEBUG_USART_Config(); // 将串口配置成日志口
    RF_Init();
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

// 进入休眠模式
void mcu_enter_sleep(void)
{
    // 休眠配置
    sleep_confing();

    /* Suspend Systick interrupt */
    HAL_SuspendTick();

    /* Entering STOP mode */
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    /* Resume the SysTick interrupt */
    HAL_ResumeTick();

    // 禁用LPTIM
    // __HAL_LPTIM_DISABLE(&LPTIMConf);
    // 唤醒配置
    wakeUp_confing();
}
