#include "r_cg_macrodriver.h"
#include "r_cg_userdefine.h"
#include "r_cg_wdt.h"
#include "r_cg_it.h"
#include "r_cg_serial.h"
#include "r_cg_intc.h"
#include "command.h"
#include "armor.h"
#include "finger.h"
#include <stdio.h>
#include <string.h>

/************************************************************
 *  define
 ************************************************************/

/************************************************************
 *  global variables
 ************************************************************/
/** システムカウンタ(1msカウントアップ) */
static uint32_t g_SysTick = 0;

/** 1ms処理回数(デバッグ用。システムカウンタとの差が1ms周期に間に合わなかった回数) */
static uint32_t g_1msCyclicProcCount = 0;

/** 1ms経過したか */
static BOOL g_Is1msElapsed = FALSE;

/** 10ms経過したか */
static BOOL g_Is10msElapsed = FALSE;

/** 100ms経過したか */
static BOOL g_Is100msElapsed = FALSE;

/** LATCHバッファ */
static uint8_t g_LatchBuffer[33];

/** LATCHトリガ受信フラグ */
static BOOL g_IsLatchTriggerReceived = FALSE;

/************************************************************
 *  prototype
 ************************************************************/
/** 1ms周期処理(メインコンテキスト) */
static void _1msCyclicProc(void);

/** 10ms周期処理(メインコンテキスト) */
static void _10msCyclicProc(void);

/** 100ms周期処理(メインコンテキスト) */
static void _100msCyclicProc(void);

/************************************************************
 *  public functions
 ************************************************************/
void Armor_init(void)
{
	R_CSI01_Start();
	R_CSI10_Start();
	R_INTC10_Start();
	
	// マスタからの受信準備
	R_CSI01_Receive(g_LatchBuffer, 33);
	
	R_IT_Start();
}

void Armor_mainLoop(void)
{
	while (1U) {
		// 1ms周期処理
		if (g_Is1msElapsed == TRUE) {
			g_Is1msElapsed = FALSE;
			g_1msCyclicProcCount++;
			_1msCyclicProc();
		}
		
		// 10ms周期処理
		if (g_Is10msElapsed == TRUE) {
			g_Is10msElapsed = FALSE;
			_10msCyclicProc();
		}
		
		// 100ms周期処理
		if (g_Is100msElapsed == TRUE) {
			g_Is100msElapsed = FALSE;
			_100msCyclicProc();
		}
		
		R_WDT_Restart();
    }
}

void Armor_1msCyclicHandler(void)
{
	// 割り込みハンドラ上でフラグ更新処理を行うため、
	// 以下の処理内でg_SysTickの値は変動しない。
	
	g_SysTick++;
	g_Is1msElapsed = TRUE;
	
	if ((g_SysTick % 10) == 0) {
		g_Is10msElapsed = TRUE;
		
		if ((g_SysTick % 100) == 0) {
			g_Is100msElapsed = TRUE;
		}
	}
}

void Armor_latchHandler(void)
{
	// マスタからのLATCH信号をスレーブに流す。
	// 最速だとINTC割り込みを取りこぼすので遅延が必要。
	P13_bit.no0 = 1;
	PULSE_DELAY();
	P13_bit.no0 = 0;
	
	g_IsLatchTriggerReceived = TRUE;
}

/************************************************************
 *  private functions
 ************************************************************/
static void _1msCyclicProc(void)
{
	// LATCHトリガがかかったなら今のLATCHバッファの内容で
	// 7SegFingerの表示を更新する。
	// TODO: LATCHバッファのデータチェックが必要
	if (g_IsLatchTriggerReceived == TRUE) {
		g_IsLatchTriggerReceived = FALSE;
		
		if (g_LatchBuffer[0] == 1) {
			Finger_setDisplayAll(&g_LatchBuffer[1]);
			PRINTF("Slave Update.\n");
		} else {
			PRINTF("Bad Command.\n");
		}
		memset(g_LatchBuffer, 0, sizeof(g_LatchBuffer));
	}
	
	Command_proc();
	Finger_update();
}

static void _10msCyclicProc(void)
{
	/* NOP */
}

static void _100msCyclicProc(void)
{
	static volatile uint8_t count = 0;
	
	count++;
	if (count >= 5) {
		count = 0;
		P3_bit.no0 ^= 1;
	}
}
