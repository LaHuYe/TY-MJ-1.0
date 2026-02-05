/**
 * ***********************************************************
 * @file addr_tx.h
 * @author cyWU
 * @brief 地址发送（单线通信协议，参考EV1527实现）
 * @version 0.1
 * @date 2026-01-20
 * @copyright Copyright (c) 2026
 * ***********************************************************
 */

#ifndef ADDR_TX_H
#define ADDR_TX_H

#include "main.h"

/**
 * @brief 地址发送初始化函数（初始化GPIO）
 */
void addr_tx_init(void);

/**
 * @brief 地址发送处理函数（100us调用一次，需要在定时器中断中调用）
 */
void addr_tx_process(void);

/**
 * @brief 使能地址发送
 * @param addr 要发送的地址（4字节数组，32位）
 * @param len 地址长度（应至少为4）
 * @return true 表示使能成功，false 表示发送忙（上一次发送未完成）
 */
bool addr_tx_enable(const uint8_t *addr, uint8_t len);

#endif /* ADDR_TX_H */
