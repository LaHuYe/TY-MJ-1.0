#include "power_control.h"
#include "tim.h"

/**
 * @brief 初始化电源GPIO
 * @param none
 * @return none
 */
void power_gpio_init(void)
{
    CHARGE_EN_GPIO_CLK_ENABLE();
    BLDC_POWER_EN_GPIO_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    /* 初始化充电使能引脚 */
    GPIO_InitStruct.Pin = CHARGE_EN_PIN;
    HAL_GPIO_Init(CHARGE_EN_GPIO, &GPIO_InitStruct);

    /* 初始化BLDC供电使能引脚 */
    GPIO_InitStruct.Pin = BLDC_POWER_EN_PIN;
    HAL_GPIO_Init(BLDC_POWER_EN_GPIO, &GPIO_InitStruct);


    HAL_GPIO_WritePin(CHARGE_EN_GPIO, CHARGE_EN_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BLDC_POWER_EN_GPIO, BLDC_POWER_EN_PIN, GPIO_PIN_RESET);

}

/**
 * @brief 设置充电使能
 * @param state 使能状态
 * @return none
 */
void set_charge_enable(bool state)
{
    HAL_GPIO_WritePin(CHARGE_EN_GPIO, CHARGE_EN_PIN, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief 设置BLDC供电使能
 * @param state 使能状态
 * @return none
 */
void set_bldc_power_enable(bool state)
{
    HAL_GPIO_WritePin(BLDC_POWER_EN_GPIO, BLDC_POWER_EN_PIN, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief 获取充电使能状态
 * @return 使能状态
 */
bool get_charge_enable(void)
{
    return HAL_GPIO_ReadPin(CHARGE_EN_GPIO, CHARGE_EN_PIN) == GPIO_PIN_SET ? true : false;
}

/**
 * @brief 获取BLDC供电使能状态
 * @return 使能状态
 */
bool get_bldc_power_enable(void)
{
    return HAL_GPIO_ReadPin(BLDC_POWER_EN_GPIO, BLDC_POWER_EN_PIN) == GPIO_PIN_SET ? true : false;
}

