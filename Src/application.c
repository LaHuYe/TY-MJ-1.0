#include "application.h"
#include "py32f0xx_hal.h"
#include "cmt2300a.h"
#include "cmt_spi3.h"
#include	"radio.h"

/****************** 初始化层开始*******************/

static app_state_t app_state = {
    .state = APP_STATE_SLEEP,
    .gear = 1, // 默认档位为1
};
extern CircularQueue s_modeCheckQueue;
#define RF_RX_TIMEOUT    1000*60*60      //60min
#define RF_PACKET_SIZE   6               /* Define the payload size here */

static uint8_t RxBuffer[RF_PACKET_SIZE];   /* RF Rx buffer */

char str[32];
uint32_t g_nRecvCount=0,g_nSendCount=0;

void app_close_all_device(void);
/****************** 初始化层开始*******************/
/****************** KEY 应用事件开始*****************/

/**
 * @brief 电源按键短按回调函数
 * @param none
 * @return none
 */
void key_power_ShortPress(void)
{
    appPrintf(LOG_NOTIC, "key_power_ShortPress\r\n");

    if (!app_state.startup_flag)
    {
        return;
    }
    app_state.gear++;
    // 档位最大为BLDC_GEAR_100
    if (app_state.gear >= BLDC_GEAR_MAX)
    {
        app_state.gear = BLDC_GEAR_100;
    }
    appPrintf(LOG_NOTIC, "Gear:%d\r\n", app_state.gear);

    if (app_state.state == APP_STATE_STARTUP)
    {
        set_bldc_power_enable(true);            // 设置BLDC供电使能
        bldc_app_init();                        // 初始化BLDC电机
        app_state.state = APP_STATE_MUSCLE_GUN; // 进入筋膜枪状态
    }
    bldc_set_gear((BLDC_Gear_t)app_state.gear); // 设置BLDC档位
    LED_EventAdd(LED_EVENT_KEY_OPERATION);      // 添加按键操作事件
}
/**
 * @brief 电源按键长按回调函数
 * @param none
 * @return none
 */
void key_power_longPress(void)
{
    appPrintf(LOG_NOTIC, "key_power_longPress\r\n");

    // 充电状态或充电满状态或电池温度异常状态，不进行开机
    if (app_state.state == APP_STATE_CHARGING ||
        app_state.state == APP_STATE_FULL_CHARGING)
    {
        return;
    }

    // 如果有低电关机事件，则不进行开机
    if (app_state.low_power_shutdown_flag == true)
    {
        return;
    }

    app_state.gear = 1;                               // 不管开关机档位都设置为1档
    app_state.startup_flag = !app_state.startup_flag; // 开关机标志位

    if (app_state.startup_flag)
    {
        app_state.state = APP_STATE_STARTUP;     // 开机状态
        LED_EventAdd(LED_EVENT_STARTUP_DISPLAY); // 开机显示
        app_state.startup_time = HAL_GetTick();  // 重置开机时间戳
    }
    else
    {
        app_close_all_device(); // 关闭所有外设
        app_state.state = APP_STATE_SLEEP;
    }
}

/**
 * @brief 充电按键插入回调函数
 * @param none
 * @return none
 */
void key_charge_InsertPress(void)
{
    appPrintf(LOG_NOTIC, "key_charge_InsertPress\r\n");
    app_state.startup_flag = false;            // 关闭开机标志位
    app_close_all_device();                    // 关闭所有外设
    set_charge_enable(true);                   // 设置充电使能
    LED_EventAdd(LED_EVENT_CHARGING);          // 添加充电事件
    app_state.low_power_shutdown_flag = false; // 设置低电关机标志位为false
    app_state.low_power_start_time = 0;        // 清零低电开始时间戳
    app_state.state = APP_STATE_CHARGING;      // 设置应用状态为充电状态
}

/**
 * @brief 充电按键拔出回调函数
 * @param none
 * @return none
 */
void key_charge_ExtractPress(void)
{
    appPrintf(LOG_NOTIC, "key_charge_ExtractPress\r\n");
    // 充电状态或充电满状态，拔出充电线，进入休眠状态
    if (app_state.state == APP_STATE_CHARGING || app_state.state == APP_STATE_FULL_CHARGING)
    {
        app_close_all_device();               // 关闭所有外设
        set_charge_enable(false);             // 充电芯片不使能
        app_state.full_charging_flag = false; // 设置充电满标志位为false
        app_state.state = APP_STATE_SLEEP;    // 进入休眠状态
    }
}

/**
 * @brief 充电按键充满回调函数
 * @param none
 * @return none
 */
void key_charge_FullPress(void)
{
    appPrintf(LOG_NOTIC, "key_charge_FullPress\r\n");
    if (app_state.state == APP_STATE_CHARGING)
    {
        app_state.full_charging_flag = true;          // 设置充电满标志位
        app_state.charging_full_time = HAL_GetTick(); // 记录充电满时间戳
    }
}

