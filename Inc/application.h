#ifndef __APPLICATION_H
#define __APPLICATION_H

#include "adc.h"
#include "addr_tx.h"
#include "bat.h"
#include "encoder.h"
#include "iwdg.h"
#include "key_manager.h"
#include "led_manager.h"
#include "main.h"
#include "tim.h"
#include "pwr_stop.h"
#include "radio.h"


// JSM是项目编号，06是硬件版本（如换板子等）06是软件大版本，009是软件小版本
#define __VERSION__     "MJ-1.0_Remote_00.00.001"

/*========================= 宏定义 =========================*/
#define BAT_VOL_LOW  3400 // 电池低电提醒阈值
#define BAT_VOL_WARN 3300 // 电池低电警告阈值
#define BAT_VOL_OFF  3200 // 电池关机阈值

#define ENTER_SLEEP_TIME 10000 // 进入休眠时间阈值

typedef enum
{
    APP_STATE_SLEEP,    // 休眠状态
    APP_STATE_WORK,     // 工作状态
    APP_STATE_CHARGING, // 充电状态
    APP_STATE_FULL,     // 充满状态
} appState_t;

typedef struct
{
    bool low_power_flag;         // 低电关机标志位
    appState_t state;            // 当前状态
    uint32_t work_time;          // 工作时间戳
    bool full_charging_flag;     // 充电满标志位
    uint32_t charging_full_time; // 充电满时间戳
    uint32_t speed;              // 转速
} app_state_t;

#define	ADDR0		0x33
#define	ADDR1		0x33

void app_Init(void);
void app_lication(void);

#endif
