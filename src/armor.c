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

void Armor_proc(void)
{
	// LATCH�g���K�����������Ȃ獡��LATCH�o�b�t�@�̓��e��
	// 7SegFinger�̕\�����X�V����B
	// TODO: LATCH�o�b�t�@�̃f�[�^�`�F�b�N���K�v
	if (g_IsLatchTriggerReceived == TRUE) {
		g_IsLatchTriggerReceived = FALSE;
	
		if (g_IsUpdatableByLatch == TRUE) {
			g_IsUpdatableByLatch = FALSE;
		} else {
			DPRINTF("Error: ������M�����O��LATCH�g���K������\n");
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
