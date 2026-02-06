#include "key_manager.h"

// 按键回调函数
void key_hall_sensor_longPress(void);
void key_hall_sensor_releasePress(void);
void key_remote_rx_longPress(void);
void key_remote_rx_releasePress(void);

/********************按键读取回调开始*****************/
uint8_t key_hall_sensor_read(void)
{
    // 读取霍尔传感器
    if (HAL_GPIO_ReadPin(HALL_SENSOR_GPIO_PORT, HALL_SENSOR_PIN))
    {
        return 1;
    }
    return 0;
}

uint8_t key_remote_rx_read(void)
{
    if (HAL_GPIO_ReadPin(REMOTE_RX_GPIO_PORT, REMOTE_RX_PIN))
    {
        return 1;
    }
    return 0;
}

/******************按键读取回调结束*********************/

// 初始化KEY EVENT
static keyCategory_t keys[KEY_NUM] = {
    [KEY_HALL_SENSOR] = {
        .fsm.keyShield = KEY_ENABLE,
        .fsm.keyDownLevel = Bit_RESET,
        .fsm.eventType = NULL_Event,
        .fsm.keyLongTime = 1000,
        .fsm.keyLastTime = 3000,
        .fsm.keyReadValue = key_hall_sensor_read,
        .func.longPressCb = key_hall_sensor_longPress,
        .func.releasePressCb = key_hall_sensor_releasePress,
    },
    [KEY_REMOTE_RX] = {
        .fsm.keyShield = KEY_ENABLE,
        .fsm.keyDownLevel = Bit_SET,
        .fsm.eventType = NULL_Event,
        .fsm.keyLongTime = 500,
        .fsm.keyLastTime = 3000,
        .fsm.keyReadValue = key_remote_rx_read,
        .func.longPressCb = key_remote_rx_longPress,
        .func.releasePressCb = key_remote_rx_releasePress,
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
    HALL_SENSOR_GPIO_CLK_ENABLE();
    REMOTE_RX_GPIO_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = HALL_SENSOR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;   
    HAL_GPIO_Init(HALL_SENSOR_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = REMOTE_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(REMOTE_RX_GPIO_PORT, &GPIO_InitStruct);

    keyParaInit(keys);
}

void user_key_handle(void)
{
    keyHandle(); // 按键处理函数
}
