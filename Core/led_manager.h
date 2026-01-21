#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <main.h>
#include "led.h"


typedef enum
{
    HOST_CHARGE_LED,
    REMOTE_CHARGE_LED,
    USER_LED_MAX,
} LED_TypeDef;

void user_led_init(void);
void user_led_handle(void);

#endif // LED_MANAGER_H
