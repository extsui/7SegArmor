#include "r_cg_macrodriver.h"
#include "r_cg_userdefine.h"
#include "r_cg_wdt.h"
#include "r_cg_it.h"
#include "r_cg_serial.h"
#include "r_cg_intc.h"
#include "r_cg_dmac.h"
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

/** �㗬��M�o�b�t�@ */
static uint8_t g_MasterReceiveBuffer[33];

/** LATCH�o�b�t�@ */
// TODO: ARMOR�t���[���T�C�Y��#define��
static uint8_t g_LatchBuffer[33];

/** �������M�o�b�t�@ */
static uint8_t g_SlaveSendBuffer[33];

/** LATCH�g���K��M�t���O */
static BOOL g_IsLatchTriggerReceived = FALSE;

/** ���b�`�ɂ��X�V���\�� */
/** LATCH�g���K���������ɍX�V�ł��邩�ۂ��̔��f�Ɏg�p����B */
/** LATCH�g���K���FALSE�ƂȂ�A1�t���[���ȏ��M�����TRUE�ɂȂ�B */
static BOOL g_IsUpdatableByLatch = FALSE;

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
	R_DMAC0_StartReceive(g_MasterReceiveBuffer, 33);
	
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

void Armor_slaveReceiveendHandler(void)
{
	// 1��ڂ̎�M��������LATCH�o�b�t�@�ɗ��߂�B
	if (!g_IsUpdatableByLatch) {
		memcpy(g_LatchBuffer, g_MasterReceiveBuffer, 33);
		g_IsUpdatableByLatch = TRUE;
	// 2��ڂ��牺���֑��M����悤�ɂ���B
	} else {
		memcpy(g_SlaveSendBuffer, g_MasterReceiveBuffer, 33);
		R_DMAC1_StartSend(g_SlaveSendBuffer, 33);
	}
	
	// LATCH�g���K�܂Ŏ�M�͌p������B
	R_DMAC0_StartReceive(g_MasterReceiveBuffer, 33);
}

void Armor_latchHandler(void)
{
	// LATCH�M���������Ƃ������Ƃ�SPI����M��
	// �������Ă���͂��Ȃ̂�DMA�𒆒f����B
	// TODO: ���̎��_��DMA�����삵�Ă���󋵂ُ͈�Ȃ̂Ń��O�o�͂���B
	volatile uint16_t remainSendCount;
	volatile uint16_t remainReceiveCount;
	remainReceiveCount = R_DMAC0_GetRemainReceiveCount();
	remainSendCount = R_DMAC1_GetRemainSendCount();
	
	// 1ms���������Ŏ�M�ĊJ����̂ň��~�߂�B
	R_DMAC0_Stop();
	
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
	
		if (g_IsUpdatableByLatch == TRUE) {
			g_IsUpdatableByLatch = FALSE;
		} else {
			PRINTF("Error: ������M�����O��LATCH�g���K������\n");
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
