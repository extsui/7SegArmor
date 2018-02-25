#include "r_cg_macrodriver.h"
#include "r_cg_userdefine.h"
#include "r_cg_serial.h" /* R_UART2_Receive() */
#include "r_cg_dmac.h"
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

// 有効コマンド受信回数
static uint32_t g_CommandRecvCount = 0;

// 1コマンド受信完了フラグ
static BOOL g_IsCommandReceived = FALSE;

/** マスタ送信完了フラグ */
/** TODO: armor.cに移動すること */
static BOOL g_IsMasterSendend = FALSE;

/************************************************************
 *  prototype
 ************************************************************/
/**
 * コマンド受信処理
 */
static void Command_onReceive(const uint8_t *command);

/**
 * 2文字を16進数1バイトに変換する。
 * エラー処理は行わない。
 */
static uint8_t atoh(uint8_t c_h, uint8_t c_l);

/************************************************************
 *  public functions
 ************************************************************/
void Command_init(void)
{
	g_CommandBufIndex = 0;
	
	// 上流からの電源供給時にUSB駆動表示LEDが点灯してしまう対策
	// 
	// [症状]
	// 上流からの電源供給時にJP1-2ショートでD1-LEDが点灯する。
	// ※期待する動作はD2-LEDのみが点灯すること。
	//
	// [原因]
	// TXD2(P13)とRXD2(P14)からH電圧が出力されており、この2端子を
	// 経由してRL78からFT232RLに電源が供給されている(3V後半)。
	//
	// [対策]
	// 起動後にP13とP14の入力電圧を見て、どちらもL電圧だったら
	// USB電源供給無しと判断してUART2を初期化しない。
	PM1_bit.no3 = 1U;	// P13入力
	PM1_bit.no4 = 1U;	// P14入力
	R_UART2_Stop();

	{
		// 最速だと判定に失敗するので少しディレイを入れる。
		volatile uint32_t t;
		for (t = 0; t < 10000; t++) {
			NOP();
		}
	}
	
	if (!((P1_bit.no3 == 0U) && (P1_bit.no4 == 0U))) {
		// USB電源供給なので端子設定を元に戻す。
		R_UART2_Create();
		R_UART2_Start();
	}
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
	static uint8_t data_all[FINGER_7SEG_NUM*FINGER_NUM];	// ローカル変数にするとスタックオーバーフロー
	
	if (command[CMD_MAGIC_OFFSET] != 'S') {
		return;
	}
	
	type = command[CMD_TYPE_OFFSET] - '0';
	switch (type) {
	case ARMOR_CMD_GET_MODE:
		// DIPSW[3:0] = [ P120 | P43 | P42 | P41 ]、負論理
		dipsw = (((uint8_t)P12_bit.no0 << 3) |
				 ((uint8_t)P4_bit.no3  << 2) |
				 ((uint8_t)P4_bit.no2  << 1) |
				 ((uint8_t)P4_bit.no1  << 0));
		dipsw = ~dipsw & 0x0F;
		PRINTF("Get Mode 0x%02x\n", dipsw);
		break;
		
	case ARMOR_CMD_SET_DISPLAY:
		// 型: [ 'S', '1', (0, 0), (0, 1), ..., (3, 7) ]
		// 例: S1000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F\n
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
		// 型: [ 'S', '2', (0, 0), (0, 1), ..., (3, 7) ]
		// 例: S2000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F\n
		for (i = 0; i < FINGER_7SEG_NUM*FINGER_NUM; i++) {
			data_all[i] = atoh(command[CMD_DATA_OFFSET + (i*2)], command[CMD_DATA_OFFSET + (i*2) + 1]);
		}
		Finger_setBrightnessAll(data_all);
		g_CommandRecvCount++;
		printf("%d\n", g_CommandRecvCount);
		break;
		
	case 3:	// 連結表示設定コマンド
		// TODO:
		// 最終的にはシリアルから設定したデータを送るようにする。
		// 開発中は固定データを送ってデバッグする。
		{
			// <通信プロトコル>
			// [0    ]: 固定で1 (表示コマンドID)
			// [1..32]: 7セグ表示データ(左上から右下に向かう順に指定)
			// 
			// ★注意★
			// DMA転送対象領域は内蔵RAMだけであり、const領域は転送できない。
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
			};
			
			R_DMAC1_StartSend((uint8_t *)&test_data[0], 33);
			while (g_IsMasterSendend == FALSE) {
				NOP();
			}
			g_IsMasterSendend = FALSE;
			
			// 少しの間待つ(DMAハンドラ用のディレイ)
			{
				volatile uint32_t t;
				for (t = 0; t < 100; t++) {
					NOP();
				}
			}
			
			R_DMAC1_StartSend((uint8_t *)&test_data[33], 33);
			while (g_IsMasterSendend == FALSE) {
				NOP();
			}
			g_IsMasterSendend = FALSE;
			
			// 少しの間待つ(DMAハンドラ用のディレイ)
			{
				volatile uint32_t t;
				for (t = 0; t < 100; t++) {
					NOP();
				}
			}
			
			// 結構待つ(1フレームDMA送信分待つ必要あり)
			// TODO: タイマ割り込みハンドラにした方がいいかも。
			{
				volatile uint32_t t;
				for (t = 0; t < 10000; t++) {
					NOP();
				}
			}
			
			// 最速だとINTC割り込みを取りこぼすので遅延が必要。
			P13_bit.no0 = 1;
			PULSE_DELAY();
			P13_bit.no0 = 0;
			
			PRINTF("Sendend.\n");
		}
		break;
		
	case 4:	// 連結輝度設定コマンド
		PRINTF("Not Implemented.");
		break;
		
	default:
		printf("Bad Type.\n");
		break;
	}
}

void Command_receivedHandler(void)
{
	// 受信文字は自動生成コード側でバッファに格納済み
	if (g_CommandBuf[g_CommandBufIndex] == '\n') {
		g_IsCommandReceived = TRUE;
	} else {
		g_CommandBufIndex++;
	}
	R_UART2_Receive(&g_CommandBuf[g_CommandBufIndex], 1);
}

void Command_masterSendendHandler(void)
{
	g_IsMasterSendend = TRUE;
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
