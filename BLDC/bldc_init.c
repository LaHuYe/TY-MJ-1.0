/******************************************************************************
 * @file    bldc_init.c
 * @brief   BLDC硬件初始化模块实现（GPIO、PWM、比较器）
 * @author  cyWu <1917507415@qq.com>
 * @date    2025-11-25
 * @version V1.0.0
 * @history
 *  - V1.0.0, 2025-11-25, cyWu, 首次发布
 ******************************************************************************/

#include "bldc_init.h"
#include "bldc_comp.h"
#include "tim.h"

/* ==================== 私有函数声明 ==================== */
static void BLDC_GPIO_Init(void);
static void BLDC_PWM_Init(void);
static void BLDC_COMP_GPIO_Init(void);
static void BLDC_COMP_CoreInit(void);

/* ==================== 公有函数实现 ==================== */

/**
 * @brief   初始化BLDC硬件（GPIO、PWM、比较器）
 * @param   无
 * @return  无
 */
void BLDC_Hardware_Init(void)
{
    /* 1. 初始化下桥臂GPIO */
    BLDC_GPIO_Init();

    /* 2. 初始化上桥臂PWM */
    BLDC_PWM_Init();

    /* 3. 初始化比较器GPIO */
    BLDC_COMP_GPIO_Init();

    /* 4. 初始化比较器模块（硬件） */
    BLDC_COMP_CoreInit();

    /* 5. 确保所有输出关闭 */
    BLDC_Stop_All_Output();
}

/**
 * @brief   设置上桥臂PWM（CCR计数值模式，精细控制）
 * @param   u_ccr U相CCR值（0 ~ 1199）
 * @param   v_ccr V相CCR值（0 ~ 1199）
 * @param   w_ccr W相CCR值（0 ~ 1199）
 * @return  无
 * @note    用于闭环速度控制的精细调节
 *          - 直接操作定时器CCR寄存器，实现单个计数值的精细调节
 *          - 相比百分比模式，精度提升12倍（1% = 12个计数值）
 *          - CCR=0时PWM输出低电平，PMOS关断
 *          - 使用硬件配置宏，便于硬件移植
 *
 *          ⚠️ 注意：仅在闭环运行阶段使用，启动阶段用BLDC_SetPWM_Duty()
 */
void BLDC_SetPWM_CCR(uint16_t u_ccr, uint16_t v_ccr, uint16_t w_ccr)
{
    /* 使用硬件配置宏直接设置CCR寄存器（高性能） */
    MOTOR_SET_PWM_U(u_ccr); /* U相上桥：TIM1_CH3（PA10）*/
    MOTOR_SET_PWM_V(v_ccr); /* V相上桥：TIM1_CH2（PA9） */
    MOTOR_SET_PWM_W(w_ccr); /* W相上桥：TIM1_CH1（PA8） */
}

/**
 * @brief   设置下桥臂GPIO状态
 * @param   u_low U相下桥臂状态（0=关闭，1=导通）
 * @param   v_low V相下桥臂状态（0=关闭，1=导通）
 * @param   w_low W相下桥臂状态（0=关闭，1=导通）
 * @return  无
 * @note    使用BSRR寄存器原子操作，高性能
 */
void BLDC_SetLowSide_GPIO(uint8_t u_low, uint8_t v_low, uint8_t w_low)
{
    /* 使用硬件配置宏和BSRR寄存器原子操作（高性能） */
    if (u_low)
        MOTOR_SET_LOW_U_HIGH(); /* U相下桥置高（导通）*/
    else
        MOTOR_SET_LOW_U_LOW(); /* U相下桥置低（关断）*/

    if (v_low)
        MOTOR_SET_LOW_V_HIGH(); /* V相下桥置高（导通）*/
    else
        MOTOR_SET_LOW_V_LOW(); /* V相下桥置低（关断）*/

    if (w_low)
        MOTOR_SET_LOW_W_HIGH(); /* W相下桥置高（导通）*/
    else
        MOTOR_SET_LOW_W_LOW(); /* W相下桥置低（关断）*/
}

/**
 * @brief   停止所有BLDC输出（PWM和GPIO）
 * @param   无
 * @return  无
 */
void BLDC_Stop_All_Output(void)
{
    /* 设置占空比为0 */
    BLDC_SetPWM_CCR(0, 0, 0);

    /* 关闭所有下桥臂GPIO */
    BLDC_SetLowSide_GPIO(0, 0, 0);
}

