#include "addr_uid.h"

CRC_HandleTypeDef CrcHandle;

/**
 * @brief 初始化CRC校验
 */
void crc32_init(void)
{
    __HAL_RCC_CRC_CLK_ENABLE();
    /* Initialize CRC */
    CrcHandle.Instance = CRC;
    if (HAL_CRC_Init(&CrcHandle) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }
}

/*
 * @brief 读取MCU唯一ID
 * @param uid 存储MCU唯一ID的数组
 */
void read_mcu_uid(uint32_t *uid)
{
    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();
    // uid[3] = HAL_GetTick(); // 获取系统时间
}

/*
 * @brief 计算设备唯一ID
 * @return 设备唯一ID
 */
uint32_t calculate_device_id(void)
{
    uint32_t mcu_uid[3] = {0};

    // 读取MCU唯一ID
    read_mcu_uid(mcu_uid);

    __IO uint32_t uwCRCValue = 0;

    // 计算CRC校验
    uwCRCValue = HAL_CRC_Calculate(&CrcHandle, mcu_uid, 3);

    return uwCRCValue;
}
