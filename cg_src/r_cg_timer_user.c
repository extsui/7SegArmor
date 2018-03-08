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
* File Name    : r_cg_timer_user.c
* Version      : CodeGenerator for RL78/G13 V2.05.00.06 [10 Nov 2017]
* Device(s)    : R5F100LE
* Tool-Chain   : CCRL
* Description  : This file implements device driver for TAU module.
* Creation Date: 
***********************************************************************************************************************/

/***********************************************************************************************************************
Includes
***********************************************************************************************************************/
#include "r_cg_macrodriver.h"
#include "r_cg_timer.h"
/* Start user code for include. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
#include "r_cg_userdefine.h"

/***********************************************************************************************************************
Pragma directive
***********************************************************************************************************************/
#pragma interrupt r_tau0_channel2_interrupt(vect=INTTM02)
/* Start user code for pragma. Do not edit comment generated here */
static timeout_fn_t timeout_fn = NULL;
/* End user code. Do not edit comment generated here */

/***********************************************************************************************************************
Global variables and functions
***********************************************************************************************************************/
/* Start user code for global. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

/***********************************************************************************************************************
* Function Name: r_tau0_channel2_interrupt
* Description  : This function is INTTM02 interrupt service routine.
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/
static void __near r_tau0_channel2_interrupt(void)
{
    /* Start user code. Do not edit comment generated here */
	ASSERT(timeout_fn != NULL);
	timeout_fn();
	timeout_fn = NULL;
	R_TAU0_Channel2_Stop();
    /* End user code. Do not edit comment generated here */
}

/* Start user code for adding. Do not edit comment generated here */
// タイムアウトタイマ設定のオーバヘッド(us)
// 「R_TAU0_SetTimeout()直前～タイムアウトハンドラ」を測定すると
// (usec + 1～2)付近の値になる。割り込み事象発生から割り込み
// ハンドラ先頭までに1～2usかかるので以下の値で丁度良いと推測する。
#define TIMEOUT_TIMER_OVEHAD_US	(4)

/**
 * 指定時間(us)後に指定した関数を1回だけ呼び出す
 * @param usec 全範囲(0～65535us)有効
 * @param fn 呼び出す関数
 * @note オーバヘッドにより事実上の最小値はTIMEOUT_TIMER_OVEHAD_US。
 * @note 占有が前提なので二重呼び出しでASSERTする。
 */
void R_TAU0_SetTimeout(uint16_t usec, timeout_fn_t fn)
{
	ASSERT(timeout_fn == NULL);
	ASSERT(fn != NULL);
	timeout_fn = fn;
	if (usec > TIMEOUT_TIMER_OVEHAD_US) {
		TDR02 = usec - TIMEOUT_TIMER_OVEHAD_US;
	} else {
		TDR02 = 0;
	}
	R_TAU0_Channel2_Start();
}
/* End user code. Do not edit comment generated here */