/* ==================== 私有函数实现 ==================== */

/**
 * @brief   初始化下桥臂GPIO
 * @param   无
 * @return  无
 * @note    配置为推挽输出，初始状态为低电平（关闭）
 */
static void BLDC_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能GPIO时钟 */
    BLDC_U_DOWN_GPIO_CLK_ENABLE();
    BLDC_V_DOWN_GPIO_CLK_ENABLE();
    BLDC_W_DOWN_GPIO_CLK_ENABLE();

    /* 配置U相下桥臂GPIO（PA7） */
    GPIO_InitStruct.Pin = BLDC_U_DOWN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; /* 推挽输出 */
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BLDC_U_DOWN_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(BLDC_U_DOWN_GPIO_PORT, BLDC_U_DOWN_PIN, GPIO_PIN_RESET); /* 初始关闭 */

    /* 配置V相下桥臂GPIO（PB1） */
    GPIO_InitStruct.Pin = BLDC_V_DOWN_PIN;
    HAL_GPIO_Init(BLDC_V_DOWN_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(BLDC_V_DOWN_GPIO_PORT, BLDC_V_DOWN_PIN, GPIO_PIN_RESET); /* 初始关闭 */

    /* 配置W相下桥臂GPIO（PA9） */
    GPIO_InitStruct.Pin = BLDC_W_DOWN_PIN;
    HAL_GPIO_Init(BLDC_W_DOWN_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(BLDC_W_DOWN_GPIO_PORT, BLDC_W_DOWN_PIN, GPIO_PIN_RESET); /* 初始关闭 */
}

/**
 * @brief   初始化上桥臂PWM
 * @param   无
 * @return  无
 * @note    只初始化PWM GPIO引脚，不启动PWM输出
 *          PWM将在 BLDC_Motor_Start() 中启动
 */
static void BLDC_PWM_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能U/V/W相上桥GPIO时钟 */
    BLDC_W_UP_GPIO_CLK_ENABLE();
    BLDC_U_UP_GPIO_CLK_ENABLE();
    BLDC_V_UP_GPIO_CLK_ENABLE();

    /* 配置GPIO为复用推挽输出模式 */
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;       /* 复用推挽输出 */
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;         /* 上拉（PMOS需要上拉） */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; /* 高速 */

    /* 配置U相上桥GPIO（PA6 -> TIM3_CH1） */
    GPIO_InitStruct.Pin = BLDC_U_UP_PIN;
    GPIO_InitStruct.Alternate = BLDC_U_UP_AF; /* 复用功能：TIM3 */
    HAL_GPIO_Init(BLDC_U_UP_GPIO_PORT, &GPIO_InitStruct);

    /* 配置V相上桥GPIO（PB0 -> TIM3_CH2） */
    GPIO_InitStruct.Pin = BLDC_V_UP_PIN;
    GPIO_InitStruct.Alternate = BLDC_V_UP_AF; /* 复用功能：TIM3 */
    HAL_GPIO_Init(BLDC_V_UP_GPIO_PORT, &GPIO_InitStruct);

    /* 配置W相上桥GPIO（PA8 -> TIM1_CH1） */
    GPIO_InitStruct.Pin = BLDC_W_UP_PIN;
    GPIO_InitStruct.Alternate = BLDC_W_UP_AF; /* 复用功能：TIM1 */
    HAL_GPIO_Init(BLDC_W_UP_GPIO_PORT, &GPIO_InitStruct);

    /* 设置初始占空比为0 */
    BLDC_SetPWM_CCR(0, 0, 0);
}

/**
 * @brief   初始化比较器GPIO
 * @param   无
 * @return  无
 * @note    比较器引脚配置为模拟输入模式（GPIO_MODE_ANALOG）
 *
 *          实际引脚配置：
 *          - U相检测：PB4（COMP2_INP1，模拟输入）
 *          - V相检测：PB6（COMP2_INP2，模拟输入）
 *          - W相检测：PF3（COMP2_INP3，模拟输入）
 *          - 中性点参考：PB3（COMP2_INM，模拟输入，1/2母线电压）
 */
