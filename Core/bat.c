#include "bat.h"
#include "adc.h"
#include "common.h"

/*========================= 私有变量 =========================*/
static CircularQueue batteryVoltageQueue; // 电压采样队列

/*========================= 函数实现 =========================*/

/**
 * @brief 初始化电池电压队列
 */
void bat_init(void)
{
    initQueue(&batteryVoltageQueue); // 初始化电池电压队列
}

/**
 * @brief   获取电池电压 ADC 采样值
 */
static uint32_t bat_adc_get_voltage(void)
{
    return adc_get_value(ADC_BAT_CH);
}

/**
 * @brief 获取电池电压的移动平均值 (单位: mV)
 * @return 电池电压的移动平均值 (单位: mV)
 */
uint16_t bat_get_voltage_mv(void)
{
    static uint32_t lastPrintTime = 0;
    uint32_t currentTime = HAL_GetTick();
    float adc_vol = 0.0f, bat_vol = 0.0f;

    // 获取电池电压的 ADC 值
    adc_vol = (VREF_V * bat_adc_get_voltage()) / 4095;

    // 计算电池电压（单位：V），分压
    bat_vol = (adc_vol * ((BAT_ADC_R1 + BAT_ADC_R2) / BAT_ADC_R2));

    // 入队电池电压值（单位：mV）
    enqueue(&batteryVoltageQueue, (uint16_t)(bat_vol * 1000)); // 存入队列（转换为 mV）

    // 获取队列中的移动平均值（单位：mV）
    uint16_t avg_bat_vol_mV = movingAverage(&batteryVoltageQueue) + 130; // 计算并获取平均值

    // 每 1000ms (1s) 记录并打印 ADC 数据
    if (currentTime - lastPrintTime >= 1000)
    {
        lastPrintTime = currentTime;
        batPrintf(LOG_DEBUG, "adc_vol:%f \r\n", adc_vol);
        adcPrintf(LOG_DEBUG, "bat_vol: %f V\r\n", bat_vol);
        adcPrintf(LOG_NOTIC, "avg_bat_vol_mV: %d mV\r\n", avg_bat_vol_mV);
    }

    // 返回移动平均值（单位：mV）
    return avg_bat_vol_mV;
}
