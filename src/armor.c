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

/************************************************************
 *  global variables
 ************************************************************/
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

void Armor_proc(void)
{
	// LATCHトリガがかかったなら今のLATCHバッファの内容で
	// 7SegFingerの表示を更新する。
	// TODO: LATCHバッファのデータチェックが必要
	if (g_IsLatchTriggerReceived == TRUE) {
		g_IsLatchTriggerReceived = FALSE;
	
		if (g_IsUpdatableByLatch == TRUE) {
			g_IsUpdatableByLatch = FALSE;
		} else {
			DPRINTF("Error: 下流受信完了前にLATCHトリガが来た\n");
		}
		
		if (g_LatchBuffer[0] == 1) {
			Finger_setDisplayAll(&g_LatchBuffer[1]);
			DPRINTF("Slave Update.\n");
		} else {
			DPRINTF("Bad Command.\n");
		}
		
		memset(g_MasterReceiveBuffer, 0, sizeof(g_MasterReceiveBuffer));
		memset(g_LatchBuffer, 0, sizeof(g_LatchBuffer));
		memset(g_SlaveSendBuffer, 0, sizeof(g_SlaveSendBuffer));
		
		R_DMAC0_StartReceive(g_MasterReceiveBuffer, 33);
	}
}

/************************************************************
 *  private functions
 ************************************************************/
