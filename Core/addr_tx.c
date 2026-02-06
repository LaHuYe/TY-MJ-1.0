/**
 * ***********************************************************
 * @file addr_tx.c
 * @author cyWU
 * @brief 地址发送（单线通信协议，参考EV1527实现）
 * @version 0.1
 * @date 2026-01-20
 * @copyright Copyright (c) 2026
 * ***********************************************************
 */
#include "addr_tx.h"

// 定时周期（与EV1527一致）
#define TIME_CYCLE 100 // 100us

// 同步码时序（100us 定时周期）
#define SYNC_HIGH_DURATION (9000 / TIME_CYCLE) // 同步码高电平：9ms = 90 个周期
#define SYNC_LOW_DURATION  (9000 / TIME_CYCLE) // 同步码低电平：9ms = 90 个周期

// 数据位时序（100us 定时周期）
#define BIT0_HIGH_DURATION (400 / TIME_CYCLE)  // bit0 高电平：400us = 4 个周期
#define BIT0_LOW_DURATION  (800 / TIME_CYCLE)  // bit0 低电平：800us = 8 个周期
#define BIT1_HIGH_DURATION (1000 / TIME_CYCLE) // bit1 高电平：1ms = 10 个周期
#define BIT1_LOW_DURATION  (200 / TIME_CYCLE)  // bit1 低电平：200us = 2 个周期

// 地址长度：4字节（32位）
#define ADDR_SIZE 4

// 发送重复次数
#define TX_REPEAT_COUNT 3

// 定义发送状态枚举
typedef enum
{
    TX_IDLE,      // 空闲状态（等待发送使能）
    TX_SYNC_HIGH, // 同步码高电平状态
    TX_SYNC_LOW,  // 同步码低电平状态
    TX_BIT_HIGH,  // 数据位高电平状态
    TX_BIT_LOW,   // 数据位低电平状态
} Addr_Tx_State_t;

// 全局变量
static Addr_Tx_State_t tx_state = TX_IDLE; // 发送状态
static bool tx_enable = false;             // 发送使能标志
static uint8_t tx_buffer[ADDR_SIZE] = {0}; // 发送地址缓冲区
static uint32_t tx_data = 0;               // 要发送的数据（32位）
static uint8_t tx_bit_count = 0;           // 已发送数据位计数
static uint32_t tx_duration_count = 0;     // 当前状态持续时间计数
static uint32_t tx_high_duration = 0;      // 当前数据位高电平持续时间
static uint32_t tx_low_duration = 0;       // 当前数据位低电平持续时间
static uint8_t tx_repeat_done = 0;         // 已完成的重复发送次数

/**
 * @brief 重置发送参数
 */
static void reset_tx_parameters(void)
{
    tx_state = TX_IDLE;
    tx_enable = false;
    tx_bit_count = 0;
    tx_duration_count = 0;
    tx_data = 0;
    tx_high_duration = 0;
    tx_low_duration = 0;
    tx_repeat_done = 0;
}

/**
 * @brief 设置GPIO输出电平
 * @param level GPIO_PIN_SET 或 GPIO_PIN_RESET
 */
static void set_tx_pin(GPIO_PinState level)
{
    HAL_GPIO_WritePin(HOST_TX_GPIO_PORT, HOST_TX_PIN, level);
}

/**
 * @brief 根据当前位设置高低电平持续时间
 */
static void set_current_bit_duration(void)
{
    uint8_t bit_position = 31 - tx_bit_count; // 从最高位开始（32位数据，最高位是bit 31）
    uint8_t current_bit = (tx_data >> bit_position) & 0x01;

    if (current_bit == 0)
    {
        // bit0：400us 高电平 + 800us 低电平
        tx_high_duration = BIT0_HIGH_DURATION;
        tx_low_duration = BIT0_LOW_DURATION;
    }
    else
    {
        // bit1：1ms 高电平 + 200us 低电平
        tx_high_duration = BIT1_HIGH_DURATION;
        tx_low_duration = BIT1_LOW_DURATION;
    }
}

/**
 * @brief 地址发送初始化函数（初始化GPIO）
 */
void addr_tx_init(void)
{
    // 使能GPIO时钟
    HOST_TX_GPIO_CLK_ENABLE();

    // 配置GPIO为推挽输出模式
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = HOST_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(HOST_TX_GPIO_PORT, &GPIO_InitStruct);

    // 初始输出低电平
    set_tx_pin(GPIO_PIN_RESET);

    // 初始化状态
    reset_tx_parameters();

    appPrintf(LOG_DEBUG, "Address TX initialized\r\n");
}

/**
 * @brief 地址发送处理函数（100us调用一次，需要在定时器中断中调用）
 */
