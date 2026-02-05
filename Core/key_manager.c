#include "key_manager.h"

// 按键回调函数
void key_power_shortPress(void);
void key_power_dbclPress(void);
void key_power_longPress(void);
void key_power_insert_longPress(void);
void key_power_insert_releasePress(void);
void key_full_longPress(void);
void key_full_releasePress(void);

/********************按键读取回调开始*****************/
uint8_t key_power_read(void)
{
    if (HAL_GPIO_ReadPin(KEY_GPIO_PORT, KEY_PIN))
    {
        return 1;
    }
    return 0;
}

uint8_t key_power_insert_read(void)
{
    if (HAL_GPIO_ReadPin(POWER_INSERT_GPIO_PORT, POWER_INSERT_PIN))
    {
        return 1;
    }
    return 0;
}

uint8_t key_full_read(void)
{
    if (HAL_GPIO_ReadPin(FULL_GPIO_PORT, FULL_PIN))
    {
        return 1;
    }
    return 0;
}

/******************按键读取回调结束*********************/

// 初始化KEY EVENT
static keyCategory_t keys[KEY_NUM] = {
    [KEY_POWER] = {
        .fsm.keyShield = KEY_ENABLE,
        .fsm.keyDownLevel = Bit_RESET,
        .fsm.eventType = NULL_Event,
        .fsm.keyLongTime = 2000,
        .fsm.keyLastTime = 3000,
        .fsm.keyReadValue = key_power_read,
        .func.ShortPressCb = key_power_shortPress,
        .func.dbclPressCb = key_power_dbclPress,
        .func.longPressCb = key_power_longPress,
    },
    [KEY_POWER_INSERT] = {
        .fsm.keyShield = KEY_ENABLE,
        .fsm.keyDownLevel = Bit_SET,
        .fsm.eventType = NULL_Event,
        .fsm.keyLongTime = 500,
        .fsm.keyLastTime = 3000,
        .fsm.keyReadValue = key_power_insert_read,
        .func.longPressCb = key_power_insert_longPress,
        .func.releasePressCb = key_power_insert_releasePress,
    },
    [KEY_FULL] = {
        .fsm.keyShield = KEY_ENABLE,
        .fsm.keyDownLevel = Bit_RESET,
        .fsm.eventType = NULL_Event,
        .fsm.keyLongTime = 500,
        .fsm.keyLastTime = 3000,
        .fsm.keyReadValue = key_full_read,
        .func.longPressCb = key_full_longPress,
        .func.releasePressCb = key_full_releasePress,
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
    KEY_GPIO_CLK_ENABLE();
    POWER_INSERT_GPIO_CLK_ENABLE();
    FULL_GPIO_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = KEY_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;   
    HAL_GPIO_Init(KEY_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = POWER_INSERT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(POWER_INSERT_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = FULL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(FULL_GPIO_PORT, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI2_3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);

    HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

    keyParaInit(keys);
}

void user_key_handle(void)
{
    keyHandle(); // 按键处理函数
}
