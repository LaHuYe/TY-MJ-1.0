/******************************************************************************
 * @file    bldc_init.h
 * @brief   BLDC硬件初始化模块头文件（GPIO、PWM、比较器）
 * @author  cyWu <1917507415@qq.com>
 * @date    2025-11-25
 * @version V1.0.0
 * @history
 *  - V1.0.0, 2025-11-25, cyWu, 首次发布
 ******************************************************************************/
#ifndef __BLDC_INIT_H__
#define __BLDC_INIT_H__

#include "main.h"

/* 前向声明，避免循环包含 */
typedef enum
{
    BLDC_PHASE_U = 0, /* U相（对应A相）*/
    BLDC_PHASE_V = 1, /* V相（对应B相）*/
    BLDC_PHASE_W = 2  /* W相（对应C相）*/
} BLDC_Phase_t;
/* ==================== 硬件架构说明 ==================== */
/**
 * @brief   BLDC驱动电路架构
 *
 * @note    硬件架构：
 *          - 上桥臂：PMOS，使用TIM3(U,V相)和TIM1(W相)的PWM控制
 *          - 下桥臂：NMOS，使用GPIO直接控制（高电平导通）
 *
 *          PWM输出配置（复用推挽+上拉）：
 *          - U相上桥：PA6  -> TIM3_CH1 (PWM, AF1)
 *          - V相上桥：PB0  -> TIM3_CH2 (PWM, AF1)
 *          - W相上桥：PA8  -> TIM1_CH1 (PWM, AF2)
 *
 *          GPIO输出配置（推挽输出）：
 *          - U相下桥：PA7  -> GPIO输出（高电平=NMOS导通）
 *          - V相下桥：PB1  -> GPIO输出（高电平=NMOS导通）
 *          - W相下桥：PA9  -> GPIO输出（高电平=NMOS导通）
 *
 *          比较器输入配置：
 *          - U/V/W相检测：PA0（多路复用）
 *          - 中性点参考：PA0（需要硬件提供1/2母线电压）
 *
 *          ⚠️ 重要说明：
 *          本模块已包含TIM1和TIM3的GPIO初始化代码（复用功能配置）。
 *          如果在 py32f0xx_hal_msp.c 中有TIM1/TIM3的初始化，请删除以避免重复。
 */

/* ==================== 硬件配置宏（便于移植到不同硬件平台）==================== */
/**
 * @brief   硬件配置宏定义
 * @note    更换硬件平台时，只需修改以下宏定义即可快速适配
 *
 * 当前硬件配置（PY32F030）：
 * - U相上桥：TIM3_CH1 (PA6)    U相下桥：PA7 (GPIO)
 * - V相上桥：TIM3_CH3 (PB0)    V相下桥：PB1 (GPIO)
 * - W相上桥：TIM1_CH1 (PA8)    W相下桥：PA9 (GPIO)
 */

/* PWM定时器寄存器宏（上桥臂控制）*/
#define MOTOR_PWM_U_REG TIM1->CCR1 /* U相PWM寄存器：TIM1_CH1 */
#define MOTOR_PWM_V_REG TIM1->CCR2 /* V相PWM寄存器：TIM1_CH2 */
#define MOTOR_PWM_W_REG TIM1->CCR3 /* W相PWM寄存器：TIM1_CH3 */

/* GPIO端口和引脚宏（下桥臂控制）*/
#define MOTOR_GPIO_U_PORT BLDC_U_DOWN_GPIO_PORT /* U相下桥GPIO端口 */
#define MOTOR_GPIO_U_PIN  BLDC_U_DOWN_PIN       /* U相下桥GPIO引脚：PA6 */
#define MOTOR_GPIO_V_PORT BLDC_V_DOWN_GPIO_PORT /* V相下桥GPIO端口 */
#define MOTOR_GPIO_V_PIN  BLDC_V_DOWN_PIN       /* V相下桥GPIO引脚：PB0 */
#define MOTOR_GPIO_W_PORT BLDC_W_DOWN_GPIO_PORT /* W相下桥GPIO端口 */
#define MOTOR_GPIO_W_PIN  BLDC_W_DOWN_PIN       /* W相下桥GPIO引脚：PB1 */