void addr_tx_process(void)
{
    // 如果未使能，直接返回
    if (!tx_enable && tx_state == TX_IDLE)
    {
        return;
    }

    switch (tx_state)
    {
    case TX_IDLE: // 空闲状态（等待发送使能）
        if (tx_enable)
        {
            // 开始发送，进入同步码高电平状态
            tx_state = TX_SYNC_HIGH;
            tx_duration_count = 0;
            set_tx_pin(GPIO_PIN_SET);   // 输出高电平
            tx_bit_count = 0;           // 重置位计数
            tx_repeat_done = 0;         // 重置重复次数
            set_current_bit_duration(); // 预置首位时序
        }
        break;

    case TX_SYNC_HIGH: // 同步码高电平状态
        tx_duration_count++;
        if (tx_duration_count >= SYNC_HIGH_DURATION)
        {
            // 同步码高电平时间到，进入同步码低电平状态
            tx_state = TX_SYNC_LOW;
            tx_duration_count = 0;
            set_tx_pin(GPIO_PIN_RESET); // 输出低电平
        }
        break;

    case TX_SYNC_LOW: // 同步码低电平状态
        tx_duration_count++;
        if (tx_duration_count >= SYNC_LOW_DURATION)
        {
            // 同步码低电平时间到，准备发送数据位
            tx_state = TX_BIT_HIGH;
            tx_duration_count = 0;
            set_current_bit_duration(); // 发送前确保首位时序
            set_tx_pin(GPIO_PIN_SET);   // 输出高电平，开始发送第一个数据位
        }
        break;

    case TX_BIT_HIGH: // 数据位高电平状态
        tx_duration_count++;
        if (tx_duration_count >= tx_high_duration)
        {
            // 数据位高电平时间到，进入数据位低电平状态
            tx_state = TX_BIT_LOW;
            tx_duration_count = 0;
            set_tx_pin(GPIO_PIN_RESET); // 输出低电平
        }
        break;

    case TX_BIT_LOW: // 数据位低电平状态
        tx_duration_count++;
        if (tx_duration_count >= tx_low_duration)
        {
            // 数据位低电平时间到，处理当前数据位
            tx_bit_count++;

            // 检查是否所有数据位已发送完成
            if (tx_bit_count >= (ADDR_SIZE * 8))
            {
                // 一帧发送完成
                tx_repeat_done++;
                if (tx_repeat_done >= TX_REPEAT_COUNT)
                {
                    // 达到重复次数，结束发送
                    appPrintf(LOG_DEBUG, "Address TX completed (x%d): 0x%02X%02X%02X%02X\r\n",
                              TX_REPEAT_COUNT, tx_buffer[0], tx_buffer[1], tx_buffer[2], tx_buffer[3]);
                    set_tx_pin(GPIO_PIN_SET); // 保持高电平
                    reset_tx_parameters();      // 重置发送参数
                }
                else
                {
                    // 继续下一帧发送
                    tx_bit_count = 0;
                    tx_duration_count = 0;
                    set_current_bit_duration(); // 预置下一帧首位
                    tx_state = TX_SYNC_HIGH;
                    set_tx_pin(GPIO_PIN_SET); // 输出高电平，开始下一帧同步码
                }
            }
            else
            {
                // 还有数据位未发送，准备下一个数据位
                set_current_bit_duration(); // 计算下一个数据位时序
                // 继续发送下一个数据位
                tx_state = TX_BIT_HIGH;
                tx_duration_count = 0;
                set_tx_pin(GPIO_PIN_SET); // 输出高电平，开始发送下一个数据位
            }
        }
        break;

    default:
        reset_tx_parameters(); // 默认状态，重置发送参数
        break;
    }
}

/**
 * @brief 使能地址发送
 * @param addr 要发送的地址（4字节数组，32位）
 * @param len 地址长度（应至少为4）
 * @return true 表示使能成功，false 表示发送忙（上一次发送未完成）
 */
bool addr_tx_enable(const uint8_t *addr, uint8_t len)
{
    if (addr == NULL)
    {
        return false;
    }

    // 检查是否正在发送
    if (tx_enable || tx_state != TX_IDLE)
    {
        // 上一次发送未完成，返回失败
        return false;
    }
    // 保存要发送的地址
    for (size_t i = 0; i < len; i++)
    {
        tx_buffer[i] = addr[i];
    }

    // 组合成32位数据（高字节在前，大端序）
    tx_data = ((uint32_t)tx_buffer[0] << 24) |
              ((uint32_t)tx_buffer[1] << 16) |
              ((uint32_t)tx_buffer[2] << 8) |
              (uint32_t)tx_buffer[3];

    // 使能发送
    tx_enable = true;
    tx_bit_count = 0;           // 重置位计数
    set_current_bit_duration(); // 预置首位时序

    appPrintf(LOG_DEBUG, "Address TX enabled (x%d): 0x%02X%02X%02X%02X\r\n",
              TX_REPEAT_COUNT, tx_buffer[0], tx_buffer[1], tx_buffer[2], tx_buffer[3]);

    return true;
}
