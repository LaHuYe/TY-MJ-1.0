/**
 * @file EV1527.c
 * @author cyWu (1917507415@qq.com)
 * @brief EV1527解码框架，定时器中断的方式解码，使用80us的定时器，直接放中断服务函数就可以，适用于所有单片机。
 * @version 0.1
 * @date 2024-03-28
 * @copyright Copyright (c) 2024
 *
 */

#include "EV1527.h"

// 定时周期
#define TIME_CYCLE 100

// 定义引导码的最小和最大持续时间（单位：us）
#define MIN_LEAD_CODE (5600 / TIME_CYCLE)
#define MAX_LEAD_CODE (16000 / TIME_CYCLE)

// 定义数据位持续时间的最小和最大范围（单位：us）
#define MIN_BIT_DURATION (100 / TIME_CYCLE)
#define MAX_BIT_DURATION (2400 / TIME_CYCLE)

// 定义功能字节在接收缓冲区中的索引位置
#define FUNCTION_BYTE_INDEX 2

// 定义功能值
#define FUNCTION_1 0x08
#define FUNCTION_2 0x04
#define FUNCTION_3 0x02
#define FUNCTION_4 0x01

// 定义数据解码状态枚举
typedef enum
{
    LEAD_CODE,       // 引导码状态
    HIGH_BIT,        // 高位数据位状态
    LOW_BIT,         // 低位数据位状态
    DATA_PROCESS,    // 数据处理状态
    FUNCTION_PROCESS // 功能处理状态
} Decode_State_t;

// 定义全局变量和缓冲区
static uint32_t Lead_Code_Count             = 0;   // 引导码计数
static uint32_t High_Bit_Count              = 0;   // 高位数据位计数
static uint32_t Low_Bit_Count               = 0;   // 低位数据位计数
static uint32_t High_Bit_Duration           = 0;   // 高位数据位持续时间
static uint32_t Low_Bit_Duration            = 0;   // 低位数据位持续时间
static uint8_t  Received_Buffer[ARRAY_SIZE] = {0}; // 接收数据缓冲区
static uint8_t  lastDataArray[ARRAY_SIZE]   = {0}; // 上一次接收数据缓冲区
static uint8_t  Received_Byte_Count         = 0;   // 接收数据字节计数
// static uint8_t consecutiveEqualCount = 0;          // 数据接收相同计数
static uint8_t        Bit_Count       = 0;         // 接收数据位计数
static uint8_t        Received_Data   = 0;         // 接收到的数据
static Decode_State_t RF_Decode_State = LEAD_CODE; // 数据解码状态

/**----------------------------------------------------------------------------------------------**
 **函数名  ：EV1527端口配置
 **功能说明：初始化IO口，不同单片机的配置输入模式不一样，自行修改。
 **----------------------------------------------------------------------------------------------**/
void EV1527_Init(void)
{
    DATA_433_GPIO_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct;
    // 配置上拉输入
    GPIO_InitStruct.Pin   = DATA_433_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(DATA_433_GPIO_PORT, &GPIO_InitStruct);
}

/**
 * @brief 比较两个整型数组是否相等
 *
 * 该函数用于比较两个整型数组是否相等。它逐个比较两个数组的元素，
 * 如果所有对应位置上的元素都相等，则返回 true，否则返回 false。
 *
 * @param array1 第一个整型数组
 * @param array2 第二个整型数组
 * @param size 数组的大小
 * @return 如果数组相等返回 true，否则返回 false
 */
