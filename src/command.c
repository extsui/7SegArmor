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
// テスト実施時に有効化すること。
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

// 有効コマンド受信回数
static uint32_t g_CommandRecvCount = 0;

// 1コマンド受信完了フラグ
static BOOL g_IsCommandReceived = FALSE;

/** 7SegArmorフレーム送信中か否か */
/** ※DMA送信ではなく複数回のフレーム送信全体を指す */
static BOOL g_IsArmorFrameSending = FALSE;

/** 7SegArmorフレーム送信数 */
static uint8_t g_ArmorFrameSendNum = 0;

/** 7SegArmorフレーム送信カウンタ */
static uint8_t g_ArmorFrameSendCount = 0;

/** コマンド処理関数 */
typedef void (*cmdFnuc)(const char *s);

typedef struct {
	char	*name;	/**< コマンド名 */
	uint8_t	len;	/**< コマンド名の長さ(効率化のために使用) */
	cmdFnuc	fn;		/**< コマンド処理関数 */
	char	*info;	/**< 情報(ヘルプコマンド時に表示) */
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

// TODO: armor.cに移動するべき。
static uint8_t data_all[FINGER_7SEG_NUM*FINGER_NUM];	// ローカル変数にするとスタックオーバーフロー

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
 * コマンド受信処理
 */
static void Command_onReceive(const char *rxBuff);

/**
 * ７SegArmorフレーム送信終了処理
 * @note DMA送信処理が全部終了したのでLATCH↑↓処理を実施する。
 */
static void armorFrameSendendProc(void);

/**
 * 2文字を16進数1バイトに変換する。
 * エラー処理は行わない。
 */
static uint8_t atoh(uint8_t c_h, uint8_t c_l);

/**
 * 複数パラメータ解析
 * 
 * 文字列sからnum個のパラメータを解析してparams[]に格納する。
 * 格納出来た個数を戻り値として返す。
 * パラメータは空白文字(" ")で区切られているものとする。
 * num個分の解析が終了するか、もしくは'\0'が来たら終了する。
 *
 * @param s 入力文字列(NULLでないこと)
 * @param params 解析結果配列(NULLでないこと)
 * @param num 解析する個数(0より大)
 * @return 解析できた個数
 * @note 解析可能な値はstrtol()で基数0指定時のものに限る。
 * @note また、解析結果はint型に丸められることに注意。
 * @note 引数の前提に記載した項目はチェックしない。
 */
static int getParam(const char *s, int params[], int num);

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

	// 一致しなかった場合
	if (cmdIndex == ARRAY_SIZE(g_CommandTable)) {
		// 空行ならプロンプト'>'を表示
		if (rxBuff[0] == '\n') {
			DPRINTF(">");
		// 空行でないなら誤ったコマンドという旨を表示
		} else {
			DPRINTF("Bad Type.\n");
		}
	}
}

/************************************************************/

static void cmdC0(const char *s)
{
	uint8_t dipsw;
	
	// DIPSW[3:0] = [ P120 | P43 | P42 | P41 ]、負論理
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
	
	// 型: [ 'C', '1', (0, 0), (0, 1), ..., (3, 7) ]
	// 例: C1000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F\n
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
	
	// 型: [ 'C', '2', (0, 0), (0, 1), ..., (3, 7) ]
	// 例: C2000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F\n
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
	// 連結輝度設定コマンド
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
	// 連結表示設定コマンド
	// TODO:
	// 最終的にはシリアルから設定したデータを送るようにする。
	// 開発中は固定データを送ってデバッグする。

	// <通信プロトコル>
	// [0    ]: 固定で1 (表示コマンドID)
	// [1..32]: 7セグ表示データ(左上から右下に向かう順に指定)
	Finger_setDisplayAll(&test_data[1]);
			
	// TODO: フラグを直接操作してはいけない。関数でラッピングするべき。
	// そもそも1フレームしか用意しないじゃなかったっけ？
	g_IsArmorFrameSending = TRUE;
	g_ArmorFrameSendNum = 4;
	g_ArmorFrameSendCount = 1;
	
	// 33バイトのDMA送信@1MHz≒270[us]
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
	// プロトタイプ宣言を外に出すとテスト追加時の
	// 修正箇所が飛んでしまうのでローカルで宣言する。
	void test_getParam(void);
	test_getParam();
#else
	DPRINTF("You should activate \"TEST_COMMAND\".\n");
#endif
}

