#include "tim.h"
// 24000000/1000/24=1KHz


// 定时器计数
#define TIM1_PERIOD (1000 - 1)
// 定时器预分频
#define TIM1_PRESCALER (24 - 1)
// 定时器计数0
#define TIM14_PERIOD (100 - 1)
// 定时器预分频
#define TIM14_PRESCALER (12 - 1)



TIM_HandleTypeDef Tim1Handle,Tim14Handle;
TIM_OC_InitTypeDef Tim1_sConfig, Tim14_sConfig;
/**
 * @brief  TIM1 Config
 * @param  None
 * @retval None
 */
void Tim1_Init(void)
{
    Tim1Handle.Instance = TIM1;                                        /* Select TIM16 */
    Tim1Handle.Init.Period = TIM1_PERIOD;                              /* Auto-reload value */
    Tim1Handle.Init.Prescaler = TIM1_PRESCALER;                        /* Prescaler of 1000-1 */
    Tim1Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;             /* Clock not divided */
    Tim1Handle.Init.CounterMode = TIM_COUNTERMODE_UP;                   /* Up counting mode */
    Tim1Handle.Init.RepetitionCounter = 1 - 1;                          /* No repetition */
    Tim1Handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE; /* Auto-reload register not buffered */
    if (HAL_TIM_Base_Init(&Tim1Handle) != HAL_OK)                       /* Initialize TIM1 */
    {
        Error_Handler(__FILE__, __LINE__);
    }

    Tim1_sConfig.OCMode = TIM_OCMODE_PWM1;              /* Configure as PWM1 output */
    Tim1_sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;      /* OC channel output is active high level */
    Tim1_sConfig.OCFastMode = TIM_OCFAST_DISABLE;       /* Fast output mode is disabled */
    Tim1_sConfig.OCNPolarity = TIM_OCNPOLARITY_HIGH;    /* OCN channel output is active high level */
    Tim1_sConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET; /* OC1N output is low in idle state */
    Tim1_sConfig.OCIdleState = TIM_OCIDLESTATE_RESET;   /* OC1 output is low in idle state */
}

/**
 * @brief  TIM14 Config
 * @param  None
 * @retval None
 */
void Tim14_Init(void)
{
    Tim14Handle.Instance = TIM14;                                        /* Select TIM16 */
    Tim14Handle.Init.Period = TIM14_PERIOD;                              /* Auto-reload value */
    Tim14Handle.Init.Prescaler = TIM14_PRESCALER;                        /* Prescaler of 1000-1 */
    Tim14Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;             /* Clock not divided */
    Tim14Handle.Init.CounterMode = TIM_COUNTERMODE_UP;                   /* Up counting mode */
    Tim14Handle.Init.RepetitionCounter = 1 - 1;                          /* No repetition */
    Tim14Handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE; /* Auto-reload register not buffered */
    if (HAL_TIM_Base_Init(&Tim14Handle) != HAL_OK)                       /* Initialize TIM16 */
    {
        Error_Handler(__FILE__, __LINE__);
    }

    /* 启动更新中断 */
    if (HAL_TIM_Base_Start_IT(&Tim14Handle) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }
}

// 设置TIM1 PWM占空比
void Tim1_PwmPulseSet(uint32_t Channel, uint32_t Pulse)
{
    uint32_t tim_ccr;

    if (Pulse < 100)
        tim_ccr = TIM1_PERIOD * Pulse * 0.01;
    else
        tim_ccr = (TIM1_PERIOD * Pulse * 0.01) + 1; // 想要占空比为100%时输出高电平,必须使TIMx_CCRx中的比较值大于自动重装载值(TIMx_ARR)

    Tim1_sConfig.Pulse = tim_ccr;
    HAL_TIM_PWM_ConfigChannel(&Tim1Handle, &Tim1_sConfig, Channel);
    HAL_TIM_PWM_Start(&Tim1Handle, Channel);
}

// 设置TIM14 PWM占空比
void Tim14_PwmPulseSet(uint32_t Channel, uint32_t Pulse)
{
    uint32_t tim_ccr;

    if (Pulse < 100)
        tim_ccr = TIM14_PERIOD * Pulse * 0.01;
    else
        tim_ccr = (TIM14_PERIOD * Pulse * 0.01) + 1; // 想要占空比为100%时输出高电平,必须使TIMx_CCRx中的比较值大于自动重装载值(TIMx_ARR)

    Tim14_sConfig.Pulse = tim_ccr;
    HAL_TIM_PWM_ConfigChannel(&Tim14Handle, &Tim14_sConfig, Channel);
    HAL_TIM_PWM_Start(&Tim14Handle, Channel);
}

void All_Tim_Init(void)
{
    Tim14_Init();
}
