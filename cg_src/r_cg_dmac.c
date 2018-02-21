/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products.
* No other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
* applicable laws, including copyright laws. 
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIESREGARDING THIS SOFTWARE, WHETHER EXPRESS, IMPLIED
* OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NON-INFRINGEMENT.  ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY
* LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE FOR ANY DIRECT,
* INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR
* ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability 
* of this software. By using this software, you agree to the additional terms and conditions found by accessing the 
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2011, 2017 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/

/***********************************************************************************************************************
* File Name    : r_cg_dmac.c
* Version      : CodeGenerator for RL78/G13 V2.05.00.06 [10 Nov 2017]
* Device(s)    : R5F100LE
* Tool-Chain   : CCRL
* Description  : This file implements device driver for DMAC module.
* Creation Date: 
***********************************************************************************************************************/

/***********************************************************************************************************************
Includes
***********************************************************************************************************************/
#include "r_cg_macrodriver.h"
#include "r_cg_dmac.h"
/* Start user code for include. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
#include "r_cg_userdefine.h"

/***********************************************************************************************************************
Pragma directive
***********************************************************************************************************************/
/* Start user code for pragma. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

/***********************************************************************************************************************
Global variables and functions
***********************************************************************************************************************/
/* Start user code for global. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

/***********************************************************************************************************************
* Function Name: R_DMAC0_Create
* Description  : This function initializes the DMA0 transfer.
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/
void R_DMAC0_Create(void)
{
    DRC0 = _80_DMA_OPERATION_ENABLE;
    NOP();
    NOP();
    DMAMK0 = 1U; /* disable INTDMA0 interrupt */
    DMAIF0 = 0U; /* clear INTDMA0 interrupt flag */
    /* Set INTDMA0 low priority */
    DMAPR10 = 1U;
    DMAPR00 = 1U;
    DMC0 = _00_DMA_TRANSFER_DIR_SFR2RAM | _00_DMA_DATA_SIZE_8 | _07_DMA_TRIGGER_SR0_CSI01;
    DSA0 = _12_DMA0_SFR_ADDRESS;
    DRA0 = _EF00_DMA0_RAM_ADDRESS;
    DBC0 = _0000_DMA0_BYTE_COUNT;
    DEN0 = 0U; /* disable DMA0 operation */
}

/***********************************************************************************************************************
* Function Name: R_DMAC0_Start
* Description  : This function enables DMA0 transfer.
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/
void R_DMAC0_Start(void)
{
    DMAIF0 = 0U; /* clear INTDMA0 interrupt flag */
    DMAMK0 = 0U; /* enable INTDMA0 interrupt */
    DEN0 = 1U;
    DST0 = 1U;
}

/***********************************************************************************************************************
* Function Name: R_DMAC0_Stop
* Description  : This function disables DMA0 transfer.
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/
void R_DMAC0_Stop(void)
{
    if (DST0 != 0U)
    {
        DST0 = 0U;
    }
    
    NOP();
    NOP();
    DEN0 = 0U; /* disable DMA0 operation */
    DMAMK0 = 1U; /* disable INTDMA0 interrupt */
    DMAIF0 = 0U; /* clear INTDMA0 interrupt flag */
}

/***********************************************************************************************************************
* Function Name: R_DMAC1_Create
* Description  : This function initializes the DMA1 transfer.
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/
void R_DMAC1_Create(void)
{
    DRC1 = _80_DMA_OPERATION_ENABLE;
    NOP();
    NOP();
    DMAMK1 = 1U; /* disable INTDMA1 interrupt */
    DMAIF1 = 0U; /* clear INTDMA1 interrupt flag */
    /* Set INTDMA1 low priority */
    DMAPR11 = 1U;
    DMAPR01 = 1U;
    DMC1 = _40_DMA_TRANSFER_DIR_RAM2SFR | _00_DMA_DATA_SIZE_8 | _08_DMA_TRIGGER_ST1_CSI10;
    DSA1 = _44_DMA1_SFR_ADDRESS;
    DRA1 = _EF00_DMA1_RAM_ADDRESS;
    DBC1 = _0000_DMA1_BYTE_COUNT;
    DEN1 = 0U; /* disable DMA1 operation */
}

/***********************************************************************************************************************
* Function Name: R_DMAC1_Start
* Description  : This function enables DMA1 transfer.
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/
void R_DMAC1_Start(void)
{
    DMAIF1 = 0U; /* clear INTDMA1 interrupt flag */
    DMAMK1 = 0U; /* enable INTDMA1 interrupt */
    DEN1 = 1U;
    DST1 = 1U;
}

/***********************************************************************************************************************
* Function Name: R_DMAC1_Stop
* Description  : This function disables DMA1 transfer.
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/
void R_DMAC1_Stop(void)
{
    if (DST1 != 0U)
    {
        DST1 = 0U;
    }
    
    NOP();
    NOP();
    DEN1 = 0U; /* disable DMA1 operation */
    DMAMK1 = 1U; /* disable INTDMA1 interrupt */
    DMAIF1 = 0U; /* clear INTDMA1 interrupt flag */
}

/* Start user code for adding. Do not edit comment generated here */
void R_DMAC0_StartReceive(uint8_t *rxBuff, uint16_t rxCount)
{
	// DMA操作許可(DMAレジスタの操作に必要)
	DEN0 = 1U;
	// 受信先アドレス
	DRA0 = (uint16_t)rxBuff;
	// 受信カウント([15:10]は0固定)
    DBC0 = rxCount & 0x03FF;
	// 受信開始
	R_DMAC0_Start();
}

uint16_t R_DMAC0_GetRemainReceiveCount(void)
{
	return DBC0;
}

void R_DMAC1_StartSend(const uint8_t *txBuff, uint16_t txCount)
{
	// DMA操作許可(DMAレジスタの操作に必要)
	DEN1 = 1U;
	// 送信元アドレス
	DRA1 = (uint16_t)txBuff;
	// 送信カウント([15:10]は0固定)
    DBC1 = txCount & 0x03FF;
	// 送信開始
	R_DMAC1_Start();
	// 初回のみソフトウェアトリガONが必要。
	STG1 = 1U;
}

uint16_t R_DMAC1_GetRemainSendCount(void)
{
	return DBC1;
}
/* End user code. Do not edit comment generated here */
