#include "application.h"
#include "typedefs.h"

#include "cmt2300a_hal.h"
#include "cmt2300a.h"
#include "cmt_spi3.h"
#include "stdio.h"

/*****************初始化层*******************/
app_state_t app_state = {
    .low_power_flag = false,
    .state = APP_STATE_SLEEP,

};

uint8_t	TX_Buffer[RF_PACKET_SIZE];		//
/****************************应用层*********************************/

/******************** KEY 应用事件开始*****************/

/**
 * @brief 电源按键短按事件
 * @param none
 * @return none
 */
void key_power_shortPress(void)
{
    printf("key_power_shortPress\r\n");
    if (app_state.state != APP_STATE_WORK)
        return;

    app_state.work_time = HAL_GetTick(); // 记录工作时间戳
    LED_EventAdd(LED_EVENT_KEY_PRESS);   // 添加按键事件

    // TODO 发送电机启动命令
		TX_Buffer[3] = 0x00;
		TX_Buffer[4] = 0x10;		//启动指令
		TX_Buffer[5]=(TX_Buffer[0]+TX_Buffer[1]+TX_Buffer[2]+TX_Buffer[3]+TX_Buffer[4])&0xFF;
	
		Radio_Send_FixedLen(TX_Buffer,RF_PACKET_SIZE);	//发射
}

/**
 * @brief 电源按键双击事件
 * @param none
 * @return none
 */
void key_power_dbclPress(void)
{
    printf("key_power_dbclPress\r\n");
    if (app_state.state != APP_STATE_WORK)
        return;

    app_state.work_time = HAL_GetTick(); // 记录工作时间戳

    // TODO 发送电机停止命令
		TX_Buffer[3] = 0x00;
		TX_Buffer[4] = 0x40;		//待机指令
		TX_Buffer[5]=(TX_Buffer[0]+TX_Buffer[1]+TX_Buffer[2]+TX_Buffer[3]+TX_Buffer[4])&0xFF;
	
		Radio_Send_FixedLen(TX_Buffer,RF_PACKET_SIZE);	//发射
}

/**
 * @brief 电源按键长按事件
 * @param none
 * @return none
 */
void key_power_longPress(void)
{
    printf("key_power_longPress\r\n");

    // TODO 发送主机关机命令
		TX_Buffer[3] = 0x00;
		TX_Buffer[4] = 0x80;		//关机指令
		TX_Buffer[5]=(TX_Buffer[0]+TX_Buffer[1]+TX_Buffer[2]+TX_Buffer[3]+TX_Buffer[4])&0xFF;
	
		Radio_Send_FixedLen(TX_Buffer,RF_PACKET_SIZE);	//发射
}

/**
 * @brief 充电器插入按键长按事件
 * @param none
 * @return none
 */
void key_power_insert_longPress(void)
{
    printf("key_power_insert_longPress\r\n");
    // TODO 发送遥控器地址，调用addr_tx_enable即可发送
    LED_EventAdd(LED_EVENT_CHARGING);     // 添加充电事件
    app_state.low_power_flag = false;     // 设置低电关机标志位为false
    app_state.state = APP_STATE_CHARGING; // 设置应用状态为充电状态
}

/**
 * @brief 充电器插入按键释放事件
 * @param none
 * @return none
 */
void key_power_insert_releasePress(void)
{
    printf("key_power_insert_releasePress\r\n");
    // 充电状态或充电满状态，拔出充电线，进入休眠状态
    if (app_state.state == APP_STATE_CHARGING || app_state.state == APP_STATE_FULL)
    {
        app_state.full_charging_flag = false; // 设置充电满标志位为false
        app_state.state = APP_STATE_SLEEP;    // 进入休眠状态
    }
}

/**
 * @brief 充满按键长按事件
 * @param none
 * @return none
 */
void key_full_longPress(void)
{
    printf("key_full_longPress\r\n");
    if (app_state.state == APP_STATE_CHARGING)
    {
        app_state.full_charging_flag = true;          // 设置充电满标志位
        app_state.charging_full_time = HAL_GetTick(); // 记录充电满时间戳
    }
}

