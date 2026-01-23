/**
 * ***********************************************************
 * @file key.h
 * @author cyWU
 * @brief 按键应用框架
 * @version 0.2
 * @date 2024-01-29
 * @copyright Copyright (c) 2024
 * ***********************************************************
 */

#ifndef __KEY_H
#define __KEY_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define KEY_TIMER_MS   1  // 定时器周期（毫秒）
#define KEY_MAX_NUMBER 12 // 最大支持按键数量
#define DEBOUNCE_TIME  30 // 消抖时间（毫秒）
#define KEY_INTERVAL   30 // 按键间隔时间（毫秒）

#define CLICK    1 // 按键单击
#define DbLCLICK 2 // 按键双击

typedef enum
{
    KEY_POWER,        // 电源按键
    KEY_POWER_INSERT, // 充电器插入
    KEY_FULL,         // 充满按键
    KEY_NUM,          // 按键数量，必须放在注册表的底部
} keyList;

typedef enum
{
    Bit_RESET = 0,
    Bit_SET
} BitAction_t;

/* 按键状态枚举 */
typedef enum
{
    KEY_NULL,    // 按键无动作
    KEY_RELEASE, // 按键释放
    KEY_SURE,    // 按键抖动消除
    KEY_UP,      // 按键释放
    KEY_DOWN,    // 按键按下
    KEY_LONG,    // 按键长按
} keyStatus_t;

/* 按键事件枚举 */
typedef enum
{
    NULL_Event,    // 空事件
    DOWN_Event,    // 按下事件
    SHORT_Event,   // 短按事件
    LONG_Event,    // 长按事件
    LAST_Event,    // 连按事件
    DBCL_Event,    // 双击事件
    RELEASE_Event, // 释放事件
} keyEvent_t;

typedef enum
{
    KEY_DISABLE = 0,
    KEY_ENABLE = !KEY_DISABLE
} keyEnable_t;

/* 按键状态机结构体 */
__packed typedef struct
{
    bool keyShortFlag;             // 按键短按标志
    bool keyLongFlag;              // 按键长按标志
    uint8_t keyInterval;           // 按键间隔时间
    uint8_t keyFrequency;          // 按键按下次数
    uint16_t keyLongTime;          // 按键长按时间
    uint16_t keyLastTime;          // 按键连按间隔时间
    uint32_t keyCount;             // 长按计数
    keyEnable_t keyShield;         // 按键使能状态
    BitAction_t keyLevel;          // 判断按键是否按下，按下: 1，松开: 0
    BitAction_t keyDownLevel;      // 按键按下时 IO 端口的电平状态
    keyStatus_t keyStatus;         // 按键当前状态
    keyEvent_t eventType;          // 按键事件类型
    uint8_t (*keyReadValue)(void); // 按键读取值函数指针
} keyFSM_t;

/* 不同事件对应的回调函数 */
__packed typedef struct
{
    void (*nullPressCb)(void);    // 无动作事件回调函数
    void (*releasePressCb)(void); // 释放事件回调函数
    void (*downPressCb)(void);    // 按下事件回调函数
    void (*ShortPressCb)(void);   // 短按事件回调函数
    void (*longPressCb)(void);    // 长按事件回调函数
    void (*lastPressCb)(void);    // 连按事件回调函数
    void (*dbclPressCb)(void);    // 双击事件回调函数
} keyFunc_t;

/* 按键类结构体 */
__packed typedef struct
{
    keyFSM_t fsm;   // 按键状态机
    keyFunc_t func; // 按键事件回调函数
} keyCategory_t;

void keyParaInit(keyCategory_t *keys);                              // 按键参数初始化函数
void setKeyEventParams(uint8_t key_event, keyCategory_t key_param); // 设置按键事件参数
void keyCheckProcess(void);                                         // 按键检测处理函数
void keyHandle(void);                                               // 按键事件处理函数
void reset_key_Status(keyList key_index);                           // 重置按键状态
#endif
