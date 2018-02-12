#include "r_cg_macrodriver.h"
#include "r_cg_userdefine.h"
#include "r_cg_serial.h"
#include "finger.h"
#include <string.h> /* memcpy() */
#include <stdio.h> /* printf() */

/************************************************************
 *
 *  �n�[�h��ˑ��ӏ�
 *
 ************************************************************/
/************************************************************
 *  define
 ************************************************************/
/** 7SEG_FINGER�Ǘ��\���� */
typedef struct {
	uint8_t display[FINGER_7SEG_NUM];		/**< �\���f�[�^ */
	uint8_t brightness[FINGER_7SEG_NUM];	/**< �P�x�f�[�^ */
} Finger;

/** 7SEG_FINGER�Ǘ��\���̃��X�g */
static Finger g_FingerList[FINGER_NUM];

/** 7SEG_FINGER�X�V�v��(�\���f�[�^���P�x�f�[�^���X�V�����ƗL���ɂȂ�) */
static BOOL g_IsReqUpdateFinger = TRUE;

/** 7SEG_FINGER�ɑ��M�\�� */
static BOOL g_IsSendableToFinger = TRUE;

/** �N�����̉�� */
static const uint8_t g_StartupDisplay[] = {
	0xAC, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0xE8,
	0x6C, 0x00, 0xE4, 0xB6, 0x9E, 0xBC, 0x00, 0x6C,
	0x6C, 0x00, 0xDA, 0xFC, 0x60, 0xE4, 0x00, 0x6C,
	0x5C, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x74
};

/** �N�����̋P�x */
static const uint8_t g_StartupBrightness[] = {
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
};

/************************************************************
 *  prototype
 ************************************************************/
static void Finger_setDisplay(Finger *f, const uint8_t *display);
static void Finger_setBrightness(Finger *f, const uint8_t *brightness);
 
/** �t���[�����M�J�n */
static void Frame_beginSend(void);

/** �t���[���X�V */
void Frame_update(void);

/************************************************************
 *  public functions
 ************************************************************/
void Finger_init(void)
{
	R_CSI00_Start();
	
	// �\���f�[�^�ݒ�
	Finger_setDisplayAll(g_StartupDisplay);
	
	// �P�x�f�[�^�ݒ�
	Finger_setBrightnessAll(g_StartupBrightness);
	
	// �X�V
	Finger_update();
}

void Finger_setDisplayAll(const uint8_t *displayAll)
{
	Finger_setDisplay(&g_FingerList[0], &displayAll[FINGER_7SEG_NUM*0]);
	Finger_setDisplay(&g_FingerList[1], &displayAll[FINGER_7SEG_NUM*1]);
	Finger_setDisplay(&g_FingerList[2], &displayAll[FINGER_7SEG_NUM*2]);
	Finger_setDisplay(&g_FingerList[3], &displayAll[FINGER_7SEG_NUM*3]);
}

void Finger_setBrightnessAll(const uint8_t *brightnessAll)
{
	Finger_setBrightness(&g_FingerList[0], &brightnessAll[FINGER_7SEG_NUM*0]);
	Finger_setBrightness(&g_FingerList[1], &brightnessAll[FINGER_7SEG_NUM*1]);
	Finger_setBrightness(&g_FingerList[2], &brightnessAll[FINGER_7SEG_NUM*2]);
	Finger_setBrightness(&g_FingerList[3], &brightnessAll[FINGER_7SEG_NUM*3]);
}

