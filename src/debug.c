#include "r_cg_macrodriver.h"
#include "r_cg_userdefine.h"
#include "r_cg_serial.h"
#include "debug.h"
#include <stdio.h>
#include <stdarg.h>

/************************************************************
 *  define
 ************************************************************/
// FIFOサイズ
#define FIFO_SIZE	(256)

/** FIFO構造体 */
typedef struct {
  int readPos;			/**< 読み込み位置 */
  int writePos;			/**< 書き込み位置 */
  char buf[FIFO_SIZE];	/**< バッファ本体 */
} Fifo;

/************************************************************
 *  global variables
 ************************************************************/
/** 1行バッファ(1回の呼び出しの出力上限文字列長) */
static char g_LineBuff[80];

/** 送信FIFO */
static Fifo g_TxFifo;

/** 送信中か */
static BOOL g_IsTxBusy;

/************************************************************
 *  prototype
 ************************************************************/
#define FIFO_NEXT(x)		(((x) + 1) % FIFO_SIZE)
#define FIFO_COUNT(fifo)	(((fifo)->writePos - (fifo)->readPos) % FIFO_SIZE)
#define FIFO_IS_EMPTY(fifo)	((fifo)->writePos == (fifo)->readPos)
#define FIFO_IS_FULL(fifo)	((FIFO_COUNT((fifo)) - 1) == FIFO_SIZE)

static void Fifo_init(Fifo *fifo);
static void Fifo_put(Fifo *fifo, uint8_t data);
static uint8_t Fifo_get(Fifo *fifo);

/************************************************************
 *  public functions
 ************************************************************/
void Debug_init(void)
{
	Fifo_init(&g_TxFifo);
	g_IsTxBusy = FALSE;
}

/**
 * デバッグ用printf()
 * @note 1回の呼び出しで1行(80文字)を越えないこと。
 * @note 連続で呼び出し過ぎるとオーバーした文字は出力されない。
 */
void dprint(const char *fmt, ...)
{
    int i;
	int len;
	va_list arg;
	
	// MEMO: 割り込み禁止で操作する。
	// 割り込みハンドラ内の場合はEI()すると
	// 多重割り込みが入るようになるので注意。
	// TODO: WORKAROUND:
	// 上記の懸念があるので割り込みハンドラ内用を別に用意するべき。
	// ※割り込みレベルが全部同じ場合は多重割り込みしないので問題なし。
	DI();
	
	arg = 0;	// 0代入は警告抑制用
	va_start(arg, fmt);
    len = vsprintf(g_LineBuff, fmt, arg);
    va_end(arg);
	
	// 1行バッファ範囲チェック
	ASSERT(len < sizeof(g_LineBuff));
	
	// FIFOへの格納
    for (i = 0; i < len; i++) {
    	if (FIFO_IS_FULL(&g_TxFifo)) {
    		// 出し過ぎエラーの場合はここに来る。
			// エラーが起きたことをマークする。
			g_TxFifo.buf[g_TxFifo.writePos-1] = '$';
    		break;
    	}
    	Fifo_put(&g_TxFifo, g_LineBuff[i]);
    }
	
	// 送信開始していない場合は送信開始
	if (g_IsTxBusy == FALSE) {
		uint8_t c;
		c = Fifo_get(&g_TxFifo);
		g_IsTxBusy = TRUE;
		R_UART2_Send(&c, 1);
	}
	
	EI();
}

void Debug_charSendendHandler(void)
{
	if (FIFO_IS_EMPTY(&g_TxFifo)) {
		g_IsTxBusy = FALSE;
	} else {
		uint8_t c = Fifo_get(&g_TxFifo);
		R_UART2_Send(&c, 1);
	}
}

/************************************************************
 *  private functions
 ************************************************************/
static void Fifo_init(Fifo *fifo)
{
	fifo->readPos = 0;
	fifo->writePos = 0;
}

static void Fifo_put(Fifo *fifo, uint8_t data)
{
	fifo->buf[fifo->writePos] = data;
	fifo->writePos = FIFO_NEXT(fifo->writePos);
}

static uint8_t Fifo_get(Fifo *fifo)
{
	uint8_t data = fifo->buf[fifo->readPos];
	fifo->readPos = FIFO_NEXT(fifo->readPos);
	return data;
}
