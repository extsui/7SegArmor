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

static uint8_t g_RecvBuf[COMMAND_BUF_SIZE];
static uint16_t g_RecvBufIndex = 0;
static uint8_t g_CommandBuf[COMMAND_BUF_SIZE];

// 有効コマンド受信回数
static uint32_t g_CommandRecvCount = 0;

// 1コマンド受信完了フラグ
static BOOL g_IsCommandReceived = FALSE;

/** コマンド処理関数 */
typedef void (*cmdFnuc)(const char *s);

typedef struct {
	char	*name;	/**< コマンド名 */
	uint8_t	len;	/**< コマンド名の長さ(効率化のために使用) */
	cmdFnuc	fn;		/**< コマンド処理関数 */
	char	*info;	/**< 情報(ヘルプコマンド時に表示) */
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
 * コマンド受信処理
 */
static void Command_onReceive(const char *rxBuff);

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

void Command_receivedHandler(void)
{
	// 受信文字は自動生成コード側でバッファに格納済み
	// [注意]
	// 処理単純化のため、異常系(バッファオーバーフロー)対策は
	// していないので、\nがないひたすら長い文字列は禁止。
	if (g_RecvBuf[g_RecvBufIndex] == '\n') {
		// 空行もコマンドとして見なすので改行も含めてコピー
		memcpy(g_CommandBuf, g_RecvBuf, g_RecvBufIndex + 1);
		g_IsCommandReceived = TRUE;
		g_RecvBufIndex = 0;
	} else {
		g_RecvBufIndex++;
	}
	// 受信データを落とさないように受信は即継続する
	R_UART2_Receive(&g_RecvBuf[g_RecvBufIndex], 1);
}

/************************************************************/

static void cmdInstruct(const char *s)
{
	static uint8_t data_all[36];	// ローカル変数にするとスタックオーバーフロー
	uint8_t i;
	
	// 例: #001FFFFFFFFFFFFFFFF01FFFFFFFFFFFFFFFF01FFFFFFFFFFFFFFFF01FFFFFFFFFFFFFFFF\n
	// ※"#0"を除いたものが引数sにセットされている
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
	
	// DIPSW[3:0] = [ P120 | P43 | P42 | P41 ]、負論理
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
	// プロトタイプ宣言を外に出すとテスト追加時の
	// 修正箇所が飛んでしまうのでローカルで宣言する。
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