void Finger_update(void)
{
	// �X�V�v���L��ő��M�\�̏ꍇ�A���M�J�n�B
	if ((g_IsReqUpdateFinger == TRUE) && (g_IsSendableToFinger == TRUE)) {
		g_IsReqUpdateFinger = FALSE;
		g_IsSendableToFinger = FALSE;
		Frame_beginSend();
	}
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

static void Finger_setDisplay(Finger *f, const uint8_t *display)
{
	memcpy(f->display, display, FINGER_7SEG_NUM);
	g_IsReqUpdateFinger = TRUE;
}

static void Finger_setBrightness(Finger *f, const uint8_t *brightness)
{
	memcpy(f->brightness, brightness, FINGER_7SEG_NUM);
	g_IsReqUpdateFinger = TRUE;
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

/************************************************************
 *  prototype
 ************************************************************/
static void makeDisplayFrame(const uint8_t *display);
static void makeBrightnessFrame(const uint8_t *brightness);
static void assertCS(uint8_t cs);
static void negateCS(uint8_t cs);
static void setCS(uint8_t cs, uint8_t value);

/************************************************************
 *  public functions
 ************************************************************/
static uint8_t g_SendIndex = 0;	// 0�`FINGER_7SEG_NUM-1
static uint8_t g_SendItem = 0;	// 0:display / 1:brightness
static uint8_t g_SendBuf[FINGER_FRAME_SIZE];

/**
 * �t���[�����M
 *
 * �ȉ��̎菇��7SEG_FINGER��K�v�ŏ����̂��̂����X�V����B
 * (1) [0]�̕\��
 * (2) [0]�̋P�x
 * (3) [1]�̕\��
 * (4) [1]�̋P�x
 * (5) [2]�̕\��
 * ...
 */
static void Frame_beginSend(void)
{
	/* �ŏ��̕\���t���[�����M�J�n(���C���R���e�L�X�g����Ăяo�����) */
	if ((g_SendIndex == 0) && (g_SendItem == 0)) {
		makeDisplayFrame(g_FingerList[g_SendIndex].display);
		g_SendItem++;
		
		assertCS(g_SendIndex);
		delay_us(10);
		R_CSI00_Send(g_SendBuf, FINGER_FRAME_SIZE);
		return;
		
	/* �\���t���[�����M�����ˋP�x�t���[�����M�J�n(���荞�݃n���h������Ăяo�����) */
	} else if ((g_SendIndex < FINGER_NUM) && (g_SendItem == 1)) {
		delay_us(10);
		negateCS(g_SendIndex);
		delay_us(10);
	
		makeBrightnessFrame(g_FingerList[g_SendIndex].brightness);
		g_SendItem = 0;
		
		assertCS(g_SendIndex);
		delay_us(10);
		R_CSI00_Send(g_SendBuf, FINGER_FRAME_SIZE);
		
		g_SendIndex++;
		return;
	
	/* �P�x�t���[�����M�����ˎ��̕\���t���[�����M�J�n(���荞�݃n���h������Ăяo�����) */
	} else if ((g_SendIndex < FINGER_NUM) && g_SendItem == 0) {
		delay_us(10);
		negateCS(g_SendIndex - 1);
		delay_us(10);

		makeDisplayFrame(g_FingerList[g_SendIndex].display);
		g_SendItem++;
		
		assertCS(g_SendIndex);
		delay_us(10);
		R_CSI00_Send(g_SendBuf, FINGER_FRAME_SIZE);
		return;
		
	/* �Ō�̋P�x�t���[�����M����(���荞�݃n���h������Ăяo�����) */
	} else if (g_SendIndex >= FINGER_NUM) {
		delay_us(10);
		negateCS(g_SendIndex - 1);
		delay_us(10);

		FINGER_LATCH = 1;
		FINGER_LATCH = 0;
	
		delay_us(10);
		
		g_SendIndex = 0;
		g_SendItem = 0;
		g_IsSendableToFinger = TRUE;
		return;
	
	} else {
		/* �����ɂ͗��Ȃ��͂� */
		printf("#");
	}
}

void Finger_sendEndHandler(void)
{
	Frame_beginSend();
}

static void makeDisplayFrame(const uint8_t *display)
{
	g_SendBuf[0] = FINGER_FRAME_TYPE_DISPLAY;
	memcpy(&g_SendBuf[1], display, FINGER_7SEG_NUM);
}

static void makeBrightnessFrame(const uint8_t *brightness)
{
	g_SendBuf[0] = FINGER_FRAME_TYPE_BRIGHTNESS;
	memcpy(&g_SendBuf[1], brightness, FINGER_7SEG_NUM);
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
