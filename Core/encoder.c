/******************************************************************************
 * @file    encoder.c
 * @brief   编码器模块驱动
 * @author  cyWu <1917507415@qq.com>
 * @date    2024-12-19
 * @version V1.0.0
 * @history
 *  - V1.0.0, 2024-12-19, cyWu, 首次发布
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "encoder.h"
#include "py32f002b_hal_conf.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/**
 * @brief 编码器旋转模式变量
 * @note  用于存储当前检测到的旋转方向，默认为空模式
 */
static volatile encoder_mode_t s_encoder_mode = ENCODER_MODE_NONE;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
 * @brief  编码器初始化函数
 * @param  None
 * @return None
 * @note   初始化编码器A和B引脚，配置编码器A为外部中断（下降沿触发）
 */
void encoder_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能编码器A和B的GPIO时钟 */
    ENCODER_A_GPIO_CLK_ENABLE();
    ENCODER_B_GPIO_CLK_ENABLE();

    /* 配置编码器A引脚为外部中断模式（下降沿触发） */
    GPIO_InitStruct.Pin = ENCODER_A_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING; /* 边沿触发中断 */
    GPIO_InitStruct.Pull = GPIO_PULLUP;                 /* 上拉输入，确保空闲时为高电平 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;       /* 高速模式，提高响应速度 */
    HAL_GPIO_Init(ENCODER_A_GPIO_PORT, &GPIO_InitStruct);

    /* 配置编码器B引脚为输入模式（上拉） */
    GPIO_InitStruct.Pin = ENCODER_B_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;       /* 普通输入模式 */
    GPIO_InitStruct.Pull = GPIO_PULLUP;           /* 上拉输入 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; /* 高速模式 */
    HAL_GPIO_Init(ENCODER_B_GPIO_PORT, &GPIO_InitStruct);

    /* 配置编码器A的外部中断优先级并使能中断 */
    HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

    /* 初始化模式为空模式 */
    s_encoder_mode = ENCODER_MODE_NONE;
}

/**
 * @brief  编码器中断处理函数（供外部中断服务程序调用）
 * @param  None
 * @return None
 * @note   在外部中断服务程序中调用此函数来处理编码器旋转检测
 *         根据编码器A和B的状态判断旋转方向：
 *         - A != B：顺时针旋转
 *         - A == B：逆时针旋转
 */
void encoder_irq_handler(void)
{
    GPIO_PinState pin_a_state;
    GPIO_PinState pin_b_state;

    /* 读取编码器A和B的当前IO状态 */
    pin_a_state = HAL_GPIO_ReadPin(ENCODER_A_GPIO_PORT, ENCODER_A_PIN);
    pin_b_state = HAL_GPIO_ReadPin(ENCODER_B_GPIO_PORT, ENCODER_B_PIN);

    /* 根据编码器A和B的状态判断旋转方向 */
    if (pin_a_state != pin_b_state)
    {
        /* A != B：判断为顺时针旋转 */
        s_encoder_mode = ENCODER_MODE_CW;
    }
    else
    {
        /* A == B：判断为逆时针旋转 */
        s_encoder_mode = ENCODER_MODE_CCW;
    }
}

/**
 * @brief  获取编码器旋转模式
 * @param  None
 * @return encoder_mode_t 编码器模式（空模式/顺时针/逆时针）
 * @note   获取后会清除当前模式，返回空模式
 */
encoder_mode_t encoder_get_mode(void)
{
    encoder_mode_t mode;

    /* 读取当前模式 */
    mode = s_encoder_mode;

    /* 清除模式，重置为空模式 */
    s_encoder_mode = ENCODER_MODE_NONE;

    return mode;
}

/**
 * @brief  清除编码器模式
 * @param  None
 * @return None
 * @note   将编码器模式重置为空模式
 */
void encoder_clear_mode(void)
{
    s_encoder_mode = ENCODER_MODE_NONE;
}
