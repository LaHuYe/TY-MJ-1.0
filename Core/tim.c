#include "tim.h"

// 48000000/1000/48=1KHz

// 20KHZ
//  定时器计数
#define TIM1_PERIOD (1200 - 1)
// 定时器预分频
#define TIM1_PRESCALER (2 - 1)

// 10KHZ
// 定时器计数
#define TIM3_PERIOD (400 - 1)
// 定时器预分频
#define TIM3_PRESCALER (12 - 1)

// 800KHZ（1.25us）
// 定时器计数
#define TIM16_PERIOD (60 - 1)
// 定时器预分频
#define TIM16_PRESCALER (1 - 1)

// 1KHz PWM（用于加热片控制）
// 定时器计数
#define TIM14_PERIOD (48000 - 1)
// 定时器预分频
#define TIM14_PRESCALER (1 - 1)

TIM_HandleTypeDef Tim1Handle, Tim3Handle, Tim16Handle, Tim14Handle;
TIM_OC_InitTypeDef Tim1_sConfig, Tim3_sConfig, Tim16_sConfig, Tim14_sConfig;

/**
 * @brief 初始化TIM的通道1-4
 * @param  TimHandle  TIM句柄
 * @param  sConfig    TIM_OC_InitTypeDef结构体
 * @retval None
 */