/* PWM设置宏（简化代码）*/
#define MOTOR_SET_PWM_U(ccr) (MOTOR_PWM_U_REG = (ccr)) /* 设置U相PWM */
#define MOTOR_SET_PWM_V(ccr) (MOTOR_PWM_V_REG = (ccr)) /* 设置V相PWM */
#define MOTOR_SET_PWM_W(ccr) (MOTOR_PWM_W_REG = (ccr)) /* 设置W相PWM */

/* GPIO下桥臂控制宏（使用BSRR寄存器原子操作）*/
#define MOTOR_SET_LOW_U_HIGH() (MOTOR_GPIO_U_PORT->BSRR = MOTOR_GPIO_U_PIN)          /* U相下桥置高（导通）*/
#define MOTOR_SET_LOW_U_LOW()  (MOTOR_GPIO_U_PORT->BSRR = (MOTOR_GPIO_U_PIN << 16U)) /* U相下桥置低（关断）*/
#define MOTOR_SET_LOW_V_HIGH() (MOTOR_GPIO_V_PORT->BSRR = MOTOR_GPIO_V_PIN)          /* V相下桥置高（导通）*/
#define MOTOR_SET_LOW_V_LOW()  (MOTOR_GPIO_V_PORT->BSRR = (MOTOR_GPIO_V_PIN << 16U)) /* V相下桥置低（关断）*/
#define MOTOR_SET_LOW_W_HIGH() (MOTOR_GPIO_W_PORT->BSRR = MOTOR_GPIO_W_PIN)          /* W相下桥置高（导通）*/
#define MOTOR_SET_LOW_W_LOW()  (MOTOR_GPIO_W_PORT->BSRR = (MOTOR_GPIO_W_PIN << 16U)) /* W相下桥置低（关断）*/

/* 比较器输入映射（相位命名：U=A相, V=B相, W=C相）*/
#define COMP_POS_U       COMP_INPUT_PLUS_IO1  /* U相(A相) 假设接 PA3 */
#define COMP_POS_V       COMP_INPUT_PLUS_IO2  /* V相(B相) 假设接 PA4 */
#define COMP_POS_W       COMP_INPUT_PLUS_IO4  /* W相(C相) 假设接 PA5 */
#define COMP_NEG_NEUTRAL COMP_INPUT_MINUS_IO1 /* 负端用中性点（母线电压1/2） */


/* ==================== 电机参数配置（便于移植到不同电机）==================== */
/**
 * @brief   电机极对数配置
 * @note    不同电机的极对数不同，更换电机时需要修改此宏：
 *          - 2极电机：BLDC_POLE_PAIRS = 1
 *          - 4极电机：BLDC_POLE_PAIRS = 2
 *          - 10极电机：BLDC_POLE_PAIRS = 5（当前配置）
 *          - 14极电机：BLDC_POLE_PAIRS = 7
 *
 *          极对数 = 电机极数 / 2
 */
#define BLDC_POLE_PAIRS 7 /* 电机极对数：当前为10极电机（5极对） */

/**
 * @brief   过零检测采样周期配置（单位：us）
 * @note    根据电机极对数和最高转速选择合适的采样周期：
 *
 *          【2极对数电机】
 *          - 100us (10kHz)  适用于 ≤6000 RPM   ✅
 *          - 50us  (20kHz)  适用于 ≤12000 RPM
 *          - 20us  (50kHz)  适用于 ≤30000 RPM
 *
 *          【5极对数电机】⚠️ 需要更快的采样频率
 *          - 100us (10kHz)  适用于 ≤2500 RPM   ⚠️ 临界
 *          - 50us  (20kHz)  适用于 ≤5000 RPM   ✅ 当前配置
 *          - 20us  (50kHz)  适用于 ≤12000 RPM
 */
#define BLDC_SAMPLE_PERIOD_US 50 /* 采样周期：50us（20kHz采样频率） */

