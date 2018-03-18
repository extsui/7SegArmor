#include "r_cg_macrodriver.h"
#include "r_cg_userdefine.h"
#include "r_cg_serial.h" /* R_UART2_Receive() */
#include "r_cg_dmac.h"
#include "r_cg_timer.h"
#include "command.h"
#include "finger.h"
#include <stdio.h>
#include <string.h> /* memset() */
#include <stdlib.h> /* strtol() */

/************************************************************
 *  define
 ************************************************************/
// �e�X�g���{���ɗL�������邱�ƁB
//#define	TEST_COMMAND

#define COMMAND_BUF_SIZE	(256)

#define PATTERN_7SEG_0	(0xfc)
#define PATTERN_7SEG_1	(0x60)
#define PATTERN_7SEG_2	(0xda)
#define PATTERN_7SEG_3	(0xf2)
#define PATTERN_7SEG_4	(0x66)
#define PATTERN_7SEG_5	(0xb6)
#define PATTERN_7SEG_6	(0xbe)
#define PATTERN_7SEG_7	(0xe4)
#define PATTERN_7SEG_8	(0xfe)
#define PATTERN_7SEG_9	(0xf6)

static uint8_t g_CommandBuf[COMMAND_BUF_SIZE];
static uint16_t g_CommandBufIndex = 0;

// �L���R�}���h��M��
static uint32_t g_CommandRecvCount = 0;

// 1�R�}���h��M�����t���O
static BOOL g_IsCommandReceived = FALSE;

/** 7SegArmor�t���[�����M�����ۂ� */
/** ��DMA���M�ł͂Ȃ�������̃t���[�����M�S�̂��w�� */
static BOOL g_IsArmorFrameSending = FALSE;

/** 7SegArmor�t���[�����M�� */
static uint8_t g_ArmorFrameSendNum = 0;

/** 7SegArmor�t���[�����M�J�E���^ */
static uint8_t g_ArmorFrameSendCount = 0;

/** �R�}���h�����֐� */
typedef void (*cmdFnuc)(const char *s);

typedef struct {
	char	*name;	/**< �R�}���h�� */
	uint8_t	len;	/**< �R�}���h���̒���(�������̂��߂Ɏg�p) */
	cmdFnuc	fn;		/**< �R�}���h�����֐� */
	char	*info;	/**< ���(�w���v�R�}���h���ɕ\��) */
} Command;

static void cmdC0(const char *s);
static void cmdC1(const char *s);
static void cmdC2(const char *s);
static void cmdC3(const char *s);
static void cmdC4(const char *s);
static void cmdC5(const char *s);
static void cmdHelp(const char *s);
static void cmdVersion(const char *s);
static void cmdSet(const char *s);
static void cmdGet(const char *s);
static void cmdDebug(const char *s);
static void cmdReboot(const char *s);
static void cmdTest(const char *s);

static const Command g_CommandTable[] = {
	{ "C0",		2,	cmdC0,		"Get Info",					},
	{ "C1",		2,	cmdC1,		"Set Display",				},
	{ "C2",		2,	cmdC2,		"Set Brightness",			},
	{ "C3",		2,	cmdC3,		"Set Chain Display",		},
	{ "C4", 	2,	cmdC4,		"Set Chain Brightness",		},
	{ "C5",		2,	cmdC5,		"Update All Chain",			},
	{ "-h",		2,	cmdHelp,	"Help",						},
	{ "-v",		2,	cmdVersion,	"Version",					},
	{ "s",		1,	cmdSet,		"Set Param : s <p1> <p2>",	},
	{ "g",		1,	cmdGet,		"Get Param : g <p1>",		},
	{ "dbg",	3,	cmdDebug,	"Debug : dbg <p1> <p2>",	},
	{ "reb",	3,	cmdReboot,	"Reboot"					},
	{ "test",	4,	cmdTest,	"Test"						},
};

// TODO: armor.c�Ɉړ�����ׂ��B
static uint8_t data_all[FINGER_7SEG_NUM*FINGER_NUM];	// ���[�J���ϐ��ɂ���ƃX�^�b�N�I�[�o�[�t���[