static void Tim_Channel_Init(TIM_HandleTypeDef *TimHandle, TIM_OC_InitTypeDef *sConfig)
{
    /* Initializes the TIM PWM channel 1 */
    if (HAL_TIM_PWM_ConfigChannel(TimHandle, sConfig, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }
    /* Initializes the TIM PWM channel 2 */
    if (HAL_TIM_PWM_ConfigChannel(TimHandle, sConfig, TIM_CHANNEL_2) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }
    /* Initializes the TIM PWM channel 3 */
    if (HAL_TIM_PWM_ConfigChannel(TimHandle, sConfig, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }
    /* Initializes the TIM PWM channel 4 */
    if (HAL_TIM_PWM_ConfigChannel(TimHandle, sConfig, TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }
    /* Starts the channel 1 PWM signal generation. */
    if (HAL_TIM_PWM_Start(TimHandle, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }
    /* Starts the channel 2 PWM signal generation. */
    if (HAL_TIM_PWM_Start(TimHandle, TIM_CHANNEL_2) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }
    /* Starts the channel 3 PWM signal generation. */
    if (HAL_TIM_PWM_Start(TimHandle, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }
    /* Starts the channel 4 PWM signal generation. */
    if (HAL_TIM_PWM_Start(TimHandle, TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }
}

/**
 * @brief  TIM1 Config
 * @param  None
 * @retval None
 */
void Tim1_Init(void)
{
    Tim1Handle.Instance = TIM1;                                         /* Select TIM1 */
    Tim1Handle.Init.Period = TIM1_PERIOD;                               /* Auto-reload value */
    Tim1Handle.Init.Prescaler = TIM1_PRESCALER;                         /* Prescaler of 800-1 */
    Tim1Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;             /* No clock division */
    Tim1Handle.Init.CounterMode = TIM_COUNTERMODE_UP;                   /* Up counting */
    Tim1Handle.Init.RepetitionCounter = 1 - 1;                          /* No repetition counting */
    Tim1Handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE; /* Auto-reload register not buffered */
    /* Initialize timer base */
    if (HAL_TIM_PWM_Init(&Tim1Handle) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }
    /* 启动更新中断 */
    if (HAL_TIM_Base_Start_IT(&Tim1Handle) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }

    Tim1_sConfig.OCMode = TIM_OCMODE_PWM1;              /* Configure as PWM1 output */
    Tim1_sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;      /* OC channel output is active high level */
    Tim1_sConfig.OCFastMode = TIM_OCFAST_DISABLE;       /* Fast output mode is disabled */
    Tim1_sConfig.OCNPolarity = TIM_OCNPOLARITY_HIGH;    /* OCN channel output is active high level */
    Tim1_sConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET; /* OC1N output is low in idle state */
    Tim1_sConfig.OCIdleState = TIM_OCIDLESTATE_RESET;   /* OC1 output is low in idle state */
    Tim_Channel_Init(&Tim1Handle, &Tim1_sConfig);
}

/**
 * @brief  TIM3 Config
 * @param  None
 * @retval None
 */
void Tim3_Init(void)
{
    Tim3Handle.Instance = TIM3;                                         /* Select TIM3 */
    Tim3Handle.Init.Period = TIM3_PERIOD;                               /* Auto-reload value */
    Tim3Handle.Init.Prescaler = TIM3_PRESCALER;                         /* Prescaler of 800-1 */
    Tim3Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;             /* No clock division */
    Tim3Handle.Init.CounterMode = TIM_COUNTERMODE_UP;                   /* Up counting */
    Tim3Handle.Init.RepetitionCounter = 1 - 1;                          /* No repetition counting */
    Tim3Handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE; /* Auto-reload register not buffered */
    /* Initialize timer base */
    if (HAL_TIM_PWM_Init(&Tim3Handle) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }

    Tim3_sConfig.OCMode = TIM_OCMODE_PWM1;              /* Configure as PWM1 output */
    Tim3_sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;      /* OC channel output is active high level */
    Tim3_sConfig.OCFastMode = TIM_OCFAST_DISABLE;       /* Fast output mode is disabled */
    Tim3_sConfig.OCNPolarity = TIM_OCNPOLARITY_HIGH;    /* OCN channel output is active high level */
    Tim3_sConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET; /* OC1N output is low in idle state */
    Tim3_sConfig.OCIdleState = TIM_OCIDLESTATE_RESET;   /* OC1 output is low in idle state */
    Tim_Channel_Init(&Tim3Handle, &Tim3_sConfig);
}

/**
 * @brief  TIM14 Config（加热片PWM控制）
 * @param  None
 * @retval None
 */
void Tim14_Init(void)
{

    Tim14Handle.Instance = TIM14;                                         /* Select TIM3 */
    Tim14Handle.Init.Period = TIM14_PERIOD;                               /* Auto-reload value */
    Tim14Handle.Init.Prescaler = TIM14_PRESCALER;                         /* Prescaler of 800-1 */
    Tim14Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;             /* No clock division */
    Tim14Handle.Init.CounterMode = TIM_COUNTERMODE_UP;                   /* Up counting */
    Tim14Handle.Init.RepetitionCounter = 1 - 1;                          /* No repetition counting */
    Tim3Handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE; /* Auto-reload register not buffered */
    /* Initialize timer base */
    if (HAL_TIM_PWM_Init(&Tim14Handle) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }

    Tim14_sConfig.OCMode = TIM_OCMODE_PWM1;              /* Configure as PWM1 output */
    Tim14_sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;      /* OC channel output is active high level */
    Tim14_sConfig.OCFastMode = TIM_OCFAST_DISABLE;       /* Fast output mode is disabled */
    Tim14_sConfig.OCNPolarity = TIM_OCNPOLARITY_HIGH;    /* OCN channel output is active high level */
    Tim14_sConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET; /* OC1N output is low in idle state */
    Tim14_sConfig.OCIdleState = TIM_OCIDLESTATE_RESET;   /* OC1 output is low in idle state */
    Tim_Channel_Init(&Tim14Handle, &Tim14_sConfig);
}

/**
 * @brief  TIM16 Config
 * @param  None
 * @retval None
 */
void Tim16_Init(void)
{
    Tim16Handle.Instance = TIM16;                                        /* Select TIM16 */
    Tim16Handle.Init.Period = TIM16_PERIOD;                              /* Auto-reload value */
    Tim16Handle.Init.Prescaler = TIM16_PRESCALER;                        /* Prescaler of 800-1 */
    Tim16Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;             /* No clock division */
    Tim16Handle.Init.CounterMode = TIM_COUNTERMODE_UP;                   /* Up counting */
    Tim16Handle.Init.RepetitionCounter = 1 - 1;                          /* No repetition counting */
    Tim16Handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE; /* Auto-reload register not buffered */
    /* Initialize timer base */
    if (HAL_TIM_PWM_Init(&Tim16Handle) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }

    /* Enable specified DMA request */
    __HAL_TIM_ENABLE_DMA(&Tim16Handle, TIM_DMA_CC1);

    Tim16_sConfig.OCMode = TIM_OCMODE_PWM1;              /* Configure as PWM1 output */
    Tim16_sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;      /* OC channel output is active high level */
    Tim16_sConfig.OCFastMode = TIM_OCFAST_DISABLE;       /* Fast output mode is disabled */
    Tim16_sConfig.OCNPolarity = TIM_OCNPOLARITY_HIGH;    /* OCN channel output is active high level */
    Tim16_sConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET; /* OC1N output is low in idle state */
    Tim16_sConfig.OCIdleState = TIM_OCIDLESTATE_RESET;   /* OC1 output is low in idle state */
    Tim_Channel_Init(&Tim16Handle, &Tim16_sConfig);
}

// 设置TIM14 PWM占空比
void Tim14_PwmPulseSet(uint32_t Channel, uint32_t Pulse)
{
    uint32_t tim_ccr;

    if (Pulse < 100)
        tim_ccr = Tim14Handle.Init.Period * Pulse * 0.01;
    else
        tim_ccr = (Tim14Handle.Init.Period * Pulse * 0.01) + 1; // 想要占空比为100%时输出高电平,必须使TIMx_CCRx中的比较值大于自动重装载值(TIMx_ARR)

    Tim14_sConfig.Pulse = tim_ccr;
    HAL_TIM_PWM_ConfigChannel(&Tim14Handle, &Tim14_sConfig, Channel);
    HAL_TIM_PWM_Start(&Tim14Handle, Channel);
}

// 设置TIM16 PWM占空比
void Tim16_PwmPulseSet(uint32_t Channel, uint32_t Pulse)
{
    uint32_t tim_ccr;

    if (Pulse < 100)
        tim_ccr = Tim16Handle.Init.Period * Pulse * 0.01;
    else
        tim_ccr = (Tim16Handle.Init.Period * Pulse * 0.01) + 1; // 想要占空比为100%时输出高电平,必须使TIMx_CCRx中的比较值大于自动重装载值(TIMx_ARR)

    Tim16_sConfig.Pulse = tim_ccr;
    HAL_TIM_PWM_ConfigChannel(&Tim16Handle, &Tim16_sConfig, Channel);
    HAL_TIM_PWM_Start(&Tim16Handle, Channel);
}

/**
 * @brief   直接设置TIM1的CCR值（精细控制）
 * @param   Channel TIM通道（TIM_CHANNEL_1/2/3/4）
 * @param   ccr_value CCR计数值（0 ~ TIM1_PERIOD）
 * @return  无
 * @note    用于电机速度闭环控制，实现更精细的PWM调节
 *          - ccr_value = 0：输出0%
 *          - ccr_value = TIM1_PERIOD：输出100%
 *          - 例如：TIM1_PERIOD=1200，ccr_value=600 → 50%占空比
 */
void Tim1_SetCCR_Direct(uint32_t Channel, uint32_t ccr_value)
{
    /* 限制CCR值范围 */
    if (ccr_value > TIM1_PERIOD)
    {
        ccr_value = TIM1_PERIOD;
    }

    /* 根据通道直接写入CCR寄存器 */
    switch (Channel)
    {
    case TIM_CHANNEL_1:
        __HAL_TIM_SET_COMPARE(&Tim1Handle, TIM_CHANNEL_1, ccr_value);
        break;
    case TIM_CHANNEL_2:
        __HAL_TIM_SET_COMPARE(&Tim1Handle, TIM_CHANNEL_2, ccr_value);
        break;
    case TIM_CHANNEL_3:
        __HAL_TIM_SET_COMPARE(&Tim1Handle, TIM_CHANNEL_3, ccr_value);
        break;
    case TIM_CHANNEL_4:
        __HAL_TIM_SET_COMPARE(&Tim1Handle, TIM_CHANNEL_4, ccr_value);
        break;
    }
}

/**
 * @brief   直接设置TIM3的CCR值（精细控制）
 * @param   Channel TIM通道（TIM_CHANNEL_1/2/3/4）
 * @param   ccr_value CCR计数值（0 ~ TIM3_PERIOD）
 * @return  无
 * @note    用于电机速度闭环控制，实现更精细的PWM调节
 *          - ccr_value = 0：输出0%
 *          - ccr_value = TIM3_PERIOD：输出100%
 *          - 例如：TIM3_PERIOD=400，ccr_value=200 → 50%占空比
 */
void Tim3_SetCCR_Direct(uint32_t Channel, uint32_t ccr_value)
{
    /* 限制CCR值范围 */
    if (ccr_value > TIM3_PERIOD)
    {
        ccr_value = TIM3_PERIOD;
    }

    /* 根据通道直接写入CCR寄存器 */
    switch (Channel)
    {
    case TIM_CHANNEL_1:
        __HAL_TIM_SET_COMPARE(&Tim3Handle, TIM_CHANNEL_1, ccr_value);
        break;
    case TIM_CHANNEL_2:
        __HAL_TIM_SET_COMPARE(&Tim3Handle, TIM_CHANNEL_2, ccr_value);
        break;
    case TIM_CHANNEL_3:
        __HAL_TIM_SET_COMPARE(&Tim3Handle, TIM_CHANNEL_3, ccr_value);
        break;
    case TIM_CHANNEL_4:
        __HAL_TIM_SET_COMPARE(&Tim3Handle, TIM_CHANNEL_4, ccr_value);
        break;
    }
}

void All_Tim_Init(void)
{
    Tim1_Init();  // 电机PWM初始化
}
