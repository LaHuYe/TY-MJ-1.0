#include "led_manager.h"
#include "tim.h"
// LED回调函数
uint8_t led_host_charging_handler(LED_Config_t *led_config);
uint8_t led_remote_charging_handler(LED_Config_t *led_config);
uint8_t led_pairing_handler(LED_Config_t *led_config);

/************LED状态设置*************/
void set_led(GPIO_TypeDef *gpio_port, uint32_t pin, uint8_t state)
{
    HAL_GPIO_WritePin(gpio_port, pin, (GPIO_PinState)state);
}

void set_led_state(uint16_t led_site, uint8_t state)
{
    switch (led_site)
    {
    case HOST_CHARGE_LED:
        set_led(HOST_CHARGE_LED_GPIO_PORT, HOST_CHARGE_LED_PIN, state);
        break;
    case REMOTE_CHARGE_LED:
        set_led(REMOTE_CHARGE_LED_GPIO_PORT, REMOTE_CHARGE_LED_PIN, state);
        break;
    default:
        break;
    }
}
// LED状态设置回调函数
void set_led_multi(uint16_t led, uint8_t state)
{
    for (size_t led_site = 0; led_site < USER_LED_MAX; led_site++)
    {
        if (led & LED_CHANNEL(led_site))
        {
            set_led_state(led_site, state);
        }
    }
}
/***********************************/

// led初始化
void user_led_init(void)
{
    HOST_CHARGE_LED_GPIO_CLK_ENABLE();
    REMOTE_CHARGE_LED_GPIO_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = HOST_CHARGE_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(HOST_CHARGE_LED_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = REMOTE_CHARGE_LED_PIN;
    HAL_GPIO_Init(REMOTE_CHARGE_LED_GPIO_PORT, &GPIO_InitStruct);

    LED_SetStateInit(set_led_multi); // 设置LED灯状态的回调函数
    SET_TimeGetInit(HAL_GetTick);    // 获取时间戳的回调函数

    // 流水灯模式初始化
    LED_EventTableItem_t led_event_category[LED_EVENT_MAX] = {
        [LED_EVENT_HOST_CHARGING] = {
            // 开机LED常亮。
            .event = LED_EVENT_HOST_CHARGING,
            .priority = PRIORITY_1,
            .config.ledMask = LED_CHANNEL(HOST_CHARGE_LED),
            // 常亮1s为一个周期，没有次数，没有循环，即一直常亮下去
            .config.state = LED_LIGHT,
            .config.runNum = 0,                     // 次数为0次数无限大，一直执行
            .ledEventHandler = led_host_charging_handler, // 事件处理回调函数
        },
        [LED_EVENT_REMOTE_CHARGING] = {
            // 遥控器充电LED常亮。
            .event = LED_EVENT_REMOTE_CHARGING,
            .priority = PRIORITY_3,
            .config.ledMask = LED_CHANNEL(REMOTE_CHARGE_LED),
            .config.state = LED_LIGHT,
            .config.runNum = 0,                     // 次数为0次数无限大，一直执行
            .ledEventHandler = led_remote_charging_handler, // 事件处理回调函数
        },
        [LED_EVENT_PAIRING] = {
            // 配对LED常亮。
            .event = LED_EVENT_PAIRING,
            .priority = PRIORITY_2,
            .config.ledMask = LED_CHANNEL(HOST_CHARGE_LED),
            .config.state = LED_BLINKING,
            .config.blinkParams.onTime = 100,
            .config.blinkParams.offTime = 100,
            .config.runNum = 5,                     // 次数为0次数无限大，一直执行
            .ledEventHandler = led_pairing_handler, // 事件处理回调函数
        },
    };

    LED_EventTableInit(led_event_category);
}

void user_led_handle(void)
{
    LED_EventHandle(); // LED处理函数
}
