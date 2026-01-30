/*
 * THE FOLLOWING FIRMWARE IS PROVIDED: (1) "AS IS" WITH NO WARRANTY; AND
 * (2)TO ENABLE ACCESS TO CODING INFORMATION TO GUIDE AND FACILITATE CUSTOMER.
 * CONSEQUENTLY, CMOSTEK SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT OR
 * CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT
 * OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION
 * CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * Copyright (C) CMOSTEK SZ.
 */

/*!
 * @file    radio.c
 * @brief   Generic radio handlers
 *
 * @version 1.2
 * @date    Jul 17 2017
 * @author  CMOSTEK R@D
 */

#include "radio.h"
#include "cmt2300a_params.h"

#include <string.h>

static bool g_nRfRxDoneFlag = false;
uint32_t g_nRfRxtimeoutCount = 0;
static uint8_t RxBuffer[RF_PACKET_SIZE]; /* RF Rx buffer */

void RF_Init(void)
{
    u8 tmp;

    CMT2300A_InitGpio();
    CMT2300A_Init();

    /* Config registers */
    CMT2300A_ConfigRegBank(CMT2300A_CMT_BANK_ADDR, g_cmt2300aCmtBank, CMT2300A_CMT_BANK_SIZE);
    CMT2300A_ConfigRegBank(CMT2300A_SYSTEM_BANK_ADDR, g_cmt2300aSystemBank, CMT2300A_SYSTEM_BANK_SIZE);
    CMT2300A_ConfigRegBank(CMT2300A_FREQUENCY_BANK_ADDR, g_cmt2300aFrequencyBank, CMT2300A_FREQUENCY_BANK_SIZE);
    CMT2300A_ConfigRegBank(CMT2300A_DATA_RATE_BANK_ADDR, g_cmt2300aDataRateBank, CMT2300A_DATA_RATE_BANK_SIZE);
    CMT2300A_ConfigRegBank(CMT2300A_BASEBAND_BANK_ADDR, g_cmt2300aBasebandBank, CMT2300A_BASEBAND_BANK_SIZE);
    CMT2300A_ConfigRegBank(CMT2300A_TX_BANK_ADDR, g_cmt2300aTxBank, CMT2300A_TX_BANK_SIZE);

    // xosc_aac_code[2:0] = 2
    tmp = (~0x07) & CMT2300A_ReadReg(CMT2300A_CUS_CMT10);
    CMT2300A_WriteReg(CMT2300A_CUS_CMT10, tmp | 0x02);

    RF_Config();

    if (FALSE == CMT2300A_IsExist())
    { 
        Log("CMT2300A not found!");
    }
    else
    {
        Log("CMT2300A ready RX");
    }

    CMT2300A_GoStby();

    /* Must clear FIFO after enable SPI to read or write the FIFO */
    CMT2300A_EnableReadFifo();
    CMT2300A_ClearInterruptFlags();
    CMT2300A_ClearRxFifo();
    CMT2300A_GoRx();
}

