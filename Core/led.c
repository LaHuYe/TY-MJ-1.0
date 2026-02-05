/**
 * @file led.c
 * @author cyWu (1917507415@qq.com)
 * @brief LED应用层框架
 * @version 0.1
 * @date 2024-02-22
 * @copyright Copyright (c) 2024
 *
 */

#include "led.h"
#include "led_manager.h"
#include "tim.h"


static LED_EventTableItem_t ledPriorityTable[PRIORITY_MAX];
static LED_EventTableItem_t ledEventTable[LED_EVENT_MAX];
static LED_SetState_t LED_SetState;
static TIME_Get_t TIME_Get;

// 设置led状态回调函数
void LED_SetStateInit(LED_SetState_t setState)
{
    LED_SetState = setState;
}

// 设置获取时间戳回调函数
void SET_TimeGetInit(TIME_Get_t getTime)
{
    TIME_Get = getTime;
}

// LED事件表初始化
void LED_EventTableInit(LED_EventTableItem_t ledEvent[])
{
    if (ledEvent == NULL)
    {
        ledPrintf(LOG_ERROR, "ledEvent is NULL\r\n");
        return;
    }
    memset(ledPriorityTable, 0, sizeof(ledPriorityTable));
    memset(ledEventTable, 0, sizeof(ledEventTable));
    memcpy(ledEventTable, ledEvent, sizeof(ledEventTable));
}

// 将LED事件加入事件表
void LED_EventAdd(LED_Event_t ledEvent)
{
    for (size_t i = 0; i < LED_EVENT_MAX; i++)
    {
        if (ledEventTable[i].event == ledEvent)
        {
            if (ledPriorityTable[ledEventTable[i].priority].event != ledEvent)
            {
                ledEventTable[i].config.currentTime = TIME_Get(); // 记录开始执行时的时间戳
                if (ledPriorityTable[ledEventTable[i].priority].event != 0)
                {
                    if (LED_SetState == NULL)
                    {
                        ledPrintf(LOG_ERROR, "LED_SetState is NULL!!!\r\n");
                        return;
                    }
                    LED_SetState((ledPriorityTable[ledEventTable[i].priority].config.ledMask) ^ (ledEventTable[i].config.ledMask), LED_OFF);
                }
            }
            ledPriorityTable[ledEventTable[i].priority] = ledEventTable[i]; // 相同优先级的会覆盖
            return;
        }
    }
}

// 将所有LED事件从事件表删除
void LED_EventAllDelete(void)
{
    for (size_t i = 0; i < PRIORITY_MAX; i++)
    {
        memset(&ledPriorityTable[i], 0, sizeof(LED_EventTableItem_t));
    }
}

// 将LED事件从事件表删除
void LED_EventDelete(LED_Event_t ledEvent)
{
    for (size_t i = 0; i < PRIORITY_MAX; i++)
    {
        if (ledPriorityTable[i].event == ledEvent)
        {
            memset(&ledPriorityTable[i], 0, sizeof(LED_EventTableItem_t));
            if (LED_SetState == NULL)
            {
                ledPrintf(LOG_ERROR, "LED_SetState is NULL!!!\r\n");
                return;
            }
            LED_SetState(ledEventTable[ledEvent].config.ledMask, LED_OFF);
            return;
        }
    }
}

// LED事件处理函数
void LED_EventHandle(void)
{
    uint8_t ret;
    int i;
    for (i = (PRIORITY_MAX - 1); i >= 0; i--)
    { // 从高优先级开始往下查询
        if (ledPriorityTable[i].event != 0)
        {
            ret = ledPriorityTable[i].ledEventHandler(&ledPriorityTable[i].config); // 执行对应LED事件处理函数
            // 正在执行事件功能返回1，执行完事件功能返回0，例：每30秒闪烁1次，闪烁的1s里返回1，闪烁完成返回0，目的是使间隔的30秒内可以运行别的LED事件
            if (ret)
            {
                return;
            }
        }
    }
}

