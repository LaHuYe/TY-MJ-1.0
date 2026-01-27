/**
 * ***********************************************************
 * @file addr_rx.h
 * @author cyWU
 * @brief 地址接收（单线通信协议）
 * @version 0.1
 * @date 2026-01-19
 * @copyright Copyright (c) 2026
 * ***********************************************************
 */

#ifndef ADDR_RX_H
#define ADDR_RX_H

#include "main.h"
#include <stdbool.h>

/**
 * @brief 初始化地址接收
 */
void addr_rx_init(void);

/**
 * @brief 反初始化地址接收
 */
void addr_rx_deinit(void);

/**
 * @brief 地址接收解码函数（100us调用一次，需要在定时器中断中调用）
 */
void addr_rx_decode(void);

/**
 * @brief 获取接收到的地址
 * @param addr 输出参数，用于存储地址（2字节数组）
 * @return true 表示地址有效，false 表示地址无效
 */
bool addr_rx_get_received_address(uint8_t *addr);

#endif
