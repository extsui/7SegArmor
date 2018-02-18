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
/** �V�X�e���J�E���^(1ms�J�E���g�A�b�v) */
static uint32_t g_SysTick = 0;

/** 1ms������(�f�o�b�O�p�B�V�X�e���J�E���^�Ƃ̍���1ms�����ɊԂɍ���Ȃ�������) */
static uint32_t g_1msCyclicProcCount = 0;

/** 1ms�o�߂����� */
static BOOL g_Is1msElapsed = FALSE;

/** 10ms�o�߂����� */
static BOOL g_Is10msElapsed = FALSE;

/** 100ms�o�߂����� */
static BOOL g_Is100msElapsed = FALSE;

/** LATCH�o�b�t�@ */
static uint8_t g_LatchBuffer[33];

/** LATCH�g���K��M�t���O */
static BOOL g_IsLatchTriggerReceived = FALSE;

/************************************************************
 *  prototype
 ************************************************************/
/** 1ms��������(���C���R���e�L�X�g) */
static void _1msCyclicProc(void);

/** 10ms��������(���C���R���e�L�X�g) */
static void _10msCyclicProc(void);

/** 100ms��������(���C���R���e�L�X�g) */
static void _100msCyclicProc(void);

/************************************************************
 *  public functions
 ************************************************************/
void Armor_init(void)
{
	R_CSI01_Start();
	R_CSI10_Start();
	R_INTC10_Start();
	
	// �}�X�^����̎�M����
	R_CSI01_Receive(g_LatchBuffer, 33);
	
	R_IT_Start();
}

void Armor_mainLoop(void)
{
	while (1U) {
		// 1ms��������
		if (g_Is1msElapsed == TRUE) {
			g_Is1msElapsed = FALSE;
			g_1msCyclicProcCount++;
			_1msCyclicProc();
		}
		
		// 10ms��������
		if (g_Is10msElapsed == TRUE) {
			g_Is10msElapsed = FALSE;
			_10msCyclicProc();
		}
		
		// 100ms��������
		if (g_Is100msElapsed == TRUE) {
			g_Is100msElapsed = FALSE;
			_100msCyclicProc();
		}
		
		R_WDT_Restart();
    }
}

void Armor_1msCyclicHandler(void)
{
	// ���荞�݃n���h����Ńt���O�X�V�������s�����߁A
	// �ȉ��̏�������g_SysTick�̒l�͕ϓ����Ȃ��B
	
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
	// �}�X�^�����LATCH�M�����X���[�u�ɗ����B
	// �ő�����INTC���荞�݂���肱�ڂ��̂Œx�����K�v�B
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
	// LATCH�g���K�����������Ȃ獡��LATCH�o�b�t�@�̓��e��
	// 7SegFinger�̕\�����X�V����B
	// TODO: LATCH�o�b�t�@�̃f�[�^�`�F�b�N���K�v
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