// 判断是否有 LED 事件正在运行
uint8_t LED_IsEventActive(void)
{
    for (size_t i = 0; i < PRIORITY_MAX; i++)
    {
        if (ledPriorityTable[i].event != 0)
        {
            return 1;
        }
    }
    return 0;
}

// 查询LED事件表中是否存在指定事件
uint8_t LED_IsEventExist(LED_Event_t ledEvent)
{
    for (size_t i = 0; i < PRIORITY_MAX; i++)
    {
        if (ledPriorityTable[i].event == ledEvent)
        {
            return 1;
        }
    }
    return 0;
}

// 计算时间差
uint32_t GetTickDiff(uint32_t meiosis)
{
    uint32_t temp = TIME_Get();
    if (temp >= meiosis)
    {
        temp = temp - meiosis;
    }
    else
    {
        temp = 0xFFFFFFFFU - meiosis + temp;
    }
    return temp;
}

uint8_t LED_Lighting_Update(LED_Config_t *led);
uint8_t LED_AlternatingBlinking_Update(LED_Config_t *led);
uint8_t LED_Blinking_Update(LED_Config_t *led);
uint8_t LED_Breathing_Update(LED_Config_t *led);
uint8_t LED_Running_Update(LED_Config_t *led);
uint8_t LED_MARQUEE_Update(LED_Config_t *led);

// LED更新函数
uint8_t LED_Update(LED_Config_t *led)
{
    uint8_t ret;
    switch (led->state)
    {
    case LED_LIGHT:
        ret = LED_Lighting_Update(led);
        break;
#if (ALTERNATING_BLINK_ENABLE)
    case LED_ALTERNATING_BLINK:
        ret = LED_AlternatingBlinking_Update(led);
        break;
#endif
#if (BLINK_ENABLE)
    case LED_BLINKING:
        ret = LED_Blinking_Update(led);
        break;
#endif
#if (BREATH_ENABLE)
    case LED_BREATHING:
        ret = LED_Breathing_Update(led);
        break;
#endif
#if (RUNNING_ENABLE)
    case LED_RUNNING:
        ret = LED_Running_Update(led);
        break;
#endif
#if (MARQUEE_ENABLE)
    case LED_MARQUEE:
        ret = LED_MARQUEE_Update(led);
        break;
#endif
    default:
        break;
    }
    return ret;
}

// LED常亮状态更新函数
static uint8_t LED_Lighting_Update(LED_Config_t *led)
{
    // 以1s为周期计算已经进行了多少次常亮
    uint16_t blinkCount = GetTickDiff(led->currentTime) / 1000;
    // 判断是否大于运行次数
    if (led->runNum > 0 && blinkCount >= led->runNum)
    {
        LED_SetState(led->ledMask, LED_OFF); // 关闭LED
        // 达到指定次数后停止闪烁
        return 0;
    }

    LED_SetState(led->ledMask, LED_ON); // 常亮
    return 1;
}

// LED闪烁状态更新函数
#if (BLINK_ENABLE)
static uint8_t LED_Blinking_Update(LED_Config_t *led)
{
    // 周期时间
    uint32_t run_cycle = led->blinkParams.onTime + led->blinkParams.offTime;
    // 计算已经进行了多少次闪烁
    uint16_t blinkCount = GetTickDiff(led->currentTime) / run_cycle;
    // 计算当前时间在周期中的位置
    uint32_t sec_left = GetTickDiff(led->currentTime) % run_cycle;
    // 判断是否大于运行次数
    if (led->runNum > 0 && blinkCount >= led->runNum)
    {

        LED_SetState(led->ledMask, LED_OFF); // 关闭LED
        // 达到指定次数后停止闪烁
        return 0;
    }

    // 判断当前时间是否在一个周期内的亮灭时间段内
    if (sec_left <= led->blinkParams.onTime)
    {
        LED_SetState(led->ledMask, led->blinkParams.orDer ? LED_OFF : LED_ON); // 先灭后亮
    }
    else if (sec_left <= run_cycle)
    {
        LED_SetState(led->ledMask, led->blinkParams.orDer ? LED_ON : LED_OFF); // 先亮后灭
    }
    return 1;
}
#endif

