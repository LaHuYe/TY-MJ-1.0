#ifndef __ADC_H
#define __ADC_H

#include "main.h"

//ADC参考电压
#define VREF_V       2.5f    // ADC 参考电压(V)

void adc_Init(void);
uint32_t adc_get_value(uint32_t ch);

#endif
