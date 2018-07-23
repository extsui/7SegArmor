#include "r_cg_macrodriver.h"
#include "r_cg_userdefine.h"
#include "r_cg_serial.h" /* R_UART2_Receive() */
#include "r_cg_dmac.h"
#include "r_cg_timer.h"
#include "command.h"
#include "armor.h"
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

static uint8_t g_RecvBuf[COMMAND_BUF_SIZE];
static uint16_t g_RecvBufIndex = 0;
static uint8_t g_CommandBuf[COMMAND_BUF_SIZE];

// �L���R�}���h��M��
static uint32_t g_CommandRecvCount = 0;

// 1�R�}���h��M�����t���O
static BOOL g_IsCommandReceived = FALSE;

/** �R�}���h�����֐� */
typedef void (*cmdFnuc)(const char *s);

typedef struct {
	char	*name;	/**< �R�}���h�� */
	uint8_t	len;	/**< �R�}���h���̒���(�������̂��߂Ɏg�p) */
	cmdFnuc	fn;		/**< �R�}���h�����֐� */
	char	*info;	/**< ���(�w���v�R�}���h���ɕ\��) */
} Command;

static void cmdInstruct(const char *s);
static void cmdUpdate(const char *s);
static void cmdHelp(const char *s);
static void cmdVersion(const char *s);
static void cmdSwitch(const char *s);
static void cmdSet(const char *s);
static void cmdGet(const char *s);
static void cmdDebug(const char *s);
static void cmdReboot(const char *s);
static void cmdTest(const char *s);

static const Command g_CommandTable[] = {
	{ "#0",		2,	cmdInstruct,"Instruct",					},
	{ "#1",		2,	cmdUpdate,	"Update",					},
	{ "-h",		2,	cmdHelp,	"Help",						},
	{ "-v",		2,	cmdVersion,	"Version",					},
	{ "sw",		2,	cmdSwitch,	"Get DIP Switch",			},
	{ "s",		1,	cmdSet,		"Set Param : s <p1> <p2>",	},
	{ "g",		1,	cmdGet,		"Get Param : g <p1>",		},
	{ "dbg",	3,	cmdDebug,	"Debug : dbg <p1> <p2>",	},
	{ "reb",	3,	cmdReboot,	"Reboot"					},
	{ "test",	4,	cmdTest,	"Test"						},
};

/************************************************************
 *  prototype
 ************************************************************/
/**
 * �R�}���h��M����
 */
static void Command_onReceive(const char *rxBuff);

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

	g_RecvBufIndex = 0;
	R_UART2_Receive(&g_RecvBuf[g_RecvBufIndex], 1);
}

void Command_proc(void)
{
	if (g_IsCommandReceived == TRUE) {
		g_IsCommandReceived = FALSE;
		Command_onReceive((char*)g_CommandBuf);
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

void Command_receivedHandler(void)
{
	// ��M�����͎��������R�[�h���Ńo�b�t�@�Ɋi�[�ς�
	// [����]
	// �����P�����̂��߁A�ُ�n(�o�b�t�@�I�[�o�[�t���[)�΍��
	// ���Ă��Ȃ��̂ŁA\n���Ȃ��Ђ����璷��������͋֎~�B
	if (g_RecvBuf[g_RecvBufIndex] == '\n') {
		// ��s���R�}���h�Ƃ��Č��Ȃ��̂ŉ��s���܂߂ăR�s�[
		memcpy(g_CommandBuf, g_RecvBuf, g_RecvBufIndex + 1);
		g_IsCommandReceived = TRUE;
		g_RecvBufIndex = 0;
	} else {
		g_RecvBufIndex++;
	}
	// ��M�f�[�^�𗎂Ƃ��Ȃ��悤�Ɏ�M�͑��p������
	R_UART2_Receive(&g_RecvBuf[g_RecvBufIndex], 1);
}

/************************************************************/

static void cmdInstruct(const char *s)
{
	static uint8_t data_all[36];	// ���[�J���ϐ��ɂ���ƃX�^�b�N�I�[�o�[�t���[
	uint8_t i;
	
	// ��: #001FFFFFFFFFFFFFFFF01FFFFFFFFFFFFFFFF01FFFFFFFFFFFFFFFF01FFFFFFFFFFFFFFFF\n
	// ��"#0"�����������̂�����s�ɃZ�b�g����Ă���
	for (i = 0; i < 36; i++) {
		data_all[i] = atoh(s[i*2], s[i*2 + 1]);
	}
	Armor_uartReceiveend(data_all);
	g_CommandRecvCount++;
	DPRINTF("%d\n", g_CommandRecvCount);
}

static void cmdUpdate(const char *s)
{
	Armor_uartLatch();
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

static void cmdSwitch(const char *s)
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
	
	if (pnum != 2) {
		return;
	}
}

static void cmdGet(const char *s)
{
	DPRINTF("Not Implemented.\n");
}

static void cmdDebug(const char *s)
{
	int i;
	int pnum = 0;
	int params[2];
	
	pnum = getParam(s, params, 2);
	
	DPRINTF("pnum = %d\n", pnum);
	for (i = 0; i < pnum; i++) {
		DPRINTF("params[%d] = %d\n", i, params[i]);
	}
	
	if (pnum != 2) {
		return;
	}
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

/************************************************************
 *  prvaite functions
 ************************************************************/
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