// �����Ӂ�
// DMA�]���Ώۗ̈�͓���RAM�����ł���Aconst�̈�͓]���ł��Ȃ��B
static uint8_t test_data[] = {
	1,
	PATTERN_7SEG_0, PATTERN_7SEG_1, PATTERN_7SEG_2, PATTERN_7SEG_3, PATTERN_7SEG_4, PATTERN_7SEG_5, PATTERN_7SEG_6, PATTERN_7SEG_7,
	PATTERN_7SEG_8, PATTERN_7SEG_9, PATTERN_7SEG_0, PATTERN_7SEG_1, PATTERN_7SEG_2, PATTERN_7SEG_3, PATTERN_7SEG_4, PATTERN_7SEG_5,
	PATTERN_7SEG_6, PATTERN_7SEG_7, PATTERN_7SEG_8, PATTERN_7SEG_9, PATTERN_7SEG_0, PATTERN_7SEG_1, PATTERN_7SEG_2, PATTERN_7SEG_3,
	PATTERN_7SEG_4, PATTERN_7SEG_5, PATTERN_7SEG_6, PATTERN_7SEG_7, PATTERN_7SEG_8, PATTERN_7SEG_9, PATTERN_7SEG_0, PATTERN_7SEG_1,
	
	1,
	PATTERN_7SEG_2, PATTERN_7SEG_3, PATTERN_7SEG_4, PATTERN_7SEG_5, PATTERN_7SEG_6, PATTERN_7SEG_7, PATTERN_7SEG_8, PATTERN_7SEG_9,
	PATTERN_7SEG_0, PATTERN_7SEG_1, PATTERN_7SEG_2, PATTERN_7SEG_3, PATTERN_7SEG_4, PATTERN_7SEG_5, PATTERN_7SEG_6, PATTERN_7SEG_7,
	PATTERN_7SEG_8, PATTERN_7SEG_9, PATTERN_7SEG_0, PATTERN_7SEG_1, PATTERN_7SEG_2, PATTERN_7SEG_3, PATTERN_7SEG_4, PATTERN_7SEG_5,
	PATTERN_7SEG_6, PATTERN_7SEG_7, PATTERN_7SEG_8, PATTERN_7SEG_9, PATTERN_7SEG_0, PATTERN_7SEG_1, PATTERN_7SEG_2, PATTERN_7SEG_3,
	
	1,
	PATTERN_7SEG_4, PATTERN_7SEG_5, PATTERN_7SEG_6, PATTERN_7SEG_7, PATTERN_7SEG_8, PATTERN_7SEG_9, PATTERN_7SEG_0, PATTERN_7SEG_1,
	PATTERN_7SEG_2, PATTERN_7SEG_3, PATTERN_7SEG_4, PATTERN_7SEG_5, PATTERN_7SEG_6, PATTERN_7SEG_7, PATTERN_7SEG_8, PATTERN_7SEG_9,
	PATTERN_7SEG_0, PATTERN_7SEG_1, PATTERN_7SEG_2, PATTERN_7SEG_3, PATTERN_7SEG_4, PATTERN_7SEG_5, PATTERN_7SEG_6, PATTERN_7SEG_7,
	PATTERN_7SEG_8, PATTERN_7SEG_9, PATTERN_7SEG_0, PATTERN_7SEG_1, PATTERN_7SEG_2, PATTERN_7SEG_3, PATTERN_7SEG_4, PATTERN_7SEG_5,

	1,
	PATTERN_7SEG_6, PATTERN_7SEG_7, PATTERN_7SEG_8, PATTERN_7SEG_9, PATTERN_7SEG_0, PATTERN_7SEG_1, PATTERN_7SEG_2, PATTERN_7SEG_3,
	PATTERN_7SEG_4, PATTERN_7SEG_5, PATTERN_7SEG_6, PATTERN_7SEG_7, PATTERN_7SEG_8, PATTERN_7SEG_9, PATTERN_7SEG_0, PATTERN_7SEG_1,
	PATTERN_7SEG_2, PATTERN_7SEG_3, PATTERN_7SEG_4, PATTERN_7SEG_5, PATTERN_7SEG_6, PATTERN_7SEG_7, PATTERN_7SEG_8, PATTERN_7SEG_9,
	PATTERN_7SEG_0, PATTERN_7SEG_1, PATTERN_7SEG_2, PATTERN_7SEG_3, PATTERN_7SEG_4, PATTERN_7SEG_5, PATTERN_7SEG_6, PATTERN_7SEG_7,
};