// LED交替闪烁状态更新函数
#if (ALTERNATING_BLINK_ENABLE)
static uint8_t LED_AlternatingBlinking_Update(LED_Config_t *led)
{
    // 计算已经进行了多少次交替
    uint16_t blinkCount = GetTickDiff(led->currentTime) / led->alternatingBlinkParams.Cycle;
    // 计算当前时间在周期中的位置
    uint32_t sec_left = GetTickDiff(led->currentTime) % led->alternatingBlinkParams.Cycle;

    // 判断是否大于运行次数
    if (led->runNum > 0 && blinkCount >= led->runNum)
    {
        LED_SetState(led->ledMask, LED_OFF); // 关闭LED
        // 达到指定次数后停止闪烁
        return 0;
    }
    // 计算每一组的时间片
    uint32_t timeSlice = led->alternatingBlinkParams.Cycle / ALTERNAT_GROUP;

    // 先关闭所有LED
    LED_SetState(led->ledMask, LED_OFF);

    // 判断当前时间是否在哪一组的亮时间段内
    for (size_t i = 0; i < ALTERNAT_GROUP; i++)
    {
        if (sec_left > (timeSlice * i) && sec_left <= (timeSlice * (i + 1)))
        {
            // 只点亮当前时间片对应的LED组
            LED_SetState(led->alternatingBlinkParams.Group[i], LED_ON);
            break; // 找到对应的时间片后立即退出循环
        }
    }
    return 1;
}
#endif

// LED呼吸状态更新函数
#if (BREATH_ENABLE)
static uint8_t LED_Breathing_Update(LED_Config_t *led)
{
    // 记录占空比
    uint8_t result = 0;

    // 计算增减步数
    uint8_t stepNum = (led->breathParams.maxCycle - led->breathParams.minCycle) / led->breathParams.step;

    // 计算呼吸周期总时间
    uint32_t breathCycleTime = led->breathParams.onTime + led->breathParams.offTime;

    // 计算已经进行了多少次呼吸
    uint16_t breathCount = GetTickDiff(led->currentTime) / breathCycleTime;

    // 计算当前时间在周期中的位置
    uint32_t timeInCycle = GetTickDiff(led->currentTime) % breathCycleTime;

    // 静态变量，用于记录上次更新时间
    static uint32_t lastUpdateTime;

    // 判断是否大于运行次数
    if (led->runNum > 0 && breathCount >= led->runNum)
    {
        led->breathParams.pwmCallback(led->breathParams.minCycle); // 关闭LED
        // 达到指定次数后停止呼吸
        return 0;
    }

    // 根据当前呼吸周期阶段进行占空比调整
    if (led->breathParams.currentCycle >= (led->breathParams.maxCycle * 2))
    {
        led->breathParams.currentCycle = led->breathParams.minCycle;
    }
    else
    {
        // 计算当前阶段每个梯度的时间
        uint32_t stepTime = (led->breathParams.currentCycle >= led->breathParams.maxCycle) ? (led->breathParams.offTime / stepNum) : (led->breathParams.onTime / stepNum);

        // 检查距离上次更新时间是否超过了应该增加的时间
        if (GetTickDiff(lastUpdateTime) >= stepTime)
        {
            // 增加占空比梯度
            led->breathParams.currentCycle += led->breathParams.step;
            // 更新上次更新时间
            lastUpdateTime = TIME_Get();
        }
    }

    // 返回当前占空比
    if (led->breathParams.currentCycle <= led->breathParams.maxCycle)
    {
        result = led->breathParams.currentCycle;
    }
    else
    {
        result = (led->breathParams.maxCycle * 2) - led->breathParams.currentCycle;
    }

    led->breathParams.pwmCallback(result);
    return 1;
}
#endif

