#ifndef __BAT_H
#define __BAT_H

#include "main.h"

/*========================= 硬件配置 =========================*/
/* ADC 分压电阻配置 */
#define BAT_ADC_R1 100 // 1MΩ 上分压电阻
#define BAT_ADC_R2 47  // 470KΩ 下分压电阻

/*========================= 电压映射参数 =========================*/
/* 放电电压范围 */
#define BASIC_DISCHARGE_VOLT_MIN 9300.0f // 放电基准未记录时最低电压（0%）
#define BASIC_DISCHARGE_VOLT_MAX 12600.0f // 放电基准未记录时最高电压（100%）

/* 充电电压范围 */
#define BASIC_CHARGE_VOLT_MIN 9300.0f // 充电基准未记录时最低电压（0%）
#define BASIC_CHARGE_VOLT_MAX 12600.0f // 充电基准未记录时最高电压（100%）

/*========================= 函数声明 =========================*/

/**
 * @brief   初始化电池管理模块
 * @param   none
 * @return  none
 */
void bat_init(void);


/**
 * @brief   获取电池电压的移动平均值
 * @param   none
 * @return  电池电压值(mV)
 */
uint16_t bat_get_voltage_mv(void);


#endif