/************************************************************
 *  prototype
 ************************************************************/
/**
 * �R�}���h��M����
 */
static void Command_onReceive(const char *rxBuff);

/**
 * �VSegArmor�t���[�����M�I������
 * @note DMA���M�������S���I�������̂�LATCH�������������{����B
 */
static void armorFrameSendendProc(void);

/**
 * 2������16�i��1�o�C�g�ɕϊ�����B
 * �G���[�����͍s��Ȃ��B
 */
static uint8_t atoh(uint8_t c_h, uint8_t c_l);

/**
 * �����p�����[�^���
 * 
 * ������s����num�̃p�����[�^����͂���params[]�Ɋi�[����B
 * �i�[�o��������߂�l�Ƃ��ĕԂ��B
 * �p�����[�^�͋󔒕���(" ")�ŋ�؂��Ă�����̂Ƃ���B
 * num���̉�͂��I�����邩�A��������'\0'��������I������B
 *
 * @param s ���͕�����(NULL�łȂ�����)
 * @param params ��͌��ʔz��(NULL�łȂ�����)
 * @param num ��͂����(0����)
 * @return ��͂ł�����
 * @note ��͉\�Ȓl��strtol()�Ŋ0�w�莞�̂��̂Ɍ���B
 * @note �܂��A��͌��ʂ�int�^�Ɋۂ߂��邱�Ƃɒ��ӁB
 * @note �����̑O��ɋL�ڂ������ڂ̓`�F�b�N���Ȃ��B
 */
static int getParam(const char *s, int params[], int num);

/************************************************************
 *  public functions
 ************************************************************/
void Command_init(void)
{
	g_CommandBufIndex = 0;
	
	// �㗬����̓d����������USB�쓮�\��LED���_�����Ă��܂��΍�
	// 
	// [�Ǐ�]
	// �㗬����̓d����������JP1-2�V���[�g��D1-LED���_������B
	// �����҂��铮���D2-LED�݂̂��_�����邱�ƁB
	//
	// [����]
	// TXD2(P13)��RXD2(P14)����H�d�����o�͂���Ă���A����2�[�q��
	// �o�R����RL78����FT232RL�ɓd������������Ă���(3V�㔼)�B
	//
	// [�΍�]
	// �N�����P13��P14�̓��͓d�������āA�ǂ����L�d����������
	// USB�d�����������Ɣ��f����UART2�����������Ȃ��B
	PM1_bit.no3 = 1U;	// P13����
	PM1_bit.no4 = 1U;	// P14����
	R_UART2_Stop();

	{
		// �ő����Ɣ���Ɏ��s����̂ŏ����f�B���C������B
		volatile uint32_t t;
		for (t = 0; t < 10000; t++) {
			NOP();
		}
	}
	
	if (!((P1_bit.no3 == 0U) && (P1_bit.no4 == 0U))) {
		// USB�d�������Ȃ̂Œ[�q�ݒ�����ɖ߂��B
		R_UART2_Create();
		R_UART2_Start();
	}
	R_UART2_Receive(&g_CommandBuf[g_CommandBufIndex], 1);
}

void Command_proc(void)
{
	if (g_IsCommandReceived == TRUE) {
		g_IsCommandReceived = FALSE;
		Command_onReceive((char*)g_CommandBuf);
		g_CommandBufIndex = 0;
		memset(g_CommandBuf, 0, COMMAND_BUF_SIZE);
		R_UART2_Receive(&g_CommandBuf[g_CommandBufIndex], 1);
	}
}