/**
 * @brief 充满按键释放事件
 * @param none
 * @return none
 */
void key_full_releasePress(void)
{
    printf("key_full_releasePress\r\n");
    if (app_state.state == APP_STATE_CHARGING)
    {
        app_state.full_charging_flag = false; // 设置充电满标志位
    }
}

/****************** KEY 应用事件结束*********************/

/******************** LED 应用事件开始*****************/
uint8_t led_standby_handler(LED_Config_t *led_config)
{
    LED_Update(led_config);
    return 0;
}
uint8_t led_low_power_handler(LED_Config_t *led_config)
{
    LED_Update(led_config);
    return 0;
}
uint8_t led_low_warn_handler(LED_Config_t *led_config)
{
    LED_Update(led_config);
    return 0;
}
uint8_t led_key_press_handler(LED_Config_t *led_config)
{
    if (LED_Update(led_config) == 0)
    {
        LED_EventDelete(LED_EVENT_KEY_PRESS);
        return 0;
    }
    return 1;
}
uint8_t led_charging_handler(LED_Config_t *led_config)
{
    LED_Update(led_config);
    return 0;
}
uint8_t led_full_handler(LED_Config_t *led_config)
{
    LED_Update(led_config);
    return 0;
}

/******************** LED 应用事件结束*****************/

/***********************用户应用层开始**********************/
// 获取编码器处理
void encoder_process(void)
{
		
		
    encoder_mode_t mode = encoder_get_mode();
    if (mode != ENCODER_MODE_NONE)
    {
        app_state.work_time = HAL_GetTick(); // 记录工作时间戳
        LED_EventAdd(LED_EVENT_KEY_PRESS);   // 添加按键事件
        if (mode == ENCODER_MODE_CW)
        {
            // 减少转速
            app_state.speed -= 23;
//            if (app_state.speed < 500)
//            {
//                app_state.speed = 500;
//            }
						TX_Buffer[3] = 0x20;
						TX_Buffer[4] = 0x00;
        }
        else if (mode == ENCODER_MODE_CCW)
        {
            // 增加转速
            app_state.speed += 23;
//            if (app_state.speed > 2800)
//            {
//                app_state.speed = 2800;
//            }
						TX_Buffer[3] = 0x10;
						TX_Buffer[4] = 0x00;
        }
				
				TX_Buffer[5]=(TX_Buffer[0]+TX_Buffer[1]+TX_Buffer[2]+TX_Buffer[3]+TX_Buffer[4])&0xFF;

        // TODO 发送转速命令
				Radio_Send_FixedLen(TX_Buffer,RF_PACKET_SIZE);
    }
}

// 获取电池电压处理
void bat_process(void)
{
    uint16_t bat_vol_mv = bat_get_voltage_mv();
//    printf("bat_vol_mv:%d\r\n", bat_vol_mv);
    /* 电池电压处理 */
    if (bat_vol_mv <= BAT_VOL_OFF - 5)
    {
        /* 关机 */
        app_state.low_power_flag = true;
        app_state.state = APP_STATE_SLEEP; // 进入休眠状态
        return;
    }
    else if (bat_vol_mv >= BAT_VOL_OFF + 5 && bat_vol_mv <= BAT_VOL_WARN - 5)
    {
        /* 低电警告 */
        LED_EventAdd(LED_EVENT_LOW_WARN);
    }
    else if (bat_vol_mv >= BAT_VOL_WARN + 5 && bat_vol_mv <= BAT_VOL_LOW)
    {
        /* 低电提醒 */
        LED_EventAdd(LED_EVENT_LOW_POWER);
    }
    else if (bat_vol_mv >= BAT_VOL_LOW + 5)
    {
        /* 电量正常 */
        LED_EventAdd(LED_EVENT_STANDBY);
    }
}

/***********************用户应用层结束*****************/

/*******************状态机应用层开始********************/

/**
 * @brief   设备休眠处理
 * @param   none
 * @return  none
 * @note    设备休眠处理
 */