static void BLDC_COMP_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能GPIO时钟 */
    BLDC_U_COMP_GPIO_CLK_ENABLE();       /* GPIOB */
    BLDC_V_COMP_GPIO_CLK_ENABLE();       /* GPIOB */
    BLDC_W_COMP_GPIO_CLK_ENABLE();       /* GPIOF */
    BLDC_COMP_NEUTRAL_GPIO_CLK_ENABLE(); /* GPIOB */

    /* 配置GPIO为模拟输入模式 */
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG; /* 模拟输入模式 */
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;    /* 下拉 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    /* 配置U相比较器输入（PB4） */
    GPIO_InitStruct.Pin = BLDC_U_COMP_PIN;
    HAL_GPIO_Init(BLDC_U_COMP_GPIO_PORT, &GPIO_InitStruct);

    /* 配置V相比较器输入（PB6） */
    GPIO_InitStruct.Pin = BLDC_V_COMP_PIN;
    HAL_GPIO_Init(BLDC_V_COMP_GPIO_PORT, &GPIO_InitStruct);

    /* 配置W相比较器输入（PF3） */
    GPIO_InitStruct.Pin = BLDC_W_COMP_PIN;
    HAL_GPIO_Init(BLDC_W_COMP_GPIO_PORT, &GPIO_InitStruct);

    /* 配置中性点参考输入（PB3） */
    GPIO_InitStruct.Pin = BLDC_COMP_NEUTRAL_PIN;
    HAL_GPIO_Init(BLDC_COMP_NEUTRAL_GPIO_PORT, &GPIO_InitStruct);
}

/* ==================== 比较器硬件初始化 ==================== */

/* HAL COMP 句柄 */
COMP_HandleTypeDef hcomp;

/**
 * @brief   初始化比较器硬件（COMP寄存器配置与启动）
 * @param   无
 * @return  无
 * @note    GPIO 已在 BLDC_COMP_GPIO_Init() 完成，此处仅做 COMP 寄存器配置与启动
 */
static void BLDC_COMP_CoreInit(void)
{
    /* 使能COMP时钟并复位 */
    __HAL_RCC_COMP2_CLK_ENABLE();
    HAL_COMP_DeInit(&hcomp);

    /* 配置比较器句柄 */
    hcomp.Instance = COMP2;                   /* 使用 COMP2 */
    hcomp.Init.InputPlus = COMP_POS_U;        /* 初始正端 U相 */
    hcomp.Init.InputMinus = COMP_NEG_NEUTRAL; /* 负端中性点（母线电压1/2） */
    hcomp.Init.OutputPol = COMP_OUTPUTPOL_NONINVERTED;
    hcomp.Init.Mode = COMP_POWERMODE_HIGHSPEED;
    hcomp.Init.Hysteresis = COMP_WINDOWMODE_DISABLE;
    hcomp.Init.WindowMode = COMP_WINDOWMODE_DISABLE;
    hcomp.Init.TriggerMode = COMP_TRIGGERMODE_NONE; /* 不使用比较器中断 */
    hcomp.Init.DigitalFilter = 2000;                /* 过零点滤波值，需按波形调整 */

    if (HAL_COMP_Init(&hcomp) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }

    /* 启动比较器（轮询模式） */
    if (HAL_COMP_Start(&hcomp) != HAL_OK)
    {
        Error_Handler(__FILE__, __LINE__);
    }
}

/**
 * @brief   设置比较器正端输入（根据浮空相）
 * @param   phase 浮空相位（U/V/W）
 * @return  无
 * @note    直接操作 COMP2->CSR，供 BLDC_COMP_SelectPhase 调用
 */
void BLDC_COMP_SetInputPlus(BLDC_Phase_t phase)
{
    uint32_t input_plus;

    switch (phase)
    {
    case BLDC_PHASE_U:
        input_plus = COMP_INPUT_PLUS_IO1;
        break;
    case BLDC_PHASE_V:
        input_plus = COMP_INPUT_PLUS_IO2;
        break;
    case BLDC_PHASE_W:
        input_plus = COMP_INPUT_PLUS_IO4;
        break;
    default:
        return;
    }

    CLEAR_BIT(COMP2->CSR, COMP_CSR_INPSEL_Msk);              /* 清除INPSEL[1:0] */
    MODIFY_REG(COMP2->CSR, COMP_CSR_INPSEL_Msk, input_plus); /* 设置新值 */

    /* 更新HAL句柄（保持一致性）*/
    hcomp.Init.InputPlus = input_plus;
}

/**
 * @brief   读取比较器输出电平
 * @return  1=高电平，0=低电平
 */
uint8_t BLDC_COMP_ReadOutput(void)
{
    return HAL_COMP_GetOutputLevel(&hcomp);
}
