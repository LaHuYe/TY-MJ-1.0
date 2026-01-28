
#include "cmt_spi3.h"
#include "py32f0xx_hal_gpio.h"
#include "py32f0xx_hal.h"

/* ************************************************************************
*  The following need to be modified by user
*  ************************************************************************ */

#define cmt_spi3_csb_1()        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET)
#define cmt_spi3_csb_0()        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET)

#define cmt_spi3_fcsb_1()       HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET)
#define cmt_spi3_fcsb_0()       HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET)
    
#define cmt_spi3_sclk_1()       HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET)
#define cmt_spi3_sclk_0()       HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET)

#define cmt_spi3_sdio_1()       HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)
#define cmt_spi3_sdio_0()       HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define cmt_spi3_sdio_read()    HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4)
/* ************************************************************************ */
    
void cmt_spi3_delay(void)
{
    u32 n = 7;
    while(n--);
}

void cmt_spi3_delay_us(void)
{
    u16 n = 8;
    while(n--);
}

void cmt_spi3_init(void)
{
		GPIO_InitTypeDef  GPIO_InitStruct;
	
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;						/* CSB  FCSB*/
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;            			/* input */
		GPIO_InitStruct.Pull = GPIO_PULLUP;                    /* Enable pull-up */
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; 
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
    cmt_spi3_csb_1();   																	/* CSB has an internal pull-up resistor */
    
		GPIO_InitStruct.Pin = GPIO_PIN_3;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;            		/* input */
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;                    /* SCLK has an internal pull-down resistor */
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; 
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    cmt_spi3_sclk_0();   																/* SCLK has an internal pull-down resistor */
    
		GPIO_InitStruct.Pin = GPIO_PIN_4;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;            		/* input */
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;                    /* SDIO has an internal pull-down resistor */
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; 
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    cmt_spi3_sdio_1();
    

    cmt_spi3_fcsb_1();  /* FCSB has an internal pull-up resistor */

    cmt_spi3_delay();
}

void cmt_spi3_send(u8 data8)
{
    u8 i;

    for(i=0; i<8; i++)
    {
        cmt_spi3_sclk_0();

        /* Send byte on the rising edge of SCLK */
        if(data8 & 0x80)
            cmt_spi3_sdio_1();
        else            
            cmt_spi3_sdio_0();

        cmt_spi3_delay();

        data8 <<= 1;
        cmt_spi3_sclk_1();
        cmt_spi3_delay();
    }
}

u8 cmt_spi3_recv(void)
{
    u8 i;
    u8 data8 = 0xFF;

    for(i=0; i<8; i++)
    {
        cmt_spi3_sclk_0();
        cmt_spi3_delay();
        data8 <<= 1;

        cmt_spi3_sclk_1();

        /* Read byte on the rising edge of SCLK */
        if(cmt_spi3_sdio_read())
            data8 |= 0x01;
        else
            data8 &= ~0x01;

        cmt_spi3_delay();
    }

    return data8;
}

void cmt_spi3_write(u8 addr, u8 dat)
{
		GPIO_InitTypeDef  GPIO_InitStruct;
	
		GPIO_InitStruct.Pin = GPIO_PIN_4;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;            		/* in */
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;                 /* SDIO has an internal pull-down resistor */
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; 
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    cmt_spi3_sdio_1();
    
    cmt_spi3_sclk_0(); 

    cmt_spi3_fcsb_1();

    cmt_spi3_csb_0();

    /* > 0.5 SCLK cycle */
    cmt_spi3_delay();
    cmt_spi3_delay();

    /* r/w = 0 */
    cmt_spi3_send(addr&0x7F);

    cmt_spi3_send(dat);

    cmt_spi3_sclk_0();

    /* > 0.5 SCLK cycle */
    cmt_spi3_delay();
    cmt_spi3_delay();

    cmt_spi3_csb_1();
    
    cmt_spi3_sdio_1();
    GPIO_InitStruct.Pin = GPIO_PIN_4;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;            		/* in */
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;                 /* SDIO has an internal pull-down resistor */
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; 
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    cmt_spi3_fcsb_1();    
}

void cmt_spi3_read(u8 addr, u8* p_dat)
{
		GPIO_InitTypeDef  GPIO_InitStruct;
	
    GPIO_InitStruct.Pin = GPIO_PIN_4;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;            		/* input */
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;                    /* SDIO has an internal pull-down resistor */
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; 
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		cmt_spi3_sdio_1();

    cmt_spi3_sclk_0(); 

    cmt_spi3_fcsb_1();

    cmt_spi3_csb_0();

    /* > 0.5 SCLK cycle */
    cmt_spi3_delay();
    cmt_spi3_delay();

    /* r/w = 1 */
    cmt_spi3_send(addr|0x80);

    /* Must set SDIO to input before the falling edge of SCLK */
    GPIO_InitStruct.Pin = GPIO_PIN_4;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;            		/* in */
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;                 /* SDIO has an internal pull-down resistor */
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; 
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    *p_dat = cmt_spi3_recv();

    cmt_spi3_sclk_0();

    /* > 0.5 SCLK cycle */
    cmt_spi3_delay();
    cmt_spi3_delay();

    cmt_spi3_csb_1();
    
		GPIO_InitStruct.Pin = GPIO_PIN_4;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;            		/* in */
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;                 /* SDIO has an internal pull-down resistor */
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; 
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    cmt_spi3_sdio_1();
		
    GPIO_InitStruct.Pin = GPIO_PIN_4;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;            		/* in */
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;                 /* SDIO has an internal pull-down resistor */
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; 
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    cmt_spi3_fcsb_1();
}

void cmt_spi3_write_fifo(const u8* p_buf, u16 len)
{
		GPIO_InitTypeDef  GPIO_InitStruct;
    u16 i;

    cmt_spi3_fcsb_1();

    cmt_spi3_csb_1();

    cmt_spi3_sclk_0();

		GPIO_InitStruct.Pin = GPIO_PIN_4;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;            		/* in */
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;                 /* SDIO has an internal pull-down resistor */
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; 
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    for(i=0; i<len; i++)
    {
        cmt_spi3_fcsb_0();

        /* > 1 SCLK cycle */
        cmt_spi3_delay();
        cmt_spi3_delay();

        cmt_spi3_send(p_buf[i]);

        cmt_spi3_sclk_0();

        /* > 2 us */
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();

        cmt_spi3_fcsb_1();

        /* > 4 us */
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
    }

    GPIO_InitStruct.Pin = GPIO_PIN_4;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;            		/* in */
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;                 /* SDIO has an internal pull-down resistor */
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; 
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    cmt_spi3_fcsb_1();
}

void cmt_spi3_read_fifo(u8* p_buf, u16 len)
{
		GPIO_InitTypeDef  GPIO_InitStruct;
    u16 i;

    cmt_spi3_fcsb_1();

    cmt_spi3_csb_1();

    cmt_spi3_sclk_0();

    GPIO_InitStruct.Pin = GPIO_PIN_4;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;            		/* in */
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;                 /* SDIO has an internal pull-down resistor */
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; 
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    for(i=0; i<len; i++)
    {
        cmt_spi3_fcsb_0();

        /* > 1 SCLK cycle */
        cmt_spi3_delay();
        cmt_spi3_delay();

        p_buf[i] = cmt_spi3_recv();

        cmt_spi3_sclk_0();

        /* > 2 us */
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();

        cmt_spi3_fcsb_1();

        /* > 4 us */
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
        cmt_spi3_delay_us();
    }

   // cmt_spi3_sdio_in();
    
    cmt_spi3_fcsb_1();
}