// LED流水灯状态更新函数
#if (RUNNING_ENABLE)
static uint8_t LED_Running_Update(LED_Config_t *led)
{
    // 记录 LED 灯的数量
    uint8_t count = 0;
    // 记录 LED 灯的映射
    uint8_t mask = led->ledMask;

    // 获取最低位的 LED 灯
    uint16_t Running_low = (led->ledMask & (~led->ledMask + 1));
    // 计算 LED 灯的数量
    while (mask)
    {
        mask &= (mask - 1);
        count++;
    }

    // 获取当前已经进行了多少次流水
    uint16_t flowCount = GetTickDiff(led->currentTime) / led->runningParams.cycleTime;

    // 判断是否超过运行次数
    if (led->runNum > 0 && flowCount >= led->runNum)
    {
        LED_SetState(led->ledMask, LED_OFF); // 关闭LED
        // 达到指定次数后停止流水
        return 0;
    }

    // 判断是否到达下次流水的间隔时间
    if (GetTickDiff(led->runningParams.lastCycleTime) >= led->runningParams.intervalTime)
    {
        // 判断是否到达 LED 点亮的间隔时间
        if (GetTickDiff(led->runningParams.lastLEDTime) >= led->runningParams.cycleTime)
        {
            // 判断是否已经点亮所有 LED
            if (led->runningParams.numLEDs >= count)
            {
                // 如果已经点亮所有 LED，则将 LED 灭掉并更新时间
                LED_SetState(led->runningParams.currentSite, LED_OFF);
                led->runningParams.lastCycleTime = TIME_Get();
                led->runningParams.numLEDs = 0;
                led->runningParams.currentSite = 0;
            }
            else
            {
                // 更新 LED 灯的数量和位置
                led->runningParams.numLEDs++;
                led->runningParams.currentSite <<= 1;
                led->runningParams.currentSite |= Running_low;
                // 点亮 LED 并更新时间
                LED_SetState(led->runningParams.currentSite, LED_ON);
                led->runningParams.lastLEDTime = TIME_Get();
            }
        }
    }
    return 1;
}
#endif

// LED跑马灯状态更新函数
#if (MARQUEE_ENABLE)
static uint8_t LED_MARQUEE_Update(LED_Config_t *led)
{
    // 记录 LED 灯的数量
    uint8_t count = 0;
    // 记录 LED 灯的映射
    uint8_t mask = led->ledMask;

    // 获取最低位的 LED 灯
    uint16_t Running_low = (led->ledMask & (~led->ledMask + 1));
    // 计算 LED 灯的数量
    while (mask)
    {
        mask &= (mask - 1);
        count++;
    }

    // 获取当前已经进行了多少次跑马
    uint16_t flowCount = GetTickDiff(led->currentTime) / led->marqueeParams.cycleTime;

    // 判断是否超过运行次数
    if (led->runNum > 0 && flowCount >= led->runNum)
    {
        LED_SetState(led->ledMask, LED_OFF); // 关闭LED
        // 达到指定次数后停止跑马
        return 0;
    }

    // 判断是否到达下次跑马的间隔时间
    if (GetTickDiff(led->marqueeParams.lastCycleTime) >= led->marqueeParams.intervalTime)
    {
        // 判断是否到达 LED 点亮的间隔时间
        if (GetTickDiff(led->marqueeParams.lastLEDTime) >= led->marqueeParams.cycleTime)
        {
            // 判断是否已经点亮所有 LED
            if (led->marqueeParams.numLEDs >= count)
            {
                // 如果已经点亮所有 LED，则将 LED 灭掉并更新时间
                LED_SetState(led->marqueeParams.currentSite, LED_OFF);
                led->marqueeParams.lastCycleTime = TIME_Get();
                led->marqueeParams.numLEDs = 0;
                led->marqueeParams.currentSite = 0;
            }
            else
            {
                LED_SetState(led->marqueeParams.currentSite, LED_OFF);
                // 更新 LED 灯的数量和位置
                led->marqueeParams.numLEDs++;
                if (led->marqueeParams.currentSite == 0)
                {
                    led->marqueeParams.currentSite = Running_low;
                }
                else
                {
                    led->marqueeParams.currentSite <<= 1;
                }
                // 点亮 LED 并更新时间
                LED_SetState(led->marqueeParams.currentSite, LED_ON);
                led->marqueeParams.lastLEDTime = TIME_Get();
            }
        }
    }
    return 1;
}
#endif
