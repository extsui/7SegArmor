#include "r_cg_macrodriver.h"
#include "r_cg_userdefine.h"
#include "r_cg_wdt.h"
#include "r_cg_it.h"
#include "r_cg_serial.h"
#include "r_cg_intc.h"
#include "r_cg_dmac.h"
#include "r_cg_timer.h"
#include "command.h"
#include "armor.h"
#include "finger.h"
#include <stdio.h>
#include <string.h>

/************************************************************
 *  define
 ************************************************************/
/** 7SEG FINGER更新周期(ms) */
#define FINGER_UPDATE_INTERVAL_MS	(16)

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

/** 上流受信バッファ */
static uint8_t g_MasterReceiveBuffer[33];

/** LATCHバッファ */
// TODO: ARMORフレームサイズ⇒#define化
static uint8_t g_LatchBuffer[33];

/** 下流送信バッファ */
static uint8_t g_SlaveSendBuffer[33];

/** LATCHトリガ受信フラグ */
static BOOL g_IsLatchTriggerReceived = FALSE;

/** ラッチによる更新が可能か */
/** LATCHトリガが来た時に更新できるか否かの判断に使用する。 */
/** LATCHトリガ後にFALSEとなり、1フレーム以上受信するとTRUEになる。 */
static BOOL g_IsUpdatableByLatch = FALSE;

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
	R_DMAC0_StartReceive(g_MasterReceiveBuffer, 33);
	
	R_IT_Start();
}

volatile static uint16_t elapsed_us = 0;

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

void Armor_slaveReceiveendHandler(void)
{
	// 1回目の受信完了時はLATCHバッファに溜める。
	if (!g_IsUpdatableByLatch) {
		memcpy(g_LatchBuffer, g_MasterReceiveBuffer, 33);
		g_IsUpdatableByLatch = TRUE;
	// 2回目から下流へ送信するようにする。
	} else {
		memcpy(g_SlaveSendBuffer, g_MasterReceiveBuffer, 33);
		R_DMAC1_StartSend(g_SlaveSendBuffer, 33);
	}
	
	// LATCHトリガまで受信は継続する。
	R_DMAC0_StartReceive(g_MasterReceiveBuffer, 33);
}

void Armor_latchHandler(void)
{
	// LATCH信号が来たということはSPI送受信が
	// 完了しているはずなのでDMAを中断する。
	// TODO: この時点でDMAが動作している状況は異常なのでログ出力する。
	volatile uint16_t remainSendCount;
	volatile uint16_t remainReceiveCount;
	remainReceiveCount = R_DMAC0_GetRemainReceiveCount();
	remainSendCount = R_DMAC1_GetRemainSendCount();
	
	// 1ms周期処理で受信再開するので一回止める。
	R_DMAC0_Stop();
	
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
	static uint8_t count1ms = 0;
	count1ms++;
	
	Command_proc();
	
	// LATCHトリガがかかったなら今のLATCHバッファの内容で
	// 7SegFingerの表示を更新する。
	// TODO: LATCHバッファのデータチェックが必要
	if (g_IsLatchTriggerReceived == TRUE) {
		g_IsLatchTriggerReceived = FALSE;
	
		if (g_IsUpdatableByLatch == TRUE) {
			g_IsUpdatableByLatch = FALSE;
		} else {
			PRINTF("Error: 下流受信完了前にLATCHトリガが来た\n");
		}
		
		if (g_LatchBuffer[0] == 1) {
			Finger_setDisplayAll(&g_LatchBuffer[1]);
			PRINTF("Slave Update.\n");
		} else {
			PRINTF("Bad Command.\n");
		}
		
		memset(g_MasterReceiveBuffer, 0, sizeof(g_MasterReceiveBuffer));
		memset(g_LatchBuffer, 0, sizeof(g_LatchBuffer));
		memset(g_SlaveSendBuffer, 0, sizeof(g_SlaveSendBuffer));
		
		R_DMAC0_StartReceive(g_MasterReceiveBuffer, 33);
	}
	
	/*
	 * 7SegArmorが7SegFinger4個を更新するのに約1.6msかかる。
	 * この周期以上に設定する必要がある。
	 * そもそも7SegFinger自身の更新に2ms*8個=16msかかるので
	 * この周期と合わせておく。
	 */
	if (count1ms >= FINGER_UPDATE_INTERVAL_MS) {
		Finger_update();
	}
}

static void _10msCyclicProc(void)
{
	static uint8_t count10ms = 0;
	count10ms++;
	
	/* NOP */
}

static void _100msCyclicProc(void)
{
	static uint8_t count100ms = 0;	
	count100ms++;
	
	if (count100ms >= 5) {
		count100ms = 0;
		P3_bit.no0 ^= 1;
	}
}