/**
 * @brief 充电按键未充满回调函数
 * @param none
 * @return none
 */
void key_charge_NotFullPress(void)
{
    appPrintf(LOG_NOTIC, "key_charge_NotFullPress\r\n");
    if (app_state.state == APP_STATE_CHARGING)
    {
        app_state.full_charging_flag = false; // 设置充电满标志位
    }
}

/****************** KEY 应用事件结束*********************/

/****************** LED 应用事件开始**********************/

/**
 * @brief 开机显示处理
 * @param led_config
 * @return none
 */
uint8_t led_startup_display_handle(LED_Config_t *led_config)
{
    LED_Update(led_config);
    return 0;
}

/**
 * @brief   按键操作处理
 * @param led_config
 * @return none
 */
uint8_t led_key_operation_handle(LED_Config_t *led_config)
{
    if (!LED_Update(led_config))
    {
        LED_EventDelete(LED_EVENT_KEY_OPERATION);
    }
    return 1;
}

/**
 * @brief 充电状态显示处理
 * @param led_config
 * @return none
 */
uint8_t led_charging_handle(LED_Config_t *led_config)
{
    LED_Update(led_config);
    return 0;
}

/**
 * @brief 充电满状态显示处理
 * @param led_config
 * @return none
 */
uint8_t led_full_charging_handle(LED_Config_t *led_config)
{
    LED_Update(led_config);
    return 0;
}

/**
 * @brief 低电关机处理
 * @param led_config
 * @return none
 */
uint8_t led_low_power_shutdown_handle(LED_Config_t *led_config)
{
    if (!LED_Update(led_config))
    {
        app_state.state = APP_STATE_SLEEP;             // 进入休眠状态
        LED_EventDelete(LED_EVENT_LOW_POWER_SHUTDOWN); // 删除低电关机事件
    }
    return 0;
}

/****************** LED 应用事件结束*********************/

/*******************用户应用层开始**********************/

/**
 * @brief   关闭所有外设
 * @param   none
 * @return  none
 * @note   关闭所有外设
 */
void app_close_all_device(void)
{
    LED_EventAllDelete();         // 关闭所有LED事件
    bldc_set_gear(BLDC_GEAR_OFF); // 设置BLDC档位为OFF档
    set_bldc_power_enable(false); // 关闭BLDC供电使能
}

/**
 * @brief   低电处理
 * @param   none
 * @return  none
 * @note    低电处理
 */
void low_power_handle(void)
{
    // 设备运行状态，进行低电处理
    if (app_state.startup_flag)
    {
        // 如果已经触发低电关机，直接返回
        if (app_state.low_power_shutdown_flag)
        {
            return;
        }

        // 电池电量小于低电阈值
        if (bat_get_processed_voltage_mv() <= LOW_POWER_THRESHOLD)
        {
            // 如果还没有记录低电开始时间，记录当前时间
            if (app_state.low_power_start_time == 0)
            {
                app_state.low_power_start_time = HAL_GetTick();
            }
            else
            {
                // 检查是否连续2秒都低于阈值
                if (HAL_GetTickDiff(app_state.low_power_start_time) >= LOW_POWER_DETECT_TIME)
                {
                    // 连续2秒都低于阈值，判定为低电，进入低电关机
                    app_close_all_device();                     // 关闭所有外设
                    app_state.low_power_shutdown_flag = true;   // 设置低电关机标志位为true
                    app_state.low_power_start_time = 0;         // 清零低电开始时间戳
                    app_state.state = APP_STATE_ALARM;          // 进入报警状态处理灯显
                    LED_EventAdd(LED_EVENT_LOW_POWER_SHUTDOWN); // 添加低电LED事件
                    appPrintf(LOG_WARNING, "battery low\r\n");
                }
            }
        }
        else
        {
            // 电池电量高于阈值，清零低电开始时间戳（防抖）
            app_state.low_power_start_time = 0;
        }
    }
    else
    {
        // 设备未运行，清零低电开始时间戳
        app_state.low_power_start_time = 0;
    }
}

/**
 * @brief   堵转处理
 * @param   none
 * @return  none
 * @note    堵转处理
 */
void stuck_handle(void)
{
    if (bldc_is_stalled())
    {
        bldc_clear_stalled_flag();         // 清除堵转标志
        app_close_all_device();            // 关闭所有外设
        app_state.state = APP_STATE_SLEEP; // 进入报警状态处理灯显
        appPrintf(LOG_WARNING, "BLDC motor stuck\r\n");
    }
}

/*******************用户应用层结束*****************/

/*******************状态机应用层开始********************/

