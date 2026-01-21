#ifndef __IWDG_H
#define __IWDG_H

#include "main.h"

#define IWDG_Reload 1024 // 32768/1024/256 = 125mHZ=8s

void iwdg_Init(void);
void iwdg_FeedDog(void);

#endif
