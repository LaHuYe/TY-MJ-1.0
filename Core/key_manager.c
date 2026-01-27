#include "key_manager.h"

// 按键回调函数
void key_power_ShortPress(void);
void key_power_longPress(void);
void key_charge_InsertPress(void);
void key_charge_ExtractPress(void);
void key_charge_FullPress(void);
void key_charge_NotFullPress(void);
/********************按键读取回调开始*****************/
uint8_t key_power_read(void)
{
    if (HAL_GPIO_ReadPin(KEY_POWER_GPIO_Port, KEY_POWER_PIN))
    {
        return 1;
    }
    return 0;
}

uint8_t key_charge_read(void)
{
    if (HAL_GPIO_ReadPin(CHARGE_GPIO_PORT, CHARGE_PIN))
    {
        return 1;
    }
    return 0;
}
uint8_t key_chargeFull_read(void)
{
    if (HAL_GPIO_ReadPin(FULL_CHARGE_GPIO_PORT, FULL_CHARGE_PIN))
    {
        return 1;
    }
    return 0;
}
/******************按键读取回调结束*********************/

// 初始化KEY EVENT
static keyCategory_t keys[KEY_NUM] = {
    [KEY_POWER] = {
        // KEY_POWER State Machine Init
        .fsm.keyShield = KEY_ENABLE,
        .fsm.keyDownLevel = Bit_RESET,
        .fsm.eventType = NULL_Event,
        .fsm.keyLongTime = 1500,
        .fsm.keyLastTime = 5000,
        .fsm.keyReadValue = key_power_read,
        // KEY_POWER Callback function init
        .func.ShortPressCb = key_power_ShortPress,
        .func.longPressCb = key_power_longPress,
    },
    [KEY_CHARGE] = {
        // KEY_CHARGE State Machine Init
        .fsm.keyShield = KEY_ENABLE,
        .fsm.keyDownLevel = Bit_SET,
        .fsm.eventType = NULL_Event,
        .fsm.keyLongTime = 1000,
        .fsm.keyLastTime = 2000,
        .fsm.keyReadValue = key_charge_read,
        // KEY_CHARGE Callback function init
        .func.longPressCb = key_charge_InsertPress,
        .func.releasePressCb = key_charge_ExtractPress,
    },
    [KEY_CHARGE_FULL] = {
        // KEY_CHARGE_FULL State Machine Init
        .fsm.keyShield = KEY_ENABLE,
        .fsm.keyDownLevel = Bit_RESET,
        .fsm.eventType = RELEASE_Event,
        .fsm.keyLongTime = 1000,
        .fsm.keyLastTime = 2000,
        .fsm.keyReadValue = key_chargeFull_read,
        // KEY_CHARGE_FULL Callback function init
        .func.longPressCb = key_charge_FullPress,
        .func.releasePressCb = key_charge_NotFullPress,
    },
};

/**
 * Key init function
 * @param none
 * @return none
 */
void user_key_Init(void)
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

    keyParaInit(keys);
}

void user_key_handle(void)
{
    keyHandle(); // 按键处理函数
}
