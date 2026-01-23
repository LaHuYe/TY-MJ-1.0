/**
 * @file led.h
 * @author cyWu (1917507415@qq.com)
 * @brief LED应用层框架
 * @version 0.1
 * @date 2024-02-22
 * @copyright Copyright (c) 2024
 *
 */

#ifndef LED_H
#define LED_H

#include <main.h>
#include <stdint.h>
#include <string.h>

#define BLINK_ENABLE             1
#define ALTERNATING_BLINK_ENABLE 1
#define BREATH_ENABLE            1
#define RUNNING_ENABLE           0
#define MARQUEE_ENABLE           1
// 定义LED开关状态
#define LED_ON  1
#define LED_OFF 0

// 定义LED通道宏
#define LED_CHANNEL(user_led) (1 << (user_led))

// LED状态枚举
typedef enum
{
    LED_LIGHT,             // 常亮状态
    LED_BLINKING,          // 闪烁状态
    LED_ALTERNATING_BLINK, // 交替闪烁模式
    LED_BREATHING,         // 呼吸状态
    LED_RUNNING,           // 流水灯状态
    LED_MARQUEE            // 跑马灯状态
} LED_State_t;

// LED事件枚举
typedef enum
{
    LED_EVENT_NONE = 0,  // 无事件（默认状态）
    LED_EVENT_STANDBY,   // 电量正常时，待机LED绿色常亮事件
    LED_EVENT_LOW_POWER, // 电量低时，待机LED红色常亮事件
    LED_EVENT_LOW_WARN,  // 电量警告时，待机LED绿色闪烁事件
    LED_EVENT_KEY_PRESS, // 按键事件
    LED_EVENT_CHARGING,  // 充电事件
    LED_EVENT_FULL,      // 充满事件
    LED_EVENT_MAX,       // 事件数量（用于边界检查）
} LED_Event_t;

// LED事件优先级枚举
typedef enum
{
    PRIORITY_1,
    PRIORITY_2,
    // PRIORITY_3,
    // PRIORITY_4,
    // PRIORITY_5,
    PRIORITY_MAX // 优先级枚举的最大值，用于数组大小
} LED_EventPriority_t;

#if (BLINK_ENABLE)
// LED 闪烁参数结构体
__packed typedef struct
{
    uint8_t orDer;    // 用于判断先开还是先关，1：先灭后亮，0：先亮后灭
    uint32_t onTime;  // 亮时间（毫秒）
    uint32_t offTime; // 灭时间（毫秒）
} LED_BlinkParams_t;
#endif

#if (ALTERNATING_BLINK_ENABLE)
#define ALTERNAT_GROUP 2
// LED交替闪烁参数结构体
__packed typedef struct
{
    uint16_t Group[ALTERNAT_GROUP]; // 交替组(2组)
    uint32_t Cycle;                 // 交替周期时间（毫秒）
} LED_AlternatingBlinkParams_t;

#endif

#if (BREATH_ENABLE)
// LED 呼吸参数结构体
__packed typedef struct
{
    uint8_t maxCycle;                  // 最大占空比（0~100%）
    uint8_t minCycle;                  // 最小占空比（0~100%）
    uint8_t currentCycle;              // 当前占空比（0~100%）
    uint8_t step;                      // 增减梯度
    uint32_t onTime;                   // 呼吸亮时间（毫秒）
    uint32_t offTime;                  // 呼吸灭时间（毫秒）
    void (*pwmCallback)(uint8_t Duty); // PWM回调函数
} LED_BreathParams_t;
#endif

#if (RUNNING_ENABLE)
// LED 流水灯参数结构体
__packed typedef struct
{
    uint32_t cycleTime;     // 流水灯周期时间（毫秒）
    uint32_t intervalTime;  // 下次运行的间隔时间（毫秒）
    uint32_t lastCycleTime; // 上次周期运行完的记录时间戳
    uint32_t lastLEDTime;   // 上个 LED 点亮的记录时间戳
    uint32_t currentSite;   // 流水灯的当前位置
    uint32_t numLEDs;       // 流水灯的 LED 数量
} LED_RunningParams_t;
#endif

#if (MARQUEE_ENABLE)
// LED 跑马灯参数结构体
__packed typedef struct
{
    uint32_t cycleTime;     // 跑马灯周期时间（毫秒）
    uint32_t intervalTime;  // 下次运行的间隔时间（毫秒）
    uint32_t lastCycleTime; // 上次周期运行完的记录时间戳
    uint32_t lastLEDTime;   // 上个 LED 点亮的记录时间戳
    uint32_t currentSite;   // 跑马灯的当前位置
    uint32_t numLEDs;       // 跑马灯的 LED 数量
} LED_MarqueeParams_t;
#endif

// LED 配置结构体
__packed typedef struct
{
    LED_State_t state;    // LED状态
    uint16_t ledMask;     // LED掩码
    uint16_t runNum;      // 运行次数（0表示无限次）
    uint32_t currentTime; // 运行时的时间戳
#if (BLINK_ENABLE)
    LED_BlinkParams_t blinkParams; // 闪烁参数
#endif
#if (BREATH_ENABLE)
    LED_BreathParams_t breathParams; // 呼吸参数
#endif
#if (RUNNING_ENABLE)
    LED_RunningParams_t runningParams; // 流水灯参数
#endif
#if (ALTERNATING_BLINK_ENABLE)
    LED_AlternatingBlinkParams_t alternatingBlinkParams; // 交替闪烁参数
#endif
#if (MARQUEE_ENABLE)
    LED_MarqueeParams_t marqueeParams; // 跑马灯参数
#endif
} LED_Config_t;

// LED事件表项结构体
__packed typedef struct
{
    LED_Event_t event;                                // 事件
    LED_EventPriority_t priority;                     // 优先级
    LED_Config_t config;                              // LED配置
    uint8_t (*ledEventHandler)(LED_Config_t *config); // LED事件处理回调函数
} LED_EventTableItem_t;

// LED状态设置函数
typedef void (*LED_SetState_t)(uint16_t led, uint8_t state);
// 获取时间戳函数
typedef uint32_t (*TIME_Get_t)(void);

// 设置led状态回调函数
void LED_SetStateInit(LED_SetState_t setState);
// 设置获取时间戳回调函数
void SET_TimeGetInit(TIME_Get_t getTime);
// LED初始化函数
void LED_EventTableInit(LED_EventTableItem_t ledEvent[]);
// 将LED事件加入事件表
void LED_EventAdd(LED_Event_t ledEvent);
// 将LED事件从事件表删除
void LED_EventDelete(LED_Event_t ledEvent);
// 将所有LED事件从事件表删除
void LED_EventAllDelete(void);
// LED事件处理函数
void LED_EventHandle(void);
// 判断是否有 LED 事件正在运行
uint8_t LED_IsEventActive(void);
// 查询LED事件表中是否存在指定事件
uint8_t LED_IsEventExist(LED_Event_t ledEvent);
// LED更新函数
uint8_t LED_Update(LED_Config_t *led);

#endif // LED_H
