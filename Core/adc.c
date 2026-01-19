#include "adc.h"

const uint32_t channels[] = {ADC_BAT_CH,ADC_BLDC_CH};
uint32_t gADCxConvertedData[sizeof(channels) / sizeof(channels[0])] = {0};
ADC_HandleTypeDef AdcHandle;
void adc_Init(void)
{

    ADC_ChannelConfTypeDef sConfig = {0};

    /* 使能 ADC 时钟 */
    __HAL_RCC_ADC_CLK_ENABLE();

    /* 设置 ADC 句柄 */
    AdcHandle.Instance = ADC1;
    if (HAL_ADC_DeInit(&AdcHandle) != HAL_OK)
        while (1)
            ;
    /* ADC 初始化 */
    AdcHandle.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV16;           /* ADC 时钟分频 */
    AdcHandle.Init.Resolution = ADC_RESOLUTION_12B;                      /* 12 位 ADC 精度 */
    AdcHandle.Init.DataAlign = ADC_DATAALIGN_RIGHT;                      /* 数据右对齐 */
    AdcHandle.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;            /* 多通道扫描模式 */
    AdcHandle.Init.EOCSelection = ADC_EOC_SEQ_CONV;                      /* 每次转换序列完成后触发中断 */
    AdcHandle.Init.LowPowerAutoWait = ENABLE;                            /* 低功耗自动等待模式 */
    AdcHandle.Init.ContinuousConvMode = ENABLE;                          /* 开启连续转换模式 */
    AdcHandle.Init.DiscontinuousConvMode = DISABLE;                      /* 禁用不连续模式 */
    AdcHandle.Init.ExternalTrigConv = ADC_SOFTWARE_START;                /* 软件触发模式 */
    AdcHandle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE; /* 无外部触发 */
    AdcHandle.Init.DMAContinuousRequests = ENABLE;                       /* 使能 DMA 连续请求 */
    AdcHandle.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;                   /* 旧数据被覆盖 */
    AdcHandle.Init.SamplingTimeCommon = ADC_SAMPLETIME_239CYCLES_5;      /* 设置默认采样时间 */

    /* 计算通道数量，提高效率 */
    uint8_t channelCount = sizeof(channels) / sizeof(channels[0]);

    /* 配置 ADC 各个通道 */
    for (uint8_t i = 0; i < channelCount; i++)
    {
        sConfig.Channel = channels[i];                     /* 选择通道 */
        sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;            /* 设置转换顺序 */
        sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5; /* 采样时间（可根据实际需求修改） */

        /* 配置 ADC 通道 */
        if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK)
        {
            Error_Handler(__FILE__, __LINE__); // 发生错误，进入错误处理函数
        }
    }

    /* 初始化 ADC */
    if (HAL_ADC_Init(&AdcHandle) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__); // 发生错误，进入错误处理函数
    }

    /* ADC 校准（初始化后进行） */
    if (HAL_ADCEx_Calibration_Start(&AdcHandle) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__); // 发生错误，进入错误处理函数
    }

    /* 启动 ADC + DMA */
    if (HAL_ADC_Start_DMA(&AdcHandle, (uint32_t *)gADCxConvertedData, channelCount) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__); // 发生错误，进入错误处理函数
    }
}

// 获取对应通道的ADC值
uint32_t adc_get_value(uint32_t ch)
{
    static uint32_t lastPrintTime = 0;
    uint32_t currentTime = HAL_GetTick();

    // 每 1000ms (1s) 记录并打印 ADC 数据
    if (currentTime - lastPrintTime >= 1000)
    {
        lastPrintTime = currentTime;
        for (uint8_t i = 0; i < sizeof(gADCxConvertedData) / sizeof(gADCxConvertedData[0]); i++)
        {
            adcPrintf(LOG_DUMP, "gADCxConvertedData[%d]: %d\r\n", i, gADCxConvertedData[i]);
        }
    }
    for (uint8_t i = 0; i < sizeof(channels) / sizeof(channels[0]); i++)
    {
        if (channels[i] == ch)
        {
            return gADCxConvertedData[i];
        }
    }
    return 0;
}