static void device_sleep_handle(void)
{
    // 直接进入休眠模式
    printf("device sleep\r\n");
    // TODO 发送睡眠命令
    LED_EventAllDelete();                // 删除所有LED事件
    mcu_enter_sleep();                   // 进入休眠模式
    if (!app_state.low_power_flag)
    {
        app_state.state = APP_STATE_WORK;    // 进入工作状态
        app_state.work_time = HAL_GetTick(); // 记录工作时间戳
    }
    printf("device wakeup\r\n");
}

/**
 * @brief   设备工作处理
 * @param   none
 * @return  none
 * @note    设备工作处理
 */
static void device_work_handle(void)
{
    
    bat_process();     // 电池电压处理
    /* 低电关机 */
    if (app_state.low_power_flag)
    {
        app_state.state = APP_STATE_SLEEP; // 进入休眠状态
        return;
    }
    encoder_process(); // 编码器处理
    if (HAL_GetTickDiff(app_state.work_time) >= ENTER_SLEEP_TIME)
    {
        LED_EventAllDelete();              // 删除所有LED事件
        app_state.state = APP_STATE_SLEEP; // 进入休眠状态
    }
}

/**
 * @brief   设备充电处理
 * @param   none
 * @return  none
 * @note    设备充电处理
 */
void device_charging_handle(void)
{
    // 充满且超过1秒，则进入充电满状态
    if (app_state.full_charging_flag)
    {
        if (HAL_GetTickDiff(app_state.charging_full_time) >= 1000)
        {
            app_state.state = APP_STATE_FULL; // 进入充电满状态
            LED_EventAdd(LED_EVENT_FULL);     // 添加充电满LED事件
        }
    }
    else
    {
        app_state.charging_full_time = HAL_GetTick(); // 重置充电满时间戳
    }
}

/**
 * @brief   应用状态机处理
 * @param   none
 * @return  none
 * @note    应用状态机处理
 */
void app_machine_handle(void)
{
    static appState_t lst_state = APP_STATE_SLEEP;
    if (lst_state != app_state.state)
    {
        lst_state = app_state.state;
        printf("app_state.state:%d\r\n", app_state.state);
    }

    switch (app_state.state)
    {
    case APP_STATE_SLEEP:
        // 休眠状态
        device_sleep_handle();
        break;
    case APP_STATE_WORK:
        // 工作状态
        device_work_handle();
        break;
    case APP_STATE_CHARGING:
        // 充电状态
        device_charging_handle();
        break;
    case APP_STATE_FULL:
        // 充满状态
        break;
    default:
        break;
    }
}

/***************状态机应用层结束************************/

/********************************应用层结束**************************************/

void version_printf(void)
{
    printf("Version:%s\r\n", __VERSION__);
}
void Tx_Buffer_init(void)
{
		TX_Buffer[0] = 0xAA;
		TX_Buffer[1] = ADDR0;
		TX_Buffer[2] = ADDR1;
		TX_Buffer[3] = 0x00;
		TX_Buffer[4] = 0x00;
		TX_Buffer[5]=(TX_Buffer[0]+TX_Buffer[1]+TX_Buffer[2]+TX_Buffer[3]+TX_Buffer[4])&0xFF;
}

void app_Init(void)
{
    adc_Init();      // ADC初始化
    All_Tim_Init();  // 定时器初始化
    user_key_Init(); // 按键初始化
    user_led_init(); // LED初始化
    encoder_init();  // 编码器初始化
    addr_tx_init();  // 地址发送初始化
    bat_init();      // 电池初始化
    addr_tx_init();  // 地址发送初始化
    // TODO 添加RF初始化
    // iwdg_Init();          // 看门狗初始化
    DEBUG_USART_Config(); // 将串口配置成日志口
    version_printf();     // 版本打印
	
		RF_Init();
		Tx_Buffer_init();
}

void app_lication(void)
{
    while (1)
    {
        // iwdg_FeedDog();       // 喂狗
        user_key_handle();    // 按键处理
        user_led_handle();    // LED处理
        app_machine_handle(); // 应用状态机处理
    }
}
