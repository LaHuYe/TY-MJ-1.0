#ifndef __POWER_CONTROL_H
#define __POWER_CONTROL_H

#include "main.h"

/**
 * @brief 初始化电源GPIO
 * @param none
 * @return none
 */
void power_gpio_init(void);

/**
 * @brief 设置充电使能
 * @param state 使能状态
 * @return none
 */
void set_charge_enable(bool state);

/**
 * @brief 设置BLDC供电使能
 * @param state 使能状态
 * @return none
 */
void set_bldc_power_enable(bool state);

/**
 * @brief 获取充电使能状态
 * @return 使能状态
 */
bool get_charge_enable(void);

/**
 * @brief 获取BLDC供电使能状态
 * @return 使能状态
 */
bool get_bldc_power_enable(void);

#endif /* __POWER_CONTROL_H */
