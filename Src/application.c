#include "application.h"

/*****************初始化层*******************/

static bool g_hall_long_press_flag = false;   // 霍尔长按标志位（在释放时清除）
static bool g_remote_addr_ready_flag = false; // 已接收到遥控器地址标志位（发送完成后清除）
static uint8_t g_remote_addr_cache[2] = {0};  // 缓存的遥控器地址

/****************************应用层*********************************/

/******************** KEY 应用事件开始*****************/
/**
 * @brief 霍尔传感器长按事件
 * @param none
 * @return none
 */
void key_hall_sensor_longPress(void)
{
    Log("key_hall_sensor_longPress\r\n");
    g_hall_long_press_flag = true;
    LED_EventAdd(LED_EVENT_HOST_CHARGING);
}

/**
 * @brief 霍尔传感器释放事件
 * @param none
 * @return none
 */
void key_hall_sensor_releasePress(void)
{
    Log("key_hall_sensor_releasePress\r\n");
    g_hall_long_press_flag = false; // 释放时清除霍尔长按标志
    LED_EventDelete(LED_EVENT_HOST_CHARGING);
}

/**
 * @brief 遥控器接收长按事件
 * @param none
 * @return none
 */
void key_remote_rx_longPress(void)
{
    Log("key_remote_rx_longPress\r\n");
    LED_EventAdd(LED_EVENT_REMOTE_CHARGING);
}

/**
 * @brief 遥控器接收释放事件
 * @param none
 * @return none
 */
void key_remote_rx_releasePress(void)
{
    Log("key_remote_rx_releasePress\r\n");
    LED_EventDelete(LED_EVENT_REMOTE_CHARGING);
}

/****************** KEY 应用事件结束*********************/

/******************** LED 应用事件开始*****************/
uint8_t led_host_charging_handler(LED_Config_t *led_config)
{
    LED_Update(led_config);
    return 0;
}
uint8_t led_remote_charging_handler(LED_Config_t *led_config)
{
    LED_Update(led_config);
    return 0;
}
uint8_t led_pairing_handler(LED_Config_t *led_config)
{
    if (LED_Update(led_config) == 0)
    {
        LED_EventDelete(LED_EVENT_PAIRING);
        return 0;
    }
    return 1;
}

/******************** LED 应用事件结束*****************/

/***********************用户应用层开始**********************/

/**
 * @brief 获取遥控器地址码
 * @param none
 * @return none
 */
void get_remote_address_code(void)
{

    if (LED_IsEventExist(LED_EVENT_REMOTE_CHARGING) && g_remote_addr_ready_flag)
    {
        LED_EventAdd(LED_EVENT_PAIRING);
    }

    if (addr_rx_get_received_address(g_remote_addr_cache))
    {
        Log("remote_address_code:0x%02X%02X\r\n", g_remote_addr_cache[0], g_remote_addr_cache[1]);
        LED_EventAdd(LED_EVENT_REMOTE_CHARGING);
        
        // 设置标志，等待霍尔长按后发送
        g_remote_addr_ready_flag = true;
    }
    else
    {
        Log("remote_address_code:NULL\r\n");
    }
}

/**
 * @brief 同步条件检查：霍尔长按且已收到地址时发送
 */
static void try_send_remote_addr_if_ready(void)
{
    if (g_hall_long_press_flag && g_remote_addr_ready_flag)
    {
        if (addr_tx_enable(g_remote_addr_cache))
        {
            // 发送触发成功后，清除接收标志；霍尔标志由释放事件清除
            g_remote_addr_ready_flag = false;
            Log("Remote addr sent trigger ok\r\n");
        }
        else
        {
            Log("Remote addr send busy\r\n");
        }
    }
}

/***********************用户应用层结束*****************/

/********************************应用层结束**************************************/

void version_printf(void)
{
    appPrintf(LOG_NOTIC, "Version:%s\r\n", __VERSION__);
}

void app_Init(void)
{
    All_Tim_Init();  // 定时器初始化
    user_key_Init(); // 按键初始化
    user_led_init(); // LED初始化
    addr_tx_init();  // 地址发送初始化
    iwdg_Init();     // 看门狗初始化
    // DEBUG_USART_Config(); // 将串口配置成日志口
    version_printf(); // 版本打印
}

void app_lication(void)
{
    while (1)
    {
        iwdg_FeedDog();                  // 喂狗
        user_key_handle();               // 按键处理
        user_led_handle();               // LED处理
        get_remote_address_code();       // 轮询获取遥控器地址并尝试发送
        try_send_remote_addr_if_ready(); // 轮询同步条件再尝试发送
    }
}
