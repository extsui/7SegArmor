#include "r_cg_macrodriver.h"
#include "r_cg_userdefine.h"
#include "r_cg_serial.h"
#include "finger.h"
#include <string.h> /* memcpy() */

/************************************************************
 *
 *  ハード非依存箇所
 *
 ************************************************************/
/************************************************************
 *  define
 ************************************************************/
/** 7SegFingerコマンド用バッファ */
static uint8_t g_FingerCmdBuffer[FINGER_NUM][FINGER_CMD_SIZE];

/************************************************************
 *  prototype
 ************************************************************/
// 送信開始
static void beginSend(void);

/************************************************************
 *  public functions
 ************************************************************/
void Finger_init(void)
{
	R_CSI00_Start();
}

void Finger_setCommand(uint8_t index, const uint8_t *cmd)
{
	if (index >= FINGER_NUM) {
		return;
	}
	memcpy(g_FingerCmdBuffer[index], cmd, FINGER_CMD_SIZE);
}

void Finger_update(void)
{
	// TODO: 前回の送信中だったらはじく処理を追加する？
	beginSend();
}

/************************************************************
 *  private functions
 ************************************************************/
 
// TODO: 要修正。最適化によっても変化する。
void delay_us(uint16_t us)
{
	volatile int i, j;
	for (i = 0; i < us; i++) {
		for (j = 0; j < 5; j++) {
			NOP();
		}
	}
}

/************************************************************
 *
 *  ハード依存箇所
 *
 ************************************************************/
/************************************************************
 *  define
 ************************************************************/
// 7SEG_FINGER接続
#define FINGER_CS0		(P2_bit.no7)
#define FINGER_CS1		(P2_bit.no6)
#define FINGER_CS2		(P2_bit.no5)
#define FINGER_CS3		(P2_bit.no4)
#define FINGER_LATCH	(P14_bit.no7)

// 7SegFinger番号(0-3)
// コマンド送信に使用する。
static uint8_t g_FingerIndex = 0;

/************************************************************
 *  prototype
 ************************************************************/
static void assertCS(uint8_t cs);
static void negateCS(uint8_t cs);
static void setCS(uint8_t cs, uint8_t value);

/************************************************************
 *  public functions
 ************************************************************/
static void beginSend(void)
{
	g_FingerIndex = 0;
	assertCS(g_FingerIndex);
	delay_us(10);
	R_CSI00_Send(g_FingerCmdBuffer[g_FingerIndex], FINGER_CMD_SIZE);
}

void Finger_sendEndHandler(void)
{
	// WORKAROUND:
	// 送信完了割り込みなのか、送信アイドル割り込みなのかが未調査。
	// r_csi00_interrupt()を見た感じ送信完了割り込みのようなので、
	// 最終バイト送信分以上は最低でも待つ必要がある。
	// ⇒一応ウェイトを加えておく。
	delay_us(10);
	
	negateCS(g_FingerIndex);
	g_FingerIndex++;
	
	if (g_FingerIndex < FINGER_NUM) {
		assertCS(g_FingerIndex);
		delay_us(10);
		R_CSI00_Send(g_FingerCmdBuffer[g_FingerIndex], FINGER_CMD_SIZE);
	} else {
		// 最終への送信が完了したらLATCH信号を送信して完了
		delay_us(10);
		FINGER_LATCH = 1;
		delay_us(10);
		FINGER_LATCH = 0;
	}
}

/************************************************************
 *  private functions
 ************************************************************/
static void assertCS(uint8_t cs)
{
	setCS(cs, 0);
}

static void negateCS(uint8_t cs)
{
	setCS(cs, 1);
}

static void setCS(uint8_t cs, uint8_t value)
{
	switch (cs) {
	case 0:
		FINGER_CS0 = value;
		break;
	case 1:
		FINGER_CS1 = value;
		break;
	case 2:
		FINGER_CS2 = value;
		break;
	case 3:
		FINGER_CS3 = value;
		break;
	default:
		break;
	}
}
