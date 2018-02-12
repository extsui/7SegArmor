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

/************************************************************
 *  define
 ************************************************************/

/************************************************************
 *  global variables
 ************************************************************/
/** システムカウンタ(1msカウントアップ) */
static uint32_t g_SysTick = 0;

/** 1ms経過したか */
static BOOL g_Is1msElapsed = FALSE;

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
	
	R_IT_Start();
}

void Armor_mainLoop(void)
{
	while (1U) {
		// 1ms周期処理
		if (g_Is1msElapsed == TRUE) {
			// 周期処理中にSysTickは進むため一時退避する必要有り。
			uint8_t nowSysTick = g_SysTick;
			
			g_Is1msElapsed = FALSE;
			_1msCyclicProc();
			
			// 10ms周期処理
			if ((nowSysTick % 10) == 0) {
				_10msCyclicProc();

				// 100ms周期処理
				if ((nowSysTick % 100) == 0) {
					_100msCyclicProc();
				}
			}
		}
		
		R_WDT_Restart();
    }
}

void Armor_1msCyclicHandler(void)
{
	g_SysTick++;
	g_Is1msElapsed = TRUE;
}

void Armor_latchHandler(void)
{
	// TODO: Not Implemented.
}

/************************************************************
 *  private functions
 ************************************************************/
static void _1msCyclicProc(void)
{
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
	if (count > 5) {
		count = 0;
		P3_bit.no0 ^= 1;
	}
}
