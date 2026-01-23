/**
 * ***********************************************************
 * @file key.c
 * @author cyWU
 * @brief 按键应用框架
 * @version 0.2
 * @date 2024-01-29
 * @copyright 版权所有 2024
 * ***********************************************************
 */

#include "key.h"
#include <math.h>

uint32_t keyCountTime;           // 计时器
uint8_t keyNum = KEY_NUM;        // 按键数量
keyCategory_t keyTable[KEY_NUM]; // 按键数组

/**
 * @brief 判断按键是否被按下
 * @param [in] key :按键状态机全局结构指针
 * @return :  true,按键状态读取成功；
 *            false,按键未使能；
 */
static bool getKeyLevel(keyFSM_t *key)
{
    if (key->keyShield == KEY_DISABLE)
        return false;

    if (key->keyReadValue() == key->keyDownLevel)
    {
        key->keyLevel = Bit_SET;
    }
    else
    {
        key->keyLevel = Bit_RESET;
    }
    return true;
}

/**
 * @brief 判断按键是否单次或多次被按下
 * @param [in] click :按键状态机全局结构指针
 * @return : true,如果按键间隔时间超过，返回事件；
 *           false,如果按键不超过按键间隔，则不返回事件；
 */
static bool keyClickFrequency(keyFSM_t *click)
{
    click->keyInterval++;
    switch (click->keyFrequency)
    {
    case CLICK:
        if (click->keyInterval >= (KEY_INTERVAL / (DEBOUNCE_TIME * KEY_TIMER_MS)))
        {
            click->eventType = SHORT_Event; // 单击事件
            return true;
        }
        break;
    case DbLCLICK:
        if (click->keyInterval >= (KEY_INTERVAL / (DEBOUNCE_TIME * KEY_TIMER_MS)))
        {
            click->eventType = DBCL_Event; // 双击事件
            return true;
        }
        break;
    /*
    根据需要添加更多的点击事件
    case XXXX:
        if (click->keyInterval >= (KEY_INTERVAL / (DEBOUNCE_TIME * KEY_TIMER_MS)))
        {
            click->eventType = XXXXXX;
            return true;
        }
        break;
    */
    default:
        return true;
    }
    return false;
}

/**
 * @brief 读取按键值
 * @param [in] Key_Buf :按键状态机全局结构指针
 * @return : true,按键值获取成功；
 *           false,按键未使能；
 */
static bool readKeyStatus(keyFSM_t *Key_Buf)
{

    if (!getKeyLevel(Key_Buf))
        return false;

    switch (Key_Buf->keyStatus)
    {
    // 状态 0: 未按下按键
    case KEY_NULL:
        if (Key_Buf->keyLevel == Bit_SET) // 按键按下
        {
            Key_Buf->keyStatus = KEY_SURE;
        }
        break;
    // 状态 1: 确认按下
    case KEY_SURE:
        if (Key_Buf->keyLevel == Bit_SET) // 确认按下按键
        {
            Key_Buf->eventType = DOWN_Event; // 触发按下事件
            Key_Buf->keyStatus = KEY_DOWN;
            Key_Buf->keyCount = 0; // 重置按键计数值
            Key_Buf->keyLongFlag = true;
        }
        else
        {
            Key_Buf->keyStatus = KEY_NULL;
        }
        break;
    // 状态 2: 按下按键
    case KEY_DOWN:
        if (Key_Buf->keyLevel != Bit_SET) // 松开按键
        {
            if (Key_Buf->keyShortFlag == false) // 按下按键置位标志位
            {
                Key_Buf->keyShortFlag = true;
                Key_Buf->keyFrequency++;  // 按键次数加 1
                Key_Buf->keyInterval = 0; // 按键间隔清零
            }
            if (keyClickFrequency(Key_Buf)) // 多击判断
            {
                Key_Buf->keyFrequency = 0;
                Key_Buf->keyStatus = KEY_NULL;
            }
            if ((Key_Buf->keyCount >= Key_Buf->keyLongTime / DEBOUNCE_TIME) &&
                (Key_Buf->keyCount < Key_Buf->keyLastTime / DEBOUNCE_TIME))
            { // 按键长按释放
                Key_Buf->keyFrequency = 0;
                Key_Buf->keyStatus = KEY_NULL;
                Key_Buf->eventType = RELEASE_Event; // 触发长按释放事件
            }
            Key_Buf->keyCount = 0; // 重置按键计数器
            Key_Buf->keyLongFlag = true;
        }
        else
        {
            if ((++Key_Buf->keyCount >= Key_Buf->keyLastTime / DEBOUNCE_TIME))
            {                          // 超过设定时间没有释放
                Key_Buf->keyCount = 0; // 重置按键计数器
                Key_Buf->keyFrequency = 0;
                Key_Buf->keyStatus = KEY_LONG;
                Key_Buf->eventType = LAST_Event; // 触发持续按事件
                Key_Buf->keyLongFlag = true;
            }
            if ((Key_Buf->keyCount >= Key_Buf->keyLongTime / DEBOUNCE_TIME) &&
                (Key_Buf->keyCount < Key_Buf->keyLastTime / DEBOUNCE_TIME))
            { // 按下按键达到设定时间
                Key_Buf->keyFrequency = 0;
                if (Key_Buf->keyLongFlag == true)
                {
                    Key_Buf->eventType = LONG_Event; // 触发长按事件
                    Key_Buf->keyLongFlag = false;
                }
            }
            Key_Buf->keyShortFlag = false; // 按下按键置位标志位
        }
        break;
    // 状态 3: 持续按键长按状态
    case KEY_LONG:
        if (Key_Buf->keyLevel != Bit_SET) // 松开按键
        {
            Key_Buf->keyStatus = KEY_NULL;
            Key_Buf->eventType = RELEASE_Event; // 触发长按释放事件
        }
        break;
    default:
        break;
    }
    return true;
}