void RF_Config(void)
{
#ifdef ENABLE_ANTENNA_SWITCH
    u8 nInt2Sel;
    /* If you enable antenna switch, GPIO1/GPIO2 will output RX_ACTIVE/TX_ACTIVE,
       and it can't output INT1/INT2 via GPIO1/GPIO2 */
    CMT2300A_EnableAntennaSwitch(0);

    /* Config GPIOs */
    CMT2300A_ConfigGpio(CMT2300A_GPIO3_SEL_INT2); /* INT2 > GPIO3 */
    /* Config interrupt */
    nInt2Sel = CMT2300A_INT_SEL_PKT_DONE; /* Config INT2 */
    nInt2Sel &= CMT2300A_MASK_INT2_SEL;
    nInt2Sel |= (~CMT2300A_MASK_INT2_SEL) & CMT2300A_ReadReg(CMT2300A_CUS_INT2_CTL);
    CMT2300A_WriteReg(CMT2300A_CUS_INT2_CTL, nInt2Sel);
#else
    CMT2300A_ConfigGpio(CMT2300A_GPIO1_SEL_INT1 | /* INT1 > GPIO1 */
                        CMT2300A_GPIO2_SEL_INT2 | /* INT2 > GPIO2 */
                        CMT2300A_GPIO3_SEL_DOUT);

    CMT2300A_ConfigInterrupt(CMT2300A_INT_SEL_SYNC_OK, /* GPIO1 > SYNC_OK */
                             CMT2300A_INT_SEL_PKT_DONE /* GPIO2 > PKT_DONE*/
    );

#endif

    /* Enable interrupt */
    CMT2300A_EnableInterrupt(
        CMT2300A_MASK_PKT_DONE_EN |
        CMT2300A_MASK_PREAM_OK_EN |
        CMT2300A_MASK_SYNC_OK_EN
        //        CMT2300A_MASK_NODE_OK_EN  |
        //        CMT2300A_MASK_CRC_OK_EN   |
        //        CMT2300A_MASK_TX_DONE_EN
    );

    /* Disable low frequency OSC calibration */
    CMT2300A_EnableLfosc(FALSE);

    /* Use a single 64-byte FIFO for either Tx or Rx */
    // CMT2300A_EnableFifoMerge(TRUE);

    // CMT2300A_SetFifoThreshold(16); // FIFO_TH

    /* This is optional, only needed when using Rx fast frequency hopping */
    /* See AN142 and AN197 for details */
    // CMT2300A_SetAfcOvfTh(0x27);

    /* Go to sleep for configuration to take effect */

    CMT2300A_GoSleep();
}


/**
 * @brief   设置接收完成标志位
 * @param   done_flag: 完成标志位
 * @return  none
 * @note    设置接收完成标志位
 */
static void RF_SetRxDoneFlag(bool done_flag)
{
    g_nRfRxDoneFlag = done_flag;
}

/**
 * @brief   获取接收完成标志位
 * @param   none
 * @return  bool: 完成标志位
 * @note    获取接收完成标志位
 */
static bool RF_GetRxDoneFlag(void)
{
    return g_nRfRxDoneFlag;
}

/**
 * @brief   获取接收缓冲区
 * @param   none
 * @return  uint8_t *: 接收缓冲区
 * @note    获取接收缓冲区
 */
uint8_t *RF_GetRxBuffer(void)
{
    //接收完成
    if (RF_GetRxDoneFlag())
    {
        RF_SetRxDoneFlag(false);
        return RxBuffer;
    }
    return NULL;
}

/**
 * @brief   接收固定长度的数据
 * @param   pBuf: 接收缓冲区
 * @param   len: 接收长度
 * @return  0: 失败, 1: 成功
 * @note    接收固定长度的数据
 */
uint8_t Radio_Recv_FixedLen(uint8_t pBuf[], uint8_t len)
{
#ifdef ENABLE_ANTENNA_SWITCH
    if (CMT2300A_ReadGpio3()) /* Read INT2, PKT_DONE */
#else
    if (CMT2300A_ReadGpio1()) /* Read INT1, SYNC OK */
    {
        /* code */
    }
    if (CMT2300A_ReadGpio2()) /* Read INT2, PKT_DONE */
#endif
    {
        // if(CMT2300A_MASK_PKT_OK_FLG & CMT2300A_ReadReg(CMT2300A_CUS_INT_FLAG))  /* Read PKT_OK flag */
        {
            CMT2300A_GoStby();
            CMT2300A_ReadFifo(pBuf, len);
            CMT2300A_ClearRxFifo();
            CMT2300A_ClearInterruptFlags();
            CMT2300A_GoRx();

            return 1;
        }
    }

    return 0;
}

/**
 * @brief   接收天线处理
 * @param   none
 * @return  none
 * @note    接收天线处理
 */
void radio_recv_handle(void)
{
    if (Radio_Recv_FixedLen(RxBuffer, RF_PACKET_SIZE))
    {
        g_nRfRxtimeoutCount = HAL_GetTick();
        RF_SetRxDoneFlag(true);
    }

    if (HAL_GetTickDiff(g_nRfRxtimeoutCount) >= RF_RX_TIMEOUT) // 根据实际应用可以调整Time Out
    {
        g_nRfRxtimeoutCount = HAL_GetTick();
        CMT2300A_GoSleep();
        CMT2300A_GoStby();
        CMT2300A_ClearInterruptFlags();
        CMT2300A_ClearRxFifo();
        CMT2300A_GoRx();
    }
}
