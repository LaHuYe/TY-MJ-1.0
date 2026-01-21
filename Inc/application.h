#ifndef __APPLICATION_H
#define __APPLICATION_H


#include "main.h"
#include "tim.h"
#include "iwdg.h"
#include "addr_rx.h"
#include "addr_tx.h"
#include "key_manager.h"
#include "led_manager.h"


// JSM是项目编号，06是硬件版本（如换板子等）06是软件大版本，009是软件小版本
#define __VERSION__     "MJ-1.0_Charge_00.00.001"
#define __EMAIL__       "1917507415@qq.com"
#define __COMMIT_HASH__ "ebd4c62979a36cd0e0eb0bab7002012b76c2e932"

void app_Init(void);
void app_lication(void);

#endif