/* ==================== 档位定义 ==================== */
/**
 * @brief   BLDC电机档位枚举
 * @note    - BLDC_GEAR_OFF (0)：关闭电机
 *          - BLDC_GEAR_1 ~ BLDC_GEAR_100 (1-100)：运行档位
 *          - 转速计算公式：RPM = 500 + (gear - 1) * 23
 *          - 档位1：500 RPM
 *          - 档位100：500 + 99*23 = 2777 RPM
 */
typedef enum
{
    BLDC_GEAR_OFF = 0,  /* 关闭 */
    BLDC_GEAR_1 = 1,    /* 1档：500 RPM */
    BLDC_GEAR_100 = 100, /* 100档：2777 RPM */
    BLDC_GEAR_MAX = 101  /* 档位上限（0-100共101个档位，用于边界检查） */
} BLDC_Gear_t;
/* ==================== 函数声明 ==================== */

/**
 * @brief   初始化BLDC硬件（GPIO、PWM、比较器）
 * @param   无
 * @return  无
 * @note    包含：
 *          1. 上桥臂PWM初始化（TIM1, TIM3）
 *          2. 下桥臂GPIO初始化（推挽输出）
 *          3. 比较器GPIO初始化（模拟输入）
 *             - U相：PB4（模拟输入）
 *             - V相：PB6（模拟输入）
 *             - W相：PF3（模拟输入）
 *             - 中性点：PB3（模拟输入，1/2母线电压）
 *          4. 比较器模块初始化（COMP2）
 */
void BLDC_Hardware_Init(void);

/**
 * @brief   设置上桥臂PWM（CCR计数值模式，精细控制）
 * @param   u_ccr U相CCR值（0 ~ 1199）
 * @param   v_ccr V相CCR值（0 ~ 1199）
 * @param   w_ccr W相CCR值（0 ~ 1199）
 * @return  无
 * @note    用于闭环速度控制的精细调节
 *          - CCR=0：输出0%（关闭PMOS）
 *          - CCR=1199：输出100%（全速）
 *          - 调节步长建议：1-5个计数值
 */
void BLDC_SetPWM_CCR(uint16_t u_ccr, uint16_t v_ccr, uint16_t w_ccr);

/**
 * @brief   设置下桥臂GPIO状态
 * @param   u_low U相下桥臂状态（0=关闭，1=导通）
 * @param   v_low V相下桥臂状态（0=关闭，1=导通）
 * @param   w_low W相下桥臂状态（0=关闭，1=导通）
 * @return  无
 * @note    NMOS下桥臂，高电平导通
 */
void BLDC_SetLowSide_GPIO(uint8_t u_low, uint8_t v_low, uint8_t w_low);

/**
 * @brief   停止所有BLDC输出（PWM和GPIO）
 * @param   无
 * @return  无
 * @note    用于电机停止或紧急停止
 */
void BLDC_Stop_All_Output(void);

/**
 * @brief   初始化比较器硬件（GPIO + COMP寄存器）
 * @param   无
 * @return  无
 * @note    包含GPIO模拟输入配置与COMP寄存器配置/启动
 */
void BLDC_COMP_HardwareInit(void);

/**
 * @brief   设置比较器正端输入（根据浮空相）
 * @param   phase 浮空相位（U/V/W）
 * @return  无
 * @note    内部直接操作 COMP2->CSR，供 BLDC_COMP_SelectPhase 调用
 */
void BLDC_COMP_SetInputPlus(BLDC_Phase_t phase);

/**
 * @brief   读取比较器输出电平
 * @return  1=高电平，0=低电平
 */
uint8_t BLDC_COMP_ReadOutput(void);

/**
 * @brief   根据档位动态更新比较器硬件数字滤波值
 * @param   gear 当前档位（1-100，0表示停止）
 * @note    默认50000，每升一档减1000，最小500
 */
void BLDC_COMP_UpdateDigitalFilter(BLDC_Gear_t gear);

/* 比较器句柄外部声明 */
extern COMP_HandleTypeDef hcomp;

#endif /* __BLDC_INIT_H__ */
