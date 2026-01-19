#include "led_manager.h"

// LED回调函数
uint8_t led_startup_display_handle(LED_Config_t *led_config);
uint8_t led_key_operation_handle(LED_Config_t *led_config);
uint8_t led_charging_handle(LED_Config_t *led_config);
uint8_t led_full_charging_handle(LED_Config_t *led_config);
uint8_t led_low_power_shutdown_handle(LED_Config_t *led_config);
uint8_t led_stuck_shutdown_handle(LED_Config_t *led_config);

/************LED颜色设置*************/
void set_led(GPIO_TypeDef *gpio_port, uint32_t pin, uint8_t state)
{
    HAL_GPIO_WritePin(gpio_port, pin, (GPIO_PinState)state);
}

void set_led_state(uint16_t led_site, uint8_t state)
{
    switch (led_site)
    {
    case USER_LED_RED:
        set_led(USER_RED_GPIO_PORT, USER_RED_PIN, state);
        break;
    case USER_LED_GREEN:
        set_led(USER_GREEN_GPIO_PORT, USER_GREEN_PIN, state);
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
/************************************/

// led初始化
void user_led_init(void)
{
    USER_RED_GPIO_CLK_ENABLE();
    USER_GREEN_GPIO_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = USER_RED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;   /* Push-pull output */
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;           /* Enable pull-up */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; /* GPIO speed */
    HAL_GPIO_Init(USER_RED_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = USER_GREEN_PIN;
    HAL_GPIO_Init(USER_GREEN_GPIO_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(USER_RED_GPIO_PORT, USER_RED_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(USER_GREEN_GPIO_PORT, USER_GREEN_PIN, GPIO_PIN_RESET);

    LED_SetStateInit(set_led_multi); // 设置LED灯状态的回调函数
    SET_TimeGetInit(HAL_GetTick);    // 获取时间戳的回调函数

    // 流水灯模式初始化
    LED_EventTableItem_t led_event_category[LED_EVENT_MAX] = {
        [LED_EVENT_STARTUP_DISPLAY] = {
            // 开机进入待机，绿灯常亮。
            .event = LED_EVENT_STARTUP_DISPLAY,
            .priority = PRIORITY_1,
            .config.ledMask = LED_CHANNEL(USER_LED_GREEN),
            // 常亮1s为一个周期，没有次数，没有循环，即一直常亮下去
            .config.state = LED_LIGHT,
            .config.runNum = 0,                 // 次数为0次数无限大，一直执行
            .ledEventHandler = led_startup_display_handle,
        },
        [LED_EVENT_KEY_OPERATION] = {
            .event = LED_EVENT_KEY_OPERATION,
            .priority = PRIORITY_3,
            .config.ledMask = LED_CHANNEL(USER_LED_GREEN),
            // 常亮1s为一个周期，没有次数，没有循环，即一直常亮下去
            .config.state = LED_BLINKING,
            .config.blinkParams.orDer = 1,
            .config.blinkParams.offTime = 0,
            .config.blinkParams.onTime = 50,
            .config.runNum = 1, // 次数为0次数无限大，一直执行
            .ledEventHandler = led_key_operation_handle,
        },
        [LED_EVENT_CHARGING] = {
            // 充电状态显示，没充满的LED跑马灯显示，充满的LED灯常亮
            .event = LED_EVENT_CHARGING,
            .priority = PRIORITY_1,
            .config.ledMask = LED_CHANNEL(USER_LED_RED),
            // 常亮1s为一个周期，没有次数，没有循环，即一直常亮下去
            .config.state = LED_BLINKING,
            .config.blinkParams.offTime = 500,
            .config.blinkParams.onTime = 500,
            .ledEventHandler = led_charging_handle,
        },
        [LED_EVENT_FULL_CHARGING] = {
            // 充电满状态显示，所有LED灯常亮
            .event = LED_EVENT_FULL_CHARGING,
            .priority = PRIORITY_1,
            .config.ledMask = LED_CHANNEL(USER_LED_GREEN),
            // 常亮1s为一个周期，没有次数，没有循环，即一直常亮下去
            .config.state = LED_LIGHT,
            .config.runNum = 0,             // 次数为0次数无限大，一直执行
            .ledEventHandler = led_full_charging_handle,
        },
        [LED_EVENT_LOW_POWER_SHUTDOWN] = {
            // 低电关机，1颗LED灯快闪5次（频率1Hz）后关机
            .event = LED_EVENT_LOW_POWER_SHUTDOWN,
            .priority = PRIORITY_1,
            .config.ledMask = LED_CHANNEL(USER_LED_RED),
            // 常亮1s为一个周期，没有次数，没有循环，即一直常亮下去
            .config.state = LED_BLINKING,
            .config.blinkParams.offTime = 500,
            .config.blinkParams.onTime = 500,
            .config.runNum = 10,             // 次数为0次数无限大，一直执行
            .ledEventHandler = led_low_power_shutdown_handle,
        },
    };
    LED_EventTableInit(led_event_category);
}

void user_led_handle(void)
{
    LED_EventHandle(); // LED处理函数
}

