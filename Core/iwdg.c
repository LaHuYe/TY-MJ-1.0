#include "iwdg.h"
#include "stdio.h"

IWDG_HandleTypeDef IwdgHandle = {0};

void iwdg_Init(void)
{
    IwdgHandle.Instance = IWDG;                    /* Select IWDG */
    IwdgHandle.Init.Prescaler = IWDG_PRESCALER_256; /* Configure prescaler to 32 */
    IwdgHandle.Init.Reload = IWDG_Reload;          /* Set IWDG counter reload value to 1024, 1s */
    /* Initialize IWDG */
    if (HAL_IWDG_Init(&IwdgHandle) != HAL_OK)
    {
        printf("IWDG Init Failed");
    }
    iwdg_FeedDog();
}

void iwdg_FeedDog(void)
{
    /* Refresh the watchdog */
    if (HAL_IWDG_Refresh(&IwdgHandle) != HAL_OK)
    {
        printf("IWDG FeedDog Failed");
    }
}
