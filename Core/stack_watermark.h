#ifndef __STACK_WATERMARK_H__
#define __STACK_WATERMARK_H__

#include <stdint.h>
#include <main.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化堆栈水印
 *
 * 该函数利用 __get_MSP() 填充从栈底（Stack_Mem）
 * 到当前主堆栈指针之间的内存区域为预设水印值（STACK_PATTERN）。
 * 注意：该函数应尽早调用，最好在任务调用前，以保证未使用区均被填充。
 */
void InitStackWatermark(void);

/**
 * @brief 获取未使用的栈空间字节数
 *
 * 从栈底开始扫描，遇到第一个非水印值时，返回该位置到栈底的字节数。
 *
 * @return 未使用栈空间大小（字节数）
 */
uint32_t GetUnusedStackSize(void);

/**
 * @brief 获取已使用的栈空间字节数
 *
 * @return 已使用栈空间大小，即栈总大小减去未使用部分
 */
uint32_t GetUsedStackSize(void);

/**
 * @brief 监测堆栈使用情况
 *
 * 利用 HAL_GetTick 每1000毫秒调用一次 GetUsedStackSize()，
 * 并通过 printf 将已使用的堆栈空间字节数打印出来。
 */
void MonitorStackUsage(void);

#ifdef __cplusplus
}
#endif

#endif /* __STACK_WATERMARK_H__ */
