#ifndef __LPTIME_H
#define __LPTIME_H

#include "main.h"

extern LPTIM_HandleTypeDef       LPTIMConf;

void lptim_clock_config(void);
void lptim_init(void);
void lptim_start(void);

#endif
