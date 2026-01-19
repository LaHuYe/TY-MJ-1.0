#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <main.h>
#include "led.h"
#include "application.h"

typedef enum
{
    USER_LED_GREEN,
    USER_LED_RED,
    USER_LED_MAX
} USER_Led_TypeDef;

void user_led_init(void);
void user_led_handle(void);

#endif // LED_MANAGER_H
