#include "bat.h"
#include "adc.h"
#include "common.h"
#include "power_control.h"

/*========================= 私有类型定义 =========================*/
typedef struct {
    float voltage;        // 当前电压值（滤波后的电压，单位：mV）
    uint16_t voltage_mv;  // 当前处理后的电压值（单位：mV）
    bool is_charging;     // 充电状态
    bool is_initialized;  // 初始化标志
    bool is_switching;    // 状态切换标志
    uint32_t switch_time; // 状态切换时间戳
} bat_state_t;

typedef struct {
    uint16_t base_voltage; // 基准电压（单位：mV）
    bool is_recorded;      // 是否已记录基准
} bat_base_t;

/*========================= 私有常量定义 =========================*/
#define DISCHARGE_ALPHA 0.90f // 放电电压滤波系数(0~1)

/*========================= 私有变量 =========================*/
static CircularQueue batteryVoltageQueue; // 电压采样队列
static bat_state_t s_state;               // 电池状态

/*========================= 私有函数声明 =========================*/
static uint32_t bat_adc_get_voltage(void);
static void bat_update_voltage_filter(float new_voltage);

/*========================= 函数实现 =========================*/

/**
 * @brief 初始化电池电压队列
 */
void bat_init(void)
{
    initQueue(&batteryVoltageQueue); // 初始化电池电压队列

    // 初始化状态
    s_state.voltage = 0.0f;
    s_state.voltage_mv = 8400; // 初始电压设为最大值（单位：mV）
    s_state.is_charging = false;
    s_state.is_initialized = false;
}

/**
 * @brief   设置电池电压
 * @param   voltage_mv: 电池电压（单位：mV）
 */
void bat_set_voltage(uint16_t voltage_mv)
{
    s_state.voltage_mv = voltage_mv;
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
    uint16_t avg_bat_vol_mV = movingAverage(&batteryVoltageQueue) + 400; // 计算并获取平均值

    // 每 1000ms (1s) 记录并打印 ADC 数据
    if (currentTime - lastPrintTime >= 1000)
    {
        lastPrintTime = currentTime;
//        batPrintf(LOG_DEBUG, "adc_vol:%f \r\n", adc_vol);
//        adcPrintf(LOG_DEBUG, "bat_vol: %f V\r\n", bat_vol);
        adcPrintf(LOG_NOTIC, "avg_bat_vol_mV: %d mV\r\n", avg_bat_vol_mV);
    }

    // 返回移动平均值（单位：mV）
    return avg_bat_vol_mV;
}

/**
 * @brief   更新电池电压
 * @param   measured_voltage: 当前采样电压(mV)
 * @param   charger_connected: 充电器连接状态
 * @return  处理后的电压值(mV)
 */
uint16_t bat_update_voltage(uint16_t measured_voltage, bool charger_connected)
{
    // 首次上电初始化
    if (!s_state.is_initialized)
    {
        s_state.voltage = (float)measured_voltage;
        s_state.is_initialized = true;
        if (charger_connected)
        {
            s_state.voltage_mv = BASIC_CHARGE_VOLT_MIN; // 充电时初始电压设为最小值
        }
        else
        {
            s_state.voltage_mv = BASIC_DISCHARGE_VOLT_MAX; // 放电时初始电压设为最大值
        }
    }

    // 更新电压滤波
    bat_update_voltage_filter((float)measured_voltage);

    // 直接处理电压值，不再转换为百分比
    uint16_t new_voltage_mv = (uint16_t)s_state.voltage;
    
    if (!charger_connected)
    {
        // 放电时电压只减不增
        if (new_voltage_mv > s_state.voltage_mv)
        {
            new_voltage_mv = s_state.voltage_mv;
        }
    }
    else
    {
        // 充电时电压只增不减
        if (new_voltage_mv < s_state.voltage_mv)
        {
            new_voltage_mv = s_state.voltage_mv;
        }
    }

    // 更新状态
    s_state.voltage_mv = new_voltage_mv;

    return s_state.voltage_mv;
}

/**
 * @brief   获取处理后的电池电压（单位：mV）
 * @return  处理后的电池电压（单位：mV）
 */
uint16_t bat_get_processed_voltage_mv(void)
{
    // return s_state.voltage_mv;
    return bat_get_voltage_mv();
}

/**
 * @brief   每100ms更新一次电池电压
 */
void bat_update_handle(void)
{
    static uint32_t last_update_time = 0;
    uint32_t current_time = HAL_GetTick();
    static uint8_t last_print_time = 0;

    // 每100ms更新一次电池电压
    if (current_time - last_update_time >= 100)
    {
        last_update_time = current_time;
        // 获取充电状态
        bool charger_connected = get_charge_enable();
        // 更新电池电压
        uint16_t bat_vol = bat_get_voltage_mv();
        uint16_t processed_vol = bat_update_voltage(bat_vol, charger_connected);

        // 1s打印一次电压
        last_print_time++;
        if (last_print_time >= 10)
        {
            last_print_time = 0;
            batPrintf(LOG_NOTIC, "bat_vol: %d mV, processed_vol: %d mV, charger_connected: %d\r\n", bat_vol, processed_vol, charger_connected);
        }
    }
}

/*========================= 私有函数实现 =========================*/

/**
 * @brief   获取电池电压 ADC 采样值
 */
static uint32_t bat_adc_get_voltage(void)
{
    return adc_get_value(ADC_BAT_CH);
}

/**
 * @brief   更新电压滤波值
 */
static void bat_update_voltage_filter(float new_voltage)
{
    if (!s_state.is_charging)
    {
        // 放电时使用指数滤波
        s_state.voltage = DISCHARGE_ALPHA * s_state.voltage + (1.0f - DISCHARGE_ALPHA) * new_voltage;
    }
    else
    {
        // 充电时直接使用采样值
        s_state.voltage = new_voltage;
    }
}