/**
 * @brief 按键事件处理函数
 * @param  None
 * @retval None
 */
void keyEventProcess(void)
{
    for (size_t i = 0; i < keyNum; i++)
    {
        if (!readKeyStatus(&keyTable[i].fsm))
            continue;
    }
}

/**
 * @brief 按键处理函数
 * @param  None
 * @retval None
 */
void keyCheckProcess(void)
{
    keyCountTime++;
    if (keyCountTime >= (DEBOUNCE_TIME / KEY_TIMER_MS))
    {
        keyCountTime = 0;
        keyEventProcess();
    }
}

/**
 * @brief 按键参数初始化函数
 * @param [in] keys :按键全局结构指针
 * @return none
 */
void keyParaInit(keyCategory_t *keys)
{
    if (NULL == keys)
    {
        return;
    }
    if (KEY_NUM >= KEY_MAX_NUMBER)
    {
        keyNum = KEY_MAX_NUMBER;
    }

    memcpy(keyTable, keys, sizeof(keyCategory_t) * keyNum);
}

/**
 * @brief 设置按键事件参数
 * @param [in] key_event  按键事件索引（枚举值）
 * @param [in] key_param  按键参数结构体，包含长按时间和持续按时间等信息
 * @return none
 */
void setKeyEventParams(uint8_t key_event, keyCategory_t key_param)
{
    keyTable[key_event].fsm.keyShield = key_param.fsm.keyShield;     // 设置长按时间
    keyTable[key_event].fsm.keyLongTime = key_param.fsm.keyLongTime; // 设置长按时间
    keyTable[key_event].fsm.keyLastTime = key_param.fsm.keyLastTime; // 设置持续按时间
    keyTable[key_event].fsm.keyStatus = KEY_NULL;                    // 复位按键状态
    keyTable[key_event].fsm.eventType = NULL_Event;                  // 清除事件类型
    keyTable[key_event].fsm.keyCount = 0;                            // 复位按键计数
}

/**
 * @brief 按键处理函数
 * @param  None
 * @retval None
 */
void keyHandle(void)
{
    for (size_t i = 0; i < keyNum; i++)
    {
        if (keyTable[i].fsm.eventType == NULL_Event)
            continue;

        switch (keyTable[i].fsm.eventType)
        {
        case RELEASE_Event:
            if (keyTable[i].func.releasePressCb == NULL)
                break;
            keyTable[i].func.releasePressCb();
            keyTable[i].fsm.eventType = NULL_Event;
            break;
        case SHORT_Event:
            if (keyTable[i].func.ShortPressCb == NULL)
                break;
            keyTable[i].func.ShortPressCb();
            keyTable[i].fsm.eventType = NULL_Event;
            break;
        case DOWN_Event:
            if (keyTable[i].func.downPressCb == NULL)
                break;
            keyTable[i].func.downPressCb();
            keyTable[i].fsm.eventType = NULL_Event;
            break;
        case LONG_Event:
            if (keyTable[i].func.longPressCb == NULL)
                break;
            keyTable[i].func.longPressCb();
            keyTable[i].fsm.eventType = NULL_Event;
            break;
        case LAST_Event:
            if (keyTable[i].func.lastPressCb == NULL)
                break;
            keyTable[i].func.lastPressCb();
            // 持续按事件需要一直执行不需要复位事件
            // keyTable[i].fsm.eventType = NULL_Event;
            break;
        case DBCL_Event:
            if (keyTable[i].func.dbclPressCb == NULL)
                break;
            keyTable[i].func.dbclPressCb();
            keyTable[i].fsm.eventType = NULL_Event;
            break;
        default:
            break;
        }
        /*
        为什么要把keyTable[i].fsm.eventType = NULL_Event放在每个事件里而不是在这里统一赋值，是因为遇到以下情况：
        当我持续按的时候松手一直不会触发释放事件，经过排查，是在处理完持续按事件的处理函数之后到了这里刚好进入定时器，
        eventType变成了释放事件，而释放事件在这里会被复位掉，所以一直不会运释放事件处理函数
        所以把keyTable[i].fsm.eventType = NULL_Event全部放到各自的事件中，避免发生这种情况
        if (keyTable[i].fsm.eventType != LAST_Event)
        {   // 持续按事件需要一直执行不需要复位事件
            keyTable[i].fsm.eventType = NULL_Event;
        }
        */
    }
}

void reset_key_Status(keyList key_index)
{
    keyTable[key_index].fsm.keyStatus = KEY_NULL;
    keyTable[key_index].fsm.eventType = NULL_Event;
    keyTable[key_index].fsm.keyCount = 0;
}
