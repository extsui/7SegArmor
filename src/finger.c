#include "r_cg_macrodriver.h"
#include "r_cg_userdefine.h"
#include "r_cg_serial.h"
#include "finger.h"
#include <string.h> /* memcpy() */

/************************************************************
 *
 *  �n�[�h��ˑ��ӏ�
 *
 ************************************************************/
/************************************************************
 *  define
 ************************************************************/
/** 7SegFinger�R�}���h�p�o�b�t�@ */
static uint8_t g_FingerCmdBuffer[FINGER_NUM][FINGER_CMD_SIZE];

/************************************************************
 *  prototype
 ************************************************************/
// ���M�J�n
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
	// TODO: �O��̑��M����������͂���������ǉ�����H
	beginSend();
}

/************************************************************
 *  private functions
 ************************************************************/
 
// TODO: �v�C���B�œK���ɂ���Ă��ω�����B
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
 *  �n�[�h�ˑ��ӏ�
 *
 ************************************************************/
/************************************************************
 *  define
 ************************************************************/
// 7SEG_FINGER�ڑ�
#define FINGER_CS0		(P2_bit.no7)
#define FINGER_CS1		(P2_bit.no6)
#define FINGER_CS2		(P2_bit.no5)
#define FINGER_CS3		(P2_bit.no4)
#define FINGER_LATCH	(P14_bit.no7)

// 7SegFinger�ԍ�(0-3)
// �R�}���h���M�Ɏg�p����B
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
	// ���M�������荞�݂Ȃ̂��A���M�A�C�h�����荞�݂Ȃ̂����������B
	// r_csi00_interrupt()�������������M�������荞�݂̂悤�Ȃ̂ŁA
	// �ŏI�o�C�g���M���ȏ�͍Œ�ł��҂K�v������B
	// �ˈꉞ�E�F�C�g�������Ă����B
	delay_us(10);
	
	negateCS(g_FingerIndex);
	g_FingerIndex++;
	
	if (g_FingerIndex < FINGER_NUM) {
		assertCS(g_FingerIndex);
		delay_us(10);
		R_CSI00_Send(g_FingerCmdBuffer[g_FingerIndex], FINGER_CMD_SIZE);
	} else {
		// �ŏI�ւ̑��M������������LATCH�M���𑗐M���Ċ���
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
