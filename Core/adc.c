#include "adc.h"

ADC_HandleTypeDef AdcHandle;
void adc_Init(void)
{
    /* 使能 ADC 时钟 */
    __HAL_RCC_ADC_CLK_ENABLE();

    /* 设置 ADC 句柄 */
    AdcHandle.Instance = ADC1;
    if (HAL_ADC_DeInit(&AdcHandle) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__); // 发生错误，进入错误处理函数
    }
    /* ADC 初始化 */
    AdcHandle.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV64;           /* ADC 时钟分频 */
    AdcHandle.Init.Resolution = ADC_RESOLUTION_12B;                      /* 12 位 ADC 精度 */
    AdcHandle.Init.DataAlign = ADC_DATAALIGN_RIGHT;                      /* 数据右对齐 */
    AdcHandle.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;            /* 多通道扫描模式 */
    AdcHandle.Init.EOCSelection = ADC_EOC_SEQ_CONV;                      /* 每次转换序列完成后触发中断 */
    AdcHandle.Init.LowPowerAutoWait = DISABLE;                           /* 低功耗自动等待模式 */
    AdcHandle.Init.ContinuousConvMode = DISABLE;                         /* 开启连续转换模式 */
    AdcHandle.Init.DiscontinuousConvMode = DISABLE;                      /* 禁用不连续模式 */
    AdcHandle.Init.ExternalTrigConv = ADC_SOFTWARE_START;                /* 软件触发模式 */
    AdcHandle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE; /* 无外部触发 */
    AdcHandle.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;                   /* 旧数据被覆盖 */
    AdcHandle.Init.SamplingTimeCommon = ADC_SAMPLETIME_239CYCLES_5;      /* 设置默认采样时间 */

    /* 初始化 ADC */
    if (HAL_ADC_Init(&AdcHandle) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__); // 发生错误，进入错误处理函数
    }

    HAL_ADC_ConfigVrefBuf(&AdcHandle, ADC_VREFBUF_2P5V); // 参考电压VDD

    /* ADC 校准（初始化后进行） */
    if (HAL_ADCEx_Calibration_Start(&AdcHandle) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__); // 发生错误，进入错误处理函数
    }
}

// 获取对应通道的ADC值
uint32_t adc_get_value(uint32_t ch)
{
    uint16_t adcvalue;
    ADC_ChannelConfTypeDef sConfig = {0};

    /* Configure channel need ADC is disable */
    if (READ_BIT(AdcHandle.Instance->CR, ADC_CR_ADEN) == ADC_CR_ADEN)
    {
        __HAL_ADC_DISABLE(&AdcHandle);
    }

    /* Config selected channels */
    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    sConfig.Channel = ch;
    if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK) /* Configure ADC Channel */
    {
        Error_Handler(__FILE__, __LINE__);
    }

    /* ADC Start */
    HAL_ADC_Start(&AdcHandle);

    /* Polling for ADC Conversion */
    HAL_ADC_PollForConversion(&AdcHandle, 1000000);

    /* Get ADC Value */
    adcvalue = HAL_ADC_GetValue(&AdcHandle);

    /* Disable ADC to clear channel configuration */
    __HAL_ADC_DISABLE(&AdcHandle);

    /* Clear the selected channels */
    sConfig.Rank = ADC_RANK_NONE;
    sConfig.Channel = ch;
    if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK) /* Configure ADC Channel */
    {
        Error_Handler(__FILE__, __LINE__);
    }

    return (uint32_t)adcvalue;
}