static void Command_onReceive(const char *rxBuff)
{
	int cmdIndex;
	for (cmdIndex = 0; cmdIndex < ARRAY_SIZE(g_CommandTable); cmdIndex++) {
		const Command *p = &g_CommandTable[cmdIndex];
		if (strncmp(p->name, rxBuff, p->len) == 0) {
			p->fn(&rxBuff[p->len]);
			break;
		}
	}

	// ��v���Ȃ������ꍇ
	if (cmdIndex == ARRAY_SIZE(g_CommandTable)) {
		// ��s�Ȃ�v�����v�g'>'��\��
		if (rxBuff[0] == '\n') {
			DPRINTF(">");
		// ��s�łȂ��Ȃ������R�}���h�Ƃ����|��\��
		} else {
			DPRINTF("Bad Type.\n");
		}
	}
}

/************************************************************/

static void cmdC0(const char *s)
{
	uint8_t dipsw;
	
	// DIPSW[3:0] = [ P120 | P43 | P42 | P41 ]�A���_��
	dipsw = (((uint8_t)P12_bit.no0 << 3) |
			 ((uint8_t)P4_bit.no3  << 2) |
			 ((uint8_t)P4_bit.no2  << 1) |
			 ((uint8_t)P4_bit.no1  << 0));
	dipsw = ~dipsw & 0x0F;
	DPRINTF("Get Mode 0x%02x\n", dipsw);
}

static void cmdC1(const char *s)
{
	uint8_t i;
	
	// �^: [ 'C', '1', (0, 0), (0, 1), ..., (3, 7) ]
	// ��: C1000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F\n
	for (i = 0; i < FINGER_7SEG_NUM*FINGER_NUM; i++) {
		data_all[i] = atoh(s[i*2], s[i*2 + 1]);
	}
	
	{
		static const uint8_t num_to_pattern[] = {
			0xfc, 0x60, 0xda, 0xf2, 0x66, 0xb6, 0xbe, 0xe4, 0xfe, 0xf6,
		};
		
		uint32_t temp = g_CommandRecvCount;
		for (i = 7; i > 0; i--) {
			data_all[i] = num_to_pattern[temp % 10];
			temp /= 10;
				if (temp == 0) {
				break;
			}
		}
	}
		
	Finger_setDisplayAll(data_all);
	g_CommandRecvCount++;
	DPRINTF("%d\n", g_CommandRecvCount);
}

static void cmdC2(const char *s)
{
	uint8_t i;
	
	// �^: [ 'C', '2', (0, 0), (0, 1), ..., (3, 7) ]
	// ��: C2000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F\n
	for (i = 0; i < FINGER_7SEG_NUM*FINGER_NUM; i++) {
		data_all[i] = atoh(s[i*2], s[i*2 + 1]);
	}
	Finger_setBrightnessAll(data_all);
	g_CommandRecvCount++;
	DPRINTF("%d\n", g_CommandRecvCount);
}

static void cmdC3(const char *s)
{
	
}

static void cmdC4(const char *s)
{
	// �A���P�x�ݒ�R�}���h
	DPRINTF("Not Implemented.\n");
}

static void cmdC5(const char *s)
{
	DPRINTF("Not Implemented.\n");
}

static void cmdHelp(const char *s)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(g_CommandTable); i++) {
		const Command *p = &g_CommandTable[i];
		DPRINTF("%s : %s\n", p->name, p->info);
	}
}

static void cmdVersion(const char *s)
{
	DPRINTF("7SegArmor ver.0.1\n");
}

static void cmdSet(const char *s)
{
	int i;
	int pnum = 0;
	int params[2];
	
	pnum = getParam(s, params, 2);
	
	DPRINTF("pnum = %d\n", pnum);
	for (i = 0; i < pnum; i++) {
		DPRINTF("params[%d] = %d\n", i, params[i]);
	}
	
}

static void cmdGet(const char *s)
{
	
	
	DPRINTF("Not Implemented.\n");
}

