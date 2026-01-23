#include "led_manager.h"
#include "tim.h"
// LED回调函数
uint8_t led_standby_handler(LED_Config_t *led_config);
uint8_t led_low_power_handler(LED_Config_t *led_config);
uint8_t led_low_warn_handler(LED_Config_t *led_config);
uint8_t led_key_press_handler(LED_Config_t *led_config);
uint8_t led_charging_handler(LED_Config_t *led_config);
uint8_t led_full_handler(LED_Config_t *led_config);

/************LED状态设置*************/
void set_led(GPIO_TypeDef *gpio_port, uint32_t pin, uint8_t state)
{
    HAL_GPIO_WritePin(gpio_port, pin, (GPIO_PinState)state);
}

void set_led_state(uint16_t led_site, uint8_t state)
{
    switch (led_site)
    {
    case USER_LED_RED:
        set_led(RED_LED_GPIO_PORT, RED_LED_PIN, state);
        break;
    case USER_LED_GREEN:
        set_led(GREEN_LED_GPIO_PORT, GREEN_LED_PIN, state);
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
    RED_LED_GPIO_CLK_ENABLE();
    GREEN_LED_GPIO_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = RED_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(RED_LED_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GREEN_LED_PIN;
    HAL_GPIO_Init(GREEN_LED_GPIO_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(RED_LED_GPIO_PORT, RED_LED_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GREEN_LED_GPIO_PORT, GREEN_LED_PIN, GPIO_PIN_RESET);

    LED_SetStateInit(set_led_multi); // 设置LED灯状态的回调函数
    SET_TimeGetInit(HAL_GetTick);    // 获取时间戳的回调函数

    // 流水灯模式初始化
    LED_EventTableItem_t led_event_category[LED_EVENT_MAX] = {
        [LED_EVENT_STANDBY] = {
            // 待机LED常亮。
            .event = LED_EVENT_STANDBY,
            .priority = PRIORITY_1,
            .config.ledMask = LED_CHANNEL(USER_LED_GREEN),
            // 常亮1s为一个周期，没有次数，没有循环，即一直常亮下去
            .config.state = LED_LIGHT,
            .config.runNum = 0,                     // 次数为0次数无限大，一直执行
            .ledEventHandler = led_standby_handler, // 事件处理回调函数
        },
        [LED_EVENT_LOW_POWER] = {
            // 电量低时，待机LED红色常亮。
            .event = LED_EVENT_LOW_POWER,
            .priority = PRIORITY_1,
            .config.ledMask = LED_CHANNEL(USER_LED_RED),
            .config.state = LED_LIGHT,
            .config.runNum = 0,                       // 次数为0次数无限大，一直执行
            .ledEventHandler = led_low_power_handler, // 事件处理回调函数
        },
        [LED_EVENT_LOW_WARN] = {
            // 电量警告时，待机LED绿色闪烁。
            .event = LED_EVENT_LOW_WARN,
            .priority = PRIORITY_1,
            .config.ledMask = LED_CHANNEL(USER_LED_GREEN),
            .config.state = LED_BLINKING,
            .config.blinkParams.onTime = 200,
            .config.blinkParams.offTime = 200,
            .config.runNum = 0,                      // 次数为0次数无限大，一直执行
            .ledEventHandler = led_low_warn_handler, // 事件处理回调函数
        },
        [LED_EVENT_KEY_PRESS] = {
            // 按键事件LED闪烁一次。
            .event = LED_EVENT_KEY_PRESS,
            .priority = PRIORITY_2,
            .config.ledMask = LED_CHANNEL(USER_LED_GREEN) | LED_CHANNEL(USER_LED_RED),
            .config.state = LED_BLINKING,
            .config.blinkParams.orDer = 1,
            .config.blinkParams.offTime = 0,
            .config.blinkParams.onTime = 50,
            .config.runNum = 1,                       // 次数为0次数无限大，一直执行
            .ledEventHandler = led_key_press_handler, // 事件处理回调函数
        },
        [LED_EVENT_CHARGING] = {
            // 充电红色LED闪烁。
            .event = LED_EVENT_CHARGING,
            .priority = PRIORITY_1,
            .config.ledMask = LED_CHANNEL(USER_LED_RED),
            .config.state = LED_BLINKING,
            .config.blinkParams.onTime = 500,
            .config.blinkParams.offTime = 500,
            .config.runNum = 0,                      // 次数为0次数无限大，一直执行
            .ledEventHandler = led_charging_handler, // 事件处理回调函数
        },
        [LED_EVENT_FULL] = {
            // 充满绿色LED闪烁。
            .event = LED_EVENT_FULL,
            .priority = PRIORITY_1,
            .config.ledMask = LED_CHANNEL(USER_LED_GREEN),
            .config.state = LED_LIGHT,
            .config.runNum = 0,                  // 次数为0次数无限大，一直执行
            .ledEventHandler = led_full_handler, // 事件处理回调函数
        },
    };

    LED_EventTableInit(led_event_category);
}

void user_led_handle(void)
{
    LED_EventHandle(); // LED处理函数
}