/**
 * @brief   设备开机处理
 * @param   none
 * @return  none
 * @note    设备开机处理
 */
static void device_startup_handle(void)
{
    // 开机时间超过阈值，则进入休眠状态
    if (HAL_GetTickDiff(app_state.startup_time) >= STARTUP_TIME)
    {
        app_close_all_device();            // 关闭所有外设
        app_state.state = APP_STATE_SLEEP; // 进入休眠状态
        appPrintf(LOG_NOTIC, "startup time out\r\n");
    }
}

/**
 * @brief   设备休眠处理
 * @param   none
 * @return  none
 * @note    设备休眠处理
 */
static void device_sleep_handle(void)
{
    static uint32_t stop_interval_time = 0;
    static bool startup_flag = false; // 首次上电标志位

    // 刚开机直接进入休眠或者已经唤醒超过3秒，则进入休眠模式
    if (HAL_GetTickDiff(stop_interval_time) >= 3000 || !startup_flag)
    {
        appPrintf(LOG_NOTIC, "device sleep\r\n");
        app_state.startup_flag = false;     // 关闭开机标志位
        startup_flag = true;                // 首次上电标志位
        LED_EventAllDelete();               // 删除所有LED事件
        reset_key_Status();                 // 重置按键状态
        HAL_Delay(1);                       // 延时1ms，等待LED完全熄灭
        mcu_enter_sleep();                  // 进入休眠模式
        stop_interval_time = HAL_GetTick(); // 记录进入休眠时间
        appPrintf(LOG_NOTIC, "device wakeup\r\n");
    }
    if (bat_get_processed_voltage_mv() >= ALLOW_STARTUP_THRESHOLD)
    {
        app_state.low_power_shutdown_flag = false; // 设置低电关机标志位为false
    }
    else
    {
        app_state.low_power_shutdown_flag = true; // 设置低电关机标志位为true
    }
}

/**
 * @brief   设备筋膜枪模式处理
 * @param   none
 * @return  none
 * @note    设备筋膜枪模式处理
 */
static void device_muscle_gun_handle(void)
{
    Motor_CurrentLimitHandle(); // 过流保护（有浮点运算，不能放在中断里，放到while中）
    stuck_handle();             // 堵转处理
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
            app_state.state = APP_STATE_FULL_CHARGING; // 进入充电满状态
            LED_EventAdd(LED_EVENT_FULL_CHARGING);     // 添加充电满LED事件
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
        appPrintf(LOG_NOTIC, "app_state.state:%d\r\n", app_state.state);
    }

    switch (app_state.state)
    {
    case APP_STATE_STARTUP:
        // 开机状态
        device_startup_handle();
        break;
    case APP_STATE_SLEEP:
        // 休眠状态
        device_sleep_handle();
        break;
    case APP_STATE_MUSCLE_GUN:
        // 筋膜枪状态
        device_muscle_gun_handle();
        break;
    case APP_STATE_CHARGING:
        // 充电状态
        device_charging_handle();
        break;
    case APP_STATE_FULL_CHARGING:
        // 充电满状态
        break;
    case APP_STATE_ALARM:
        // 报警状态
        break;
    default:
        break;
    }
}

/**
 * @brief   应用状态机处理
 * @param   none
 * @return  none
 * @note    应用状态机处理
 */
void app_RF_Recv_handle()
{
		uint8_t	i = 0;
		uint8_t	sum = 0;
	
		printf("recv:%d  0x%u  0x%u 0x%u 0x%u 0x%u 0x%u\n", g_nRecvCount,RxBuffer[0],RxBuffer[1],RxBuffer[2],RxBuffer[3],RxBuffer[4],RxBuffer[5]);
		
		sum = (RxBuffer[0] + RxBuffer[1] + RxBuffer[2] + RxBuffer[3] + RxBuffer[4])&0xFF;
		
		if((RxBuffer[0] == 0xAA)&&(RxBuffer[5] == sum))
		{
			if(RxBuffer[3] == 0x10)
			{
				//加速.
				 app_state.gear++;
				// 档位最大为BLDC_GEAR_100
				if (app_state.gear >= BLDC_GEAR_MAX)
				{
					app_state.gear = BLDC_GEAR_100;
				}
				bldc_set_gear((BLDC_Gear_t)app_state.gear);
				LED_EventAdd(LED_EVENT_KEY_OPERATION); 
			}
			else if(RxBuffer[2] == 0x10)
			{
				//减速
				app_state.gear--;
				// 档位最大为BLDC_GEAR_100
				if (app_state.gear <= BLDC_GEAR_1)
				{
					app_state.gear = BLDC_GEAR_1;
				}
				bldc_set_gear((BLDC_Gear_t)app_state.gear);
				LED_EventAdd(LED_EVENT_KEY_OPERATION); 
			}
			
			if(RxBuffer[4] == 0x10)
			{
				if(app_state.state != APP_STATE_MUSCLE_GUN)
				{
					//启动
					set_bldc_power_enable(true);            // 设置BLDC供电使能
					bldc_app_init();                        // 初始化BLDC电机
					app_state.state = APP_STATE_MUSCLE_GUN; // 进入筋膜枪状
					bldc_set_gear((BLDC_Gear_t)app_state.gear); // 设置BLDC档位
					LED_EventAdd(LED_EVENT_KEY_OPERATION);      // 添加按键操作事件
				}
			}
			else if(RxBuffer[4] == 0x40)
			{
				//待机
				set_bldc_power_enable(false); 
				app_state.state = APP_STATE_STARTUP; 
				app_close_all_device(); 
			}
			else if(RxBuffer[4] == 0x80)
			{
				//关机
				app_state.state = APP_STATE_SLEEP;
				device_sleep_handle();
			}
			
		}
		for(i=0;i<RF_PACKET_SIZE;i++) //Clear Buff
				RxBuffer[i]=0;
				 
}
/***************状态机应用层结束************************/

