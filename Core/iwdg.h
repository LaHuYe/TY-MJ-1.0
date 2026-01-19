#ifndef __IWDG_H
#define __IWDG_H

#include "main.h"

#define IWDG_Reload 3072 // 32768/3072/32 = 333mHZ=3s

void iwdg_Init(void);
void iwdg_FeedDog(void);

#endif