bool arraysEqual(uint8_t array1[], uint8_t array2[], uint8_t size)
{
    for (int i = 0; i < size; ++i)
    {
        if (array1[i] != array2[i])
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief 将源数组的值复制到目标数组
 *
 * 该函数将源数组的值复制到目标数组中。它逐个将源数组的元素复制到目标数组的对应位置。
 *
 * @param source 源数组
 * @param destination 目标数组
 * @param size 数组的大小
 */
void copyArray(uint8_t source[], uint8_t destination[], uint8_t size)
{
    for (int i = 0; i < size; ++i)
    {
        destination[i] = source[i];
    }
}

/**----------------------------------------------------------------------------------------------**
 **函数名  ：RF信号解码函数
 **功能说明：解码从433MHz接收到的信号，并根据解码结果执行相应功能
 **调用说明：100us调用一次
 **----------------------------------------------------------------------------------------------**/
void RF_Signal_Decode(void)
{
    switch (RF_Decode_State)
    {
    case LEAD_CODE: // 引导码
        // 判断是否低电平
        if (HAL_GPIO_ReadPin(DATA_433_GPIO_PORT, DATA_433_PIN) == GPIO_PIN_RESET)
        {
            Lead_Code_Count++;
        }
        else // 高电平判断范围
        {
            // 判断引导码范围是否合法
            if (Lead_Code_Count >= MIN_LEAD_CODE && Lead_Code_Count <= MAX_LEAD_CODE)
            {
                Lead_Code_Count = 0;
                Reset_Decode_Parameters();  // 重置解码参数
                RF_Decode_State = HIGH_BIT; // 进入高位数据位判断状态
            }
            else
            {
                Reset_Decode_Parameters(); // 引导码范围不合法，重置解码参数
            }
        }
        break;

    case HIGH_BIT:
        // 判断是否高电平
        if (HAL_GPIO_ReadPin(DATA_433_GPIO_PORT, DATA_433_PIN) == GPIO_PIN_SET)
        {
            High_Bit_Count++;
        }
        else // 低电平判断范围
        {
            // 判断高位数据位范围是否合法
            if (High_Bit_Count >= MIN_BIT_DURATION && High_Bit_Count <= MAX_BIT_DURATION)
            {
                High_Bit_Duration = High_Bit_Count; // 保存计数值，用于区分0和1
                High_Bit_Count    = 0;
                RF_Decode_State   = LOW_BIT; // 进入低位数据位判断状态
            }
            else
            {
                Reset_Decode_Parameters(); // 高位数据位范围不合法，重置解码参数
            }
        }
        break;

    case LOW_BIT:
        // 判断是否低电平
        if (HAL_GPIO_ReadPin(DATA_433_GPIO_PORT, DATA_433_PIN) == GPIO_PIN_RESET)
        {
            Low_Bit_Count++;
        }
        else // 高电平判断范围
        {
            // 判断低位数据位范围是否合法
            if (Low_Bit_Count >= MIN_BIT_DURATION && Low_Bit_Count <= MAX_BIT_DURATION)
            {
                Low_Bit_Duration = Low_Bit_Count; // 保存计数值，用于区分0和1
                Low_Bit_Count    = 0;
                RF_Decode_State  = DATA_PROCESS; // 进入数据处理状态
            }
            else
            {
                Reset_Decode_Parameters(); // 低位数据位范围不合法，重置解码参数
            }
        }
        break;

    case DATA_PROCESS:
        Decode_Data(); // 解码数据
        if (Received_Byte_Count == 3)
        {
            for (size_t i = 0; i < Received_Byte_Count; i++)
            {
                appPrintf(LOG_DEBUG, "Received_Buffer[%d]:%02X \r\n", i, Received_Buffer[i]);
            }
            compareDataArrays(Received_Buffer);
            // 接收到全部数据，包括地址和数据
            Reset_Decode_Parameters(); // 重置解码参数
        }
        else
        {                               // 数据没接收完
            RF_Decode_State = HIGH_BIT; // 继续解码数据
        }
        break;

    case FUNCTION_PROCESS:
        Execute_Function();        // 执行功能
        Reset_Decode_Parameters(); // 重置解码参数
        break;

    default:
        Reset_Decode_Parameters(); // 默认状态，重置解码参数
        break;
    }
}

/**----------------------------------------------------------------------------------------------**
 **函数名  ：Reset_Decode_Parameters
 **功能说明：重置解码参数，用于开始新的解码周期
 **----------------------------------------------------------------------------------------------**/
void Reset_Decode_Parameters(void)
{
    Bit_Count           = 0;
    Received_Data       = 0x00;
    Received_Byte_Count = 0;
    Lead_Code_Count     = 0;
    High_Bit_Count      = 0;
    Low_Bit_Count       = 0;
    High_Bit_Duration   = 0;
    Low_Bit_Duration    = 0;
    RF_Decode_State     = LEAD_CODE;
}

/**----------------------------------------------------------------------------------------------**
 **函数名  ：Decode_Data
 **功能说明：解码数据位，将解码后的数据存入相应的缓冲区中
 **----------------------------------------------------------------------------------------------**/
void Decode_Data(void)
{
    Received_Data <<= 1;
    // 根据高低电平持续时间判断0和1，然后将数据移位存入缓冲区
    if (High_Bit_Duration > Low_Bit_Duration)
    {
        Received_Data |= 0x01;
    }
    else
    {
        Received_Data &= 0xFE;
    }

    Bit_Count++;

    // 每接收8位数据，存入数据数组
    if (Bit_Count == 8)
    {
        Received_Buffer[Received_Byte_Count] = Received_Data;
        Received_Data                        = 0x00;
        Bit_Count                            = 0;
        Received_Byte_Count++;
    }
}

/**----------------------------------------------------------------------------------------------**
 **函数名  compareDataArrays
 **功能说明：将上次数组与本次数组进行比较，如果超过3次相同则数据正确
 **----------------------------------------------------------------------------------------------**/
void EV1527_Handle(uint8_t *DataArray);
void compareDataArrays(uint8_t *currentDataArray)
{
    // if (arraysEqual(lastDataArray, currentDataArray, ARRAY_SIZE))
    // {
    //     consecutiveEqualCount++;
    // }
    // else
    // {
    //     consecutiveEqualCount = 0;
    // }

    copyArray(currentDataArray, lastDataArray, ARRAY_SIZE);
    // if (consecutiveEqualCount >= 3)
    // {
    EV1527_Handle(lastDataArray);
    // consecutiveEqualCount = 0;
    // }
}

/**----------------------------------------------------------------------------------------------**
 **函数名  ：Execute_Function
 **功能说明：执行功能，根据解码后的数据进行相应操作
 **----------------------------------------------------------------------------------------------**/
void Execute_Function(void)
{
    // 判断解码后的功能字节，并执行相应操作
    switch (Received_Buffer[FUNCTION_BYTE_INDEX])
    {
    case FUNCTION_1:
        // 执行功能1
        break;

    case FUNCTION_2:
        // 执行功能2
        break;

    case FUNCTION_3:
        // 执行功能3
        break;

    default:
        // 默认操作
        break;
    }
}