static void cmdDebug(const char *s)
{
	// �A���\���ݒ�R�}���h
	// TODO:
	// �ŏI�I�ɂ̓V���A������ݒ肵���f�[�^�𑗂�悤�ɂ���B
	// �J�����͌Œ�f�[�^�𑗂��ăf�o�b�O����B

	// <�ʐM�v���g�R��>
	// [0    ]: �Œ��1 (�\���R�}���hID)
	// [1..32]: 7�Z�O�\���f�[�^(���ォ��E���Ɍ��������Ɏw��)
	Finger_setDisplayAll(&test_data[1]);
			
	// TODO: �t���O�𒼐ڑ��삵�Ă͂����Ȃ��B�֐��Ń��b�s���O����ׂ��B
	// ��������1�t���[�������p�ӂ��Ȃ�����Ȃ����������H
	g_IsArmorFrameSending = TRUE;
	g_ArmorFrameSendNum = 4;
	g_ArmorFrameSendCount = 1;
	
	// 33�o�C�g��DMA���M@1MHz��270[us]
	R_DMAC1_StartSend((uint8_t *)&test_data[g_ArmorFrameSendCount*33], 33);
}

static void cmdReboot(const char *s)
{
	DPRINTF("Reboot after about 1sec. (by WDT)\n");
	while (1);
}

static void cmdTest(const char *s)
{
#ifdef TEST_COMMAND
	// �v���g�^�C�v�錾���O�ɏo���ƃe�X�g�ǉ�����
	// �C���ӏ������ł��܂��̂Ń��[�J���Ő錾����B
	void test_getParam(void);
	test_getParam();
#else
	DPRINTF("You should activate \"TEST_COMMAND\".\n");
#endif
}

/************************************************************/

void Command_receivedHandler(void)
{
	// ��M�����͎��������R�[�h���Ńo�b�t�@�Ɋi�[�ς�
	if (g_CommandBuf[g_CommandBufIndex] == '\n') {
		g_IsCommandReceived = TRUE;
	} else {
		g_CommandBufIndex++;
	}
	R_UART2_Receive(&g_CommandBuf[g_CommandBufIndex], 1);
}

void Command_masterSendendHandler(void)
{
	g_ArmorFrameSendCount++;
		
	// �t���[�����M��
	if (g_ArmorFrameSendCount <= g_ArmorFrameSendNum) {
		// [DMA�Ԓx������]
		// �����@���DMA�������荞�݂̏������ԕ��҂B
		// �EOK�F20us
		// �EOK�F15us
		// �ENG�F10us
		//   �˒x�����Ԃ́A20[us] ���Ă����Ζ��Ȃ��Ɣ��f�B
		R_TAU0_BusyWait(20);
		
		// ���̃t���[�����M
		R_DMAC1_StartSend((uint8_t *)&test_data[g_ArmorFrameSendCount*33], 33);
		
	// �ŏI�t���[�����M��
	} else {
		// [LATCH�X�V�x������]
		// DMA�]��1��+���̎��Ԃ��m�ۂ���K�v������B
		// �EDMA1�񗝘_�l�F33*8[bit] / 1[Mbps] = 264[us]
		// �EDMA1�񑪒�l�F267�`268[us]
		//   �˒x�����Ԃ́A300[us] ���Ă����Ζ��Ȃ��Ɣ��f�B
		R_TAU0_SetTimeout(300, armorFrameSendendProc);
	}
}

/************************************************************
 *  prvaite functions
 ************************************************************/
static void armorFrameSendendProc(void)
{
	// �ő�����INTC���荞�݂���肱�ڂ��̂Œx�����K�v�B
	P13_bit.no0 = 1;
	PULSE_DELAY();
	P13_bit.no0 = 0;
	
	g_IsArmorFrameSending = FALSE;
}

