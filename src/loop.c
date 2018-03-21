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
void Loop_init(void)
{
	R_IT_Start();
}

volatile static uint16_t elapsed_us = 0;

void Loop_main(void)
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

void Loop_1msCyclicHandler(void)
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

/************************************************************
 *  private functions
 ************************************************************/
static void _1msCyclicProc(void)
{
	Command_proc();
	Armor_proc();
	Finger_proc();
}

static void _10msCyclicProc(void)
{
	static int count10ms = 0;
	count10ms++;
	
	/* NOP */
}

static void _100msCyclicProc(void)
{
	static int count100ms = 0;	
	count100ms++;
	
	if (count100ms >= 5) {
		count100ms = 0;
		P3_bit.no0 ^= 1;
	}
}
