/**
 * ***********************************************************
 * @file addr_rx.c
 * @author cyWU
 * @brief 地址接收（单线通信协议，参考EV1527实现）
 * @version 0.1
 * @date 2026-01-19
 * @copyright Copyright (c) 2026
 * ***********************************************************
 */
#include "addr_rx.h"
#include "log.h"
#include "main.h"
#include <stdbool.h>

// 定时周期（与EV1527一致）
#define TIME_CYCLE 100 // 100us

// 同步码和数据位阈值沿用 EV1527（100us 定时周期）
// 引导码 5.6ms~16ms
#define MIN_LEAD_CODE (5600 / TIME_CYCLE)
#define MAX_LEAD_CODE (16000 / TIME_CYCLE)
// 数据位宽 100us~2400us
#define MIN_BIT_DURATION (100 / TIME_CYCLE)
#define MAX_BIT_DURATION (2400 / TIME_CYCLE)
#define BIT_HIGH_MIN     MIN_BIT_DURATION
#define BIT_HIGH_MAX     MAX_BIT_DURATION
#define BIT_LOW_MIN      MIN_BIT_DURATION
#define BIT_LOW_MAX      MAX_BIT_DURATION

// 地址长度：2字节（16位）
#define ADDR_SIZE 2

// 定义解码状态枚举（与EV1527完全一致）
typedef enum
{
    LEAD_CODE,    // 引导码状态
    HIGH_BIT,     // 高位数据位状态
    LOW_BIT,      // 低位数据位状态
    DATA_PROCESS, // 数据处理状态
} Addr_Decode_State_t;

// 全局变量（与EV1527完全一致）
static Addr_Decode_State_t decode_state = LEAD_CODE; // 解码状态
static uint32_t lead_code_count = 0;                 // 引导码计数
static uint32_t high_bit_count = 0;                  // 高位数据位计数
static uint32_t low_bit_count = 0;                   // 低位数据位计数
static uint32_t high_bit_duration = 0;               // 高位数据位持续时间
static uint32_t low_bit_duration = 0;                // 低位数据位持续时间
static uint8_t received_buffer[ADDR_SIZE] = {0};     // 接收地址缓冲区
static uint8_t bit_count = 0;                        // 接收数据位计数
static uint8_t received_data = 0;                    // 接收到的数据
static uint8_t received_byte_count = 0;              // 接收数据字节计数
static bool address_received = false;                // 地址接收完成标志

/**
 * @brief 重置解码参数（不清空接收缓冲区）
 */
static void reset_decode_parameters(void)
{
    bit_count = 0;
    received_data = 0x00;
    received_byte_count = 0;
    lead_code_count = 0;
    high_bit_count = 0;
    low_bit_count = 0;
    high_bit_duration = 0;
    low_bit_duration = 0;
    decode_state = LEAD_CODE;
    // 注意：不清空 received_buffer 和 address_received，地址会一直保存在数组中
}

/**
 * @brief 解码数据位
 */
static void decode_data_bit(void)
{
    received_data <<= 1;

    // 根据高低电平持续时间判断0和1
    // Bit0: 400us 高 + 800us 低 (高<低)
    // Bit1: 1ms 高 + 200us 低 (高>低)
    if (high_bit_duration > low_bit_duration)
    {
        received_data |= 0x01; // Bit1
    }
    else
    {
        received_data &= 0xFE; // Bit0
    }

    bit_count++;

    // 每接收8位数据，存入数据数组
    if (bit_count == 8)
    {
        received_buffer[received_byte_count] = received_data;
        received_data = 0x00;
        bit_count = 0;
        received_byte_count++;
    }
}

/**
 * @brief 地址接收解码函数（100us调用一次，与EV1527完全一致的逻辑）
 */
void addr_rx_decode(void)
{
    switch (decode_state)
    {
    case LEAD_CODE: // 引导码（与EV1527完全一致）
        // 判断是否低电平
        if (HAL_GPIO_ReadPin(REMOTE_RX_GPIO_PORT, REMOTE_RX_PIN) == GPIO_PIN_RESET)
        {
            lead_code_count++;
        }
        else // 高电平判断范围
        {
            // 判断引导码范围是否合法
            if (lead_code_count >= MIN_LEAD_CODE && lead_code_count <= MAX_LEAD_CODE)
            {
                lead_code_count = 0;
                reset_decode_parameters(); // 重置解码参数
                decode_state = HIGH_BIT;   // 进入高位数据位判断状态
            }
            else
            {
                reset_decode_parameters(); // 引导码范围不合法，重置解码参数
            }
        }
        break;

    case HIGH_BIT: // 高位数据位（与EV1527完全一致）
        // 判断是否高电平
        if (HAL_GPIO_ReadPin(REMOTE_RX_GPIO_PORT, REMOTE_RX_PIN) == GPIO_PIN_SET)
        {
            high_bit_count++;
        }
        else // 低电平判断范围
        {
            // 判断高位数据位范围是否合法
            if (high_bit_count >= MIN_BIT_DURATION && high_bit_count <= MAX_BIT_DURATION)
            {
                high_bit_duration = high_bit_count; // 保存计数值，用于区分0和1
                high_bit_count = 0;
                decode_state = LOW_BIT; // 进入低位数据位判断状态
            }
            else
            {
                reset_decode_parameters(); // 高位数据位范围不合法，重置解码参数
            }
        }
        break;

    case LOW_BIT: // 低位数据位（与EV1527完全一致）
        // 判断是否低电平
        if (HAL_GPIO_ReadPin(REMOTE_RX_GPIO_PORT, REMOTE_RX_PIN) == GPIO_PIN_RESET)
        {
            low_bit_count++;
        }
        else // 高电平判断范围
        {
            // 判断低位数据位范围是否合法
            if (low_bit_count >= MIN_BIT_DURATION && low_bit_count <= MAX_BIT_DURATION)
            {
                low_bit_duration = low_bit_count; // 保存计数值，用于区分0和1
                low_bit_count = 0;
                decode_state = DATA_PROCESS; // 进入数据处理状态
            }
            else
            {
                reset_decode_parameters(); // 低位数据位范围不合法，重置解码参数
            }
        }
        break;

    case DATA_PROCESS:     // 数据处理（与EV1527逻辑一致，但只接收2字节）
        decode_data_bit(); // 解码数据
        if (received_byte_count == ADDR_SIZE)
        {
            // 接收到完整的2字节地址
            address_received = true;
            appPrintf(LOG_DEBUG, "Address received: 0x%02X%02X\r\n",
                      received_buffer[0], received_buffer[1]);

            // 地址已保存在 received_buffer 中，可以通过 addr_rx_get_received_address() 获取
            // 重置解码参数，准备接收下一帧（不清空 received_buffer）
            reset_decode_parameters();
        }
        else
        {
            // 数据没接收完，继续解码数据
            decode_state = HIGH_BIT;
        }
        break;

    default:
        reset_decode_parameters(); // 默认状态，重置解码参数
        break;
    }
}

/**
 * @brief 获取接收到的地址
 * @param addr 输出参数，用于存储地址（2字节数组）
 * @return true 表示地址有效，false 表示地址无效
 */
bool addr_rx_get_received_address(uint8_t *addr)
{
    if (addr == NULL)
    {
        return false;
    }

    // 检查地址是否已接收完成
    if (address_received)
    {
        addr[0] = received_buffer[0];
        addr[1] = received_buffer[1];
        address_received = false;
        return true;
    }

    return false;
}