static uint8_t atoh(uint8_t c_h, uint8_t c_l)
{
	uint8_t h, l;
	uint8_t hex = 0;
	
	if (('a' <= c_h) && (c_h <= 'f')) {
		h = c_h - 'a' + 10;
	} else if (('A' <= c_h) && (c_h <= 'F')) {
		h = c_h - 'A' + 10;
	} else if (('0' <= c_h) && (c_h <= '9')) {
		h = c_h - '0';
	} else {
		/* error */
		h = 0x00;
	}
	
	if (('a' <= c_l) && (c_l <= 'f')) {
		l = c_l - 'a' + 10;
	} else if (('A' <= c_l) && (c_l <= 'F')) {
		l = c_l - 'A' + 10;
	} else if (('0' <= c_l) && (c_l <= '9')) {
		l = c_l - '0';
	} else {
		/* error */
		l = 0x00;
	}
	
	hex = (h << 4) | (l & 0x0F);
	return hex;
}

static int getParam(const char *s, int params[], int num)
{
	int ok_num = 0;
	char *startp = (char*)s;
	char *endp = NULL;
	long val = 0;
	
	do {
		val = strtol(startp, &endp, 0);
		// 1�����ȏ�ǂ߂Ă���Ȃ牽�������͂ł��Ă���B
		if (startp != endp) {
			params[ok_num] = (int)val;
			ok_num++;
		// ������͂ł��Ȃ��������Ǖ�����Ō�ł��Ȃ��B
		} else if (val == 0) {
			break;
		}
		startp = endp;
	} while ((endp != NULL) && (ok_num < num));
	
	return ok_num;
}

/************************************************************
 *  �e�X�g�R�[�h
 ************************************************************/
#ifdef TEST_COMMAND

void test_getParam(void)
{
	int params[2];
	int retval;
	
	DPRINTF("TEST: getParam()\n");
	
	DPRINTF("[����n] ����1�A0\n");
	retval = 0;
	memset(params, 0, sizeof(params));
	retval = getParam("1", params, 1);
	ASSERT(retval == 1);
	ASSERT(params[0] == 1);
	
	DPRINTF("[����n] ����1�A��0\n");
	retval = 0;
	memset(params, 0, sizeof(params));
	retval = getParam("1", params, 1);
	ASSERT(retval == 1);
	ASSERT(params[0] == 1);

	DPRINTF("[����n] ����1�A��������\n");
	retval = 0;
	memset(params, 0, sizeof(params));
	retval = getParam("123", params, 1);
	ASSERT(retval == 1);
	ASSERT(params[0] == 123);

	DPRINTF("[�ُ�n] ����1�A��������\n");
	retval = 0;
	memset(params, 0, sizeof(params));
	params[0] = 0xCC;
	retval = getParam("", params, 1);
	ASSERT(retval == 0);
	ASSERT(params[0] == 0xCC);

	DPRINTF("[�ُ�n] ����1�A���s�̂�\n");
	retval = 0;
	memset(params, 0, sizeof(params));
	params[0] = 0xCC;
	retval = getParam("\n", params, 1);
	ASSERT(retval == 0);
	ASSERT(params[0] == 0xCC);

	DPRINTF("[�ُ�n] ����1�A�s������\n");
	retval = 0;
	memset(params, 0, sizeof(params));
	params[0] = 0xCC;
	retval = getParam("$", params, 1);
	ASSERT(retval == 0);
	ASSERT(params[0] == 0xCC);
	
	DPRINTF("[����n] ����2�A�ŏ�������\n");
	retval = 0;
	memset(params, 0, sizeof(params));
	retval = getParam(" 1 2", params, 2);
	ASSERT(retval == 2);
	ASSERT(params[0] == 1);
	ASSERT(params[1] == 2);
	
	DPRINTF("[����n] ����2�A����������\n");
	retval = 0;
	memset(params, 0, sizeof(params));
	retval = getParam("123  456", params, 2);
	ASSERT(retval == 2);
	ASSERT(params[0] == 123);
	ASSERT(params[1] == 456);
	
	DPRINTF("[�ُ�n] ����2�A3�w��\n");
	retval = 0;
	memset(params, 0, sizeof(params));
	retval = getParam("123 456 $", params, 2);
	ASSERT(retval == 2);
	ASSERT(params[0] == 123);
	ASSERT(params[1] == 456);
	
	DPRINTF("TEST: OK!\n");
}

#endif /* TEST_COMMAND */
