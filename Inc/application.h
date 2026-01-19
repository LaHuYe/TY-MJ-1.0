#ifndef __APPLICATION_H
#define __APPLICATION_H

#include "adc.h"
#include "bat.h"
#include "bldc_app.h"
#include "bldc_comp.h"
#include "common.h"
#include "key.h"
#include "key_manager.h"
#include "led.h"
#include "main.h"
#include "power_control.h"
#include "pwr_stop.h"
#include "tim.h"

// G13是项目编号，02是硬件版本（如换板子等）00是软件大版本，000是软件小版本
#define __VERSION__     "G13_02.00.000"
#define __DEVELOPER__   "WuChuYuan"
#define __EMAIL__       "1917507415@qq.com"
#define __COMMIT_HASH__ "09e1ad24dac69b532a88f8628d81ee3be51de31b"

/*========================= 开机时间阈值 =========================*/
#define STARTUP_TIME 360000 // 开机时间阈值（单位：ms）

/*========================= 低电阈值 =========================*/
#define LOW_POWER_THRESHOLD     9500  // 低电关机阈值（单位：mV）
#define ALLOW_STARTUP_THRESHOLD 10000 // 允许开机电压阈值（单位：mV）
#define LOW_POWER_DETECT_TIME   2000  // 低电连续检测时间（单位：ms，连续2秒低于阈值才判定为低电）

/*========================= 电池温度异常阈值 =========================*/
#define BATTERY_OVER_TEMPERATURE          70 // 电池放电过温阈值
#define BATTERY_OVER_TEMPERATURE_RECOVERY 60 // 电池放电过温恢复阈值

#define BATTERY_OVER_TEMPERATURE_CHARGING          42 // 电池充电过温阈值
#define BATTERY_OVER_TEMPERATURE_CHARGING_RECOVERY 38 // 电池充电过温恢复阈值
#define BATTERY_LOW_TEMPERATURE_CHARGING           3  // 电池充电低温阈值
#define BATTERY_LOW_TEMPERATURE_CHARGING_RECOVERY  8  // 电池充电低温恢复阈值

/*========================= 应用状态枚举 =========================*/
// 应用状态枚举
typedef enum
{
    APP_STATE_STARTUP,       // 开机状态
    APP_STATE_SLEEP,         // 休眠状态
    APP_STATE_MUSCLE_GUN,    // 筋膜枪状态
    APP_STATE_CHARGING,      // 充电状态
    APP_STATE_FULL_CHARGING, // 充电满状态
    APP_STATE_ALARM,         // 报警状态
} appState_t;

#include "led_manager.h"

// 应用状态机结构体
typedef struct
{
    appState_t state;                // 应用状态
    uint8_t gear;                    // 档位
    bool startup_flag;               // 开机标志位
    uint32_t startup_time;           // 开机时间戳
    bool low_power_shutdown_flag;   // 低电关机标志位
    uint32_t low_power_start_time;   // 低电开始时间戳（用于连续检测）
    bool full_charging_flag;         // 充电满标志位
    uint32_t charging_full_time;     // 充电满时间戳
} app_state_t;

void app_Init(void);
void app_lication(void);

#endif
