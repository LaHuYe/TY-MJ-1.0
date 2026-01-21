#ifndef __TIM_H
#define __TIM_H

#include "main.h"

void All_Tim_Init(void);
void Tim1_PwmPulseSet(uint32_t Channel, uint32_t Pulse);
void Tim14_PwmPulseSet(uint32_t Channel, uint32_t Pulse);
#endif
