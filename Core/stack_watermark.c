#include "stack_watermark.h"

/* 是否启用堆栈使用监控功能 */
#define STACK_MONITOR_ENABLE 1

/* 定义水印填充值 */
#define STACK_PATTERN 0xAA

/* 栈大小：确保与启动文件中 Stack_Size 保持一致 */
#ifndef STACK_SIZE
#define STACK_SIZE 0x800 // 示例：0x800 = 2048 字节
#endif

/* 使用率报警阈值（百分比），如 90 表示使用率超过90%报警 */
#define STACK_USAGE_WARN_THRESHOLD 90

/* 外部链接符号，由启动文件导出 */
extern unsigned char Stack_Mem[];

/**
 * @brief 填充从栈底到当前主堆栈指针之间的内存为水印值
 *
 * 说明：
 * - Cortex-M 系列的栈从高地址向低地址增长，
 *   当前的 MSP 指向正被使用的栈顶部，
 *   所以从链接脚本定义的栈底（Stack_Mem）到当前 MSP之间
 *   应该是空闲（未使用）的区域。
 * - 通过填充这部分区域为固定数值，可以在后续检测中，
 *   计算这部分区域被覆盖的最大深度，从而估算栈的最大使用量。
 */
void InitStackWatermark(void)
{
#if STACK_MONITOR_ENABLE
    /* 使用 CMSIS 函数获取当前 MSP（主堆栈指针） */
    uint32_t currentSP = __get_MSP();

    uint8_t *start = Stack_Mem;
    uint8_t *end = (uint8_t *)currentSP;

    while (start < end)
    {
        *start++ = STACK_PATTERN;
    }
#endif
}

/**
 * @brief 从栈底开始扫描连续保持水印值的字节数
 *
 * 注意：该扫描区域只涵盖在 InitStackWatermark() 调用后，
 *  未被运行过程中栈使用覆盖的区域。
 *
 * @return 未使用的栈空间字节数
 */
uint32_t GetUnusedStackSize(void)
{
    uint32_t unused = 0;
    uint8_t *p = Stack_Mem;
    uint8_t *limit = Stack_Mem + STACK_SIZE; // 理论上栈底到栈顶的总区域

    while ((p < limit) && (*p == STACK_PATTERN))
    {
        unused++;
        p++;
    }
    return unused;
}

/**
 * @brief 获取已使用的栈空间大小
 *
 * 计算公式：栈总大小 - 未使用的部分
 *
 * @return 已使用栈空间字节数
 */
uint32_t GetUsedStackSize(void)
{
    return STACK_SIZE - GetUnusedStackSize();
}

/**
 * @brief 监测堆栈使用情况的任务函数
 *
 * 利用 HAL_GetTick 每1000毫秒调用一次 GetUsedStackSize()，
 * 并通过 printf 将已使用的堆栈空间字节数打印出来。
 */
void MonitorStackUsage(void)
{
#if STACK_MONITOR_ENABLE
    static uint32_t previousTick;
    static uint8_t warned = 0;
    uint32_t currentTick = HAL_GetTick();

    if ((currentTick - previousTick) >= 1000)
    {
        uint32_t used = GetUsedStackSize();
        uint32_t usedPercent = (used * 100) / STACK_SIZE;

        Log("Stack used: %lu bytes (%lu%%)\n", (unsigned long)used, (unsigned long)usedPercent);

        if (usedPercent >= STACK_USAGE_WARN_THRESHOLD && !warned)
        {
            Log("Stack usage warning! Used %lu%% of stack.\n", (unsigned long)usedPercent);
            warned = 1; // 报警一次
        }

        previousTick = currentTick;
    }
#endif
}
