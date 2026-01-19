#ifndef __ADC_H
#define __ADC_H

#include "main.h"

#define ADC_RESOLUTION     4095     // 12位 ADC 最大值
#define VREF_V             3.3f     // 参考电压为 4.0V

void adc_Init(void);
uint32_t adc_get_value(uint32_t ch);

#endif
