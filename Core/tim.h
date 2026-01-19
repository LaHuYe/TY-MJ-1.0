#ifndef __TIM_H
#define __TIM_H

#include "main.h"

void All_Tim_Init(void);

// 设置PWM占空比
void Tim14_PwmPulseSet(uint32_t Channel, uint32_t Pulse);
void Tim16_PwmPulseSet(uint32_t Channel, uint32_t Pulse);

// 直接设置CCR值
void Tim1_SetCCR_Direct(uint32_t Channel, uint32_t ccr_value);
void Tim3_SetCCR_Direct(uint32_t Channel, uint32_t ccr_value);
#endif