/************************************************************/

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
	g_ArmorFrameSendCount++;
		
	// フレーム送信中
	if (g_ArmorFrameSendCount <= g_ArmorFrameSendNum) {
		// [DMA間遅延時間]
		// 下流機器のDMA完了割り込みの処理時間分待つ。
		// ・OK：20us
		// ・OK：15us
		// ・NG：10us
		//   ⇒遅延時間は、20[us] 見ておけば問題ないと判断。
		R_TAU0_BusyWait(20);
		
		// 次のフレーム送信
		R_DMAC1_StartSend((uint8_t *)&test_data[g_ArmorFrameSendCount*33], 33);
		
	// 最終フレーム送信後
	} else {
		// [LATCH更新遅延時間]
		// DMA転送1回分+αの時間を確保する必要がある。
		// ・DMA1回理論値：33*8[bit] / 1[Mbps] = 264[us]
		// ・DMA1回測定値：267～268[us]
		//   ⇒遅延時間は、300[us] 見ておけば問題ないと判断。
		R_TAU0_SetTimeout(300, armorFrameSendendProc);
	}
}

/************************************************************
 *  prvaite functions
 ************************************************************/
static void armorFrameSendendProc(void)
{
	// 最速だとINTC割り込みを取りこぼすので遅延が必要。
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
		// 1文字以上読めているなら何かしら解析できている。
		if (startp != endp) {
			params[ok_num] = (int)val;
			ok_num++;
		// 何も解析できなかったけど文字列最後でもない。
		} else if (val == 0) {
			break;
		}
		startp = endp;
	} while ((endp != NULL) && (ok_num < num));
	
	return ok_num;
}

/************************************************************
 *  テストコード
 ************************************************************/
#ifdef TEST_COMMAND

void test_getParam(void)
{
	int params[2];
	int retval;
	
	DPRINTF("TEST: getParam()\n");
	
	DPRINTF("[正常系] 引数1個、0\n");
	retval = 0;
	memset(params, 0, sizeof(params));
	retval = getParam("1", params, 1);
	ASSERT(retval == 1);
	ASSERT(params[0] == 1);
	
	DPRINTF("[正常系] 引数1個、非0\n");
	retval = 0;
	memset(params, 0, sizeof(params));
	retval = getParam("1", params, 1);
	ASSERT(retval == 1);
	ASSERT(params[0] == 1);

	DPRINTF("[正常系] 引数1個、複数文字\n");
	retval = 0;
	memset(params, 0, sizeof(params));
	retval = getParam("123", params, 1);
	ASSERT(retval == 1);
	ASSERT(params[0] == 123);

	DPRINTF("[異常系] 引数1個、文字無し\n");
	retval = 0;
	memset(params, 0, sizeof(params));
	params[0] = 0xCC;
	retval = getParam("", params, 1);
	ASSERT(retval == 0);
	ASSERT(params[0] == 0xCC);

	DPRINTF("[異常系] 引数1個、改行のみ\n");
	retval = 0;
	memset(params, 0, sizeof(params));
	params[0] = 0xCC;
	retval = getParam("\n", params, 1);
	ASSERT(retval == 0);
	ASSERT(params[0] == 0xCC);

	DPRINTF("[異常系] 引数1個、不明文字\n");
	retval = 0;
	memset(params, 0, sizeof(params));
	params[0] = 0xCC;
	retval = getParam("$", params, 1);
	ASSERT(retval == 0);
	ASSERT(params[0] == 0xCC);
	
	DPRINTF("[正常系] 引数2個、最小文字列\n");
	retval = 0;
	memset(params, 0, sizeof(params));
	retval = getParam(" 1 2", params, 2);
	ASSERT(retval == 2);
	ASSERT(params[0] == 1);
	ASSERT(params[1] == 2);
	
	DPRINTF("[正常系] 引数2個、複数文字列\n");
	retval = 0;
	memset(params, 0, sizeof(params));
	retval = getParam("123  456", params, 2);
	ASSERT(retval == 2);
	ASSERT(params[0] == 123);
	ASSERT(params[1] == 456);
	
	DPRINTF("[異常系] 引数2個、3個指定\n");
	retval = 0;
	memset(params, 0, sizeof(params));
	retval = getParam("123 456 $", params, 2);
	ASSERT(retval == 2);
	ASSERT(params[0] == 123);
	ASSERT(params[1] == 456);
	
	DPRINTF("TEST: OK!\n");
}

#endif /* TEST_COMMAND */
