#include "r_cg_macrodriver.h"
#include "r_cg_userdefine.h"
#include "r_cg_serial.h" /* R_UART2_Receive() */
#include "command.h"
#include "finger.h"
#include <stdio.h>
#include <string.h> /* memset() */

/************************************************************
 *  define
 ************************************************************/

#define CMD_MAGIC_OFFSET	(0)
#define CMD_TYPE_OFFSET		(1)
#define CMD_DATA_OFFSET		(2)

#define COMMAND_BUF_SIZE	(256)

static uint8_t g_CommandBuf[COMMAND_BUF_SIZE];
static uint16_t g_CommandBufIndex = 0;

// �L���R�}���h��M��
static uint32_t g_CommandRecvCount = 0;

// 1�R�}���h��M�����t���O
static BOOL g_IsCommandReceived = FALSE;

/************************************************************
 *  prototype
 ************************************************************/
/**
 * �R�}���h��M����
 */
static void Command_onReceive(const uint8_t *command);

/**
 * 2������16�i��1�o�C�g�ɕϊ�����B
 * �G���[�����͍s��Ȃ��B
 */
static uint8_t atoh(uint8_t c_h, uint8_t c_l);

/************************************************************
 *  public functions
 ************************************************************/
void Command_init(void)
{
	g_CommandBufIndex = 0;
    R_UART2_Start();
	R_UART2_Receive(&g_CommandBuf[g_CommandBufIndex], 1);
}

void Command_proc(void)
{
	if (g_IsCommandReceived == TRUE) {
		g_IsCommandReceived = FALSE;
		Command_onReceive(g_CommandBuf);
		g_CommandBufIndex = 0;
		memset(g_CommandBuf, 0, COMMAND_BUF_SIZE);
		R_UART2_Receive(&g_CommandBuf[g_CommandBufIndex], 1);
	}
}

static void Command_onReceive(const uint8_t *command)
{
	int i;
	uint8_t type;
	uint8_t dipsw;
	static uint8_t data_all[FINGER_7SEG_NUM*FINGER_NUM];	// ���[�J���ϐ��ɂ���ƃX�^�b�N�I�[�o�[�t���[
	
	if (command[CMD_MAGIC_OFFSET] != 'S') {
		return;
	}
	
	type = command[CMD_TYPE_OFFSET] - '0';
	switch (type) {
	case ARMOR_CMD_GET_MODE:
		// DIPSW[3:0] = [ P120 | P43 | P42 | P41 ]�A���_��
		dipsw = (((uint8_t)P12_bit.no0 << 3) |
				 ((uint8_t)P4_bit.no3  << 2) |
				 ((uint8_t)P4_bit.no2  << 1) |
				 ((uint8_t)P4_bit.no1  << 0));
		dipsw = ~dipsw & 0x0F;
		PRINTF("Get Mode 0x%02x\n", dipsw);
		break;
		
	case ARMOR_CMD_SET_DISPLAY:
		// �^: [ 'S', '1', (0, 0), (0, 1), ..., (3, 7) ]
		// ��: S1000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F\n
		for (i = 0; i < FINGER_7SEG_NUM*FINGER_NUM; i++) {
			data_all[i] = atoh(command[CMD_DATA_OFFSET + (i*2)], command[CMD_DATA_OFFSET + (i*2) + 1]);
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
		printf("%d\n", g_CommandRecvCount);
		break;
		
	case ARMOR_CMD_SET_BRIGHTNESS:
		// �^: [ 'S', '2', (0, 0), (0, 1), ..., (3, 7) ]
		// ��: S2000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F\n
		for (i = 0; i < FINGER_7SEG_NUM*FINGER_NUM; i++) {
			data_all[i] = atoh(command[CMD_DATA_OFFSET + (i*2)], command[CMD_DATA_OFFSET + (i*2) + 1]);
		}
		Finger_setBrightnessAll(data_all);
		g_CommandRecvCount++;
		printf("%d\n", g_CommandRecvCount);
		break;
		
	case 9:
		// CSI10: ���M�@�\
		//R_CSI10_Send();
		break;
		
	default:
		printf("Bad Type.\n");
		break;
	}
}

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
