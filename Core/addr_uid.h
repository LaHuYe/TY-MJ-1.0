#ifndef ADDR_UID_H
#define ADDR_UID_H

#include "main.h"


/**
 * @brief 初始化CRC校验
 */
void crc32_init(void);

/*
 * @brief 计算设备唯一ID
 * @return 设备唯一ID
 */
uint32_t calculate_device_id(void);


#endif