/********************************应用层结束**************************************/

void version_printf(void)
{
    appPrintf(LOG_NOTIC, "Developer:%s\r\n", __DEVELOPER__);
    appPrintf(LOG_NOTIC, "Email:%s\r\n", __EMAIL__);
    appPrintf(LOG_NOTIC, "Version:%s\r\n", __VERSION__);
    appPrintf(LOG_NOTIC, "Compiler Date:%s\r\n", __DATE__);
    appPrintf(LOG_NOTIC, "Compiler Time:%s\r\n", __TIME__);
    appPrintf(LOG_NOTIC, "COMMIT_HASH:%s\r\n", __COMMIT_HASH__);
}

extern int32_t test;
extern uint32_t test_pwm;

void app_Init(void)
{
    // 系统初始化
		cmt_spi3_init();	//433初始化
	
    DEBUG_USART_Config(); // 将串口配置成日志口
    version_printf();     // 打印版本信息

    // 外设初始化
    adc_Init();        // 初始化ADC
    All_Tim_Init();    // 初始化定时器
    user_key_Init();   // 初始化按键
    user_led_init();   // 初始化LED
    power_gpio_init(); // 初始化电源控制

    // 模块初始化
    bat_init(); // 初始化电池
		
		RF_Init();
    // set_bldc_power_enable(true);            // 设置BLDC供电使能
    // bldc_app_init();                        // 初始化BLDC电机
    // bldc_set_gear((BLDC_Gear_t)app_state.gear); // 设置BLDC档位
}

uint8_t Radio_Recv_FixedLen(uint8_t pBuf[],uint8_t len)
{
#ifdef ENABLE_ANTENNA_SWITCH
	      if(CMT2300A_ReadGpio3())  /* Read INT2, PKT_DONE */
#else
				if(CMT2300A_ReadGpio1()) /* Read INT1, SYNC OK */
				{
				  /******/
				}
        if(CMT2300A_ReadGpio2())  /* Read INT2, PKT_DONE */
#endif	
		   {
//        if(CMT2300A_MASK_PKT_OK_FLG & CMT2300A_ReadReg(CMT2300A_CUS_INT_FLAG))  /* Read PKT_OK flag */
				 {
						CMT2300A_GoStby();
						CMT2300A_ReadFifo(pBuf,len);
						CMT2300A_ClearRxFifo();
						CMT2300A_ClearInterruptFlags();
						CMT2300A_GoRx();
						
						return 1;
				 }
		   }

		return 0;
}

void radio_Recv(void)
{

	 if(Radio_Recv_FixedLen(RxBuffer,RF_PACKET_SIZE))
			 {
				  g_nRfRxtimeoutCount=0;  //清除 Time Out计数
				  g_nRecvCount++;
					app_RF_Recv_handle();

			 }
        
       if(g_nRfRxtimeoutCount>RF_RX_TIMEOUT) //根据实际应用可以调整Time Out
			 {
				  g_nRfRxtimeoutCount=0;
					CMT2300A_GoSleep();
					CMT2300A_GoStby();
					CMT2300A_ClearInterruptFlags();
					CMT2300A_ClearRxFifo();
					CMT2300A_GoRx();
			 }				 
}

void app_lication(void)
{
    while (1)
    {
        // Log("test:%d, test_pwm:%d speed:%dRPM\r\n", test, test_pwm, BLDC_COMP_GetSpeed());
        user_key_handle();    // 按键处理函数
        user_led_handle();    // LED处理函数
        low_power_handle();   // 低电处理
        app_machine_handle(); // 状态机处理函数
				radio_Recv();					//接收天线处理函数
    }
}
