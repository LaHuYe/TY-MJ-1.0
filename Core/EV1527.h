/**
 * @file EV1527.h
 * @author cyWu (1917507415@qq.com)
 * @brief EV1527解码框架
 * @version 0.1
 * @date 2024-03-28
 * @copyright Copyright (c) 2024
 *
 */

#ifndef __EV1527_H
#define __EV1527_H

#include "main.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
// 数组大小
#define ARRAY_SIZE 3

void EV1527_Init(void);
void RF_Signal_Decode(void);
void Decode_Data(void);
void Execute_Function(void);
void Reset_Decode_Parameters(void);
void compareDataArrays(uint8_t *currentDataArray);
bool arraysEqual(uint8_t array1[], uint8_t array2[], uint8_t size);
void copyArray(uint8_t source[], uint8_t destination[], uint8_t size);

#endif
