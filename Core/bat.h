#ifndef __BAT_H
#define __BAT_H

#include "main.h"

/*========================= 硬件配置 =========================*/
/* ADC 分压电阻配置 */
#define BAT_ADC_R1 1000 // 1000KΩ 上分压电阻
#define BAT_ADC_R2 330  // 330KΩ 下分压电阻

/*========================= 电压映射参数 =========================*/
/* 放电电压范围 */
#define BASIC_DISCHARGE_VOLT_MIN 9300.0f // 放电基准未记录时最低电压（0%）
#define BASIC_DISCHARGE_VOLT_MAX 12600.0f // 放电基准未记录时最高电压（100%）

/* 充电电压范围 */
#define BASIC_CHARGE_VOLT_MIN 9300.0f // 充电基准未记录时最低电压（0%）
#define BASIC_CHARGE_VOLT_MAX 12600.0f // 充电基准未记录时最高电压（100%）

/*========================= 函数声明 =========================*/

// 定义电池电量的电压范围宏
// #define BATTERY_DEEP_DISCHARGE 3000 // 深度放电电压
// #define BATTERY_CRITICAL_LOW   3100 // 临界低电压
// #define BATTERY_LEVEL_1        3200
// #define BATTERY_LEVEL_2        3300
// #define BATTERY_LEVEL_3        3400
// #define BATTERY_LEVEL_4        3500
// #define BATTERY_LEVEL_5        3600
// #define BATTERY_LEVEL_6        3700
// #define BATTERY_LEVEL_7        3800
// #define BATTERY_LEVEL_8        3900
// #define BATTERY_LEVEL_9        4000
// #define BATTERY_LEVEL_10       4100

/**
 * @brief   初始化电池管理模块
 * @param   none
 * @return  none
 */
void bat_init(void);

/**
 * @brief   设置电池电压
 * @param   voltage_mv: 电池电压（单位：mV）
 */
void bat_set_voltage(uint16_t voltage_mv);

/**
 * @brief   获取电池电压的移动平均值
 * @param   none
 * @return  电池电压值(mV)
 */
uint16_t bat_get_voltage_mv(void);

/**
 * @brief   每100ms更新一次电池电量
 * @param   none
 */
void bat_update_handle(void);

/**
 * @brief   获取处理后的电池电压（单位：mV）
 * @param   none
 * @return  处理后的电池电压值（单位：mV）
 */
uint16_t bat_get_processed_voltage_mv(void);

#endif
