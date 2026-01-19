#ifndef __COMMON_H
#define __COMMON_H

/*******************************************************************************
 * Include common files
 ******************************************************************************/
// C基础库
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "main.h"

#define QUEUE_SIZE 20

// 定义循环队列结构体
typedef struct {
    uint16_t data[QUEUE_SIZE]; // 存储数据的数组
    uint8_t front;             // 队列头指针
    uint8_t rear;              // 队列尾指针
    uint8_t count;             // 队列元素个数
} CircularQueue;


void initQueue(CircularQueue *queue);
void clearQueue(CircularQueue *queue);
void enqueue(CircularQueue *queue, uint16_t value);
uint16_t movingAverage(CircularQueue *queue);

#endif
