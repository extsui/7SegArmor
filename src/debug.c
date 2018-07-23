#include "r_cg_macrodriver.h"
#include "r_cg_userdefine.h"
#include "r_cg_serial.h"
#include "debug.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/************************************************************
 *  define
 ************************************************************/
// FIFOサイズ
#define FIFO_SIZE	(256)

/** FIFO構造体(固定サイズFIFO) */
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
static __inline int Fifo_next(int x);
static __inline int Fifo_count(const Fifo *fifo);
static __inline int Fifo_is_empty(const Fifo *fifo);
static __inline int Fifo_is_full(const Fifo *fifo);
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
 * @warning
 *
 * ★★★注意★★★
 * 本関数は必ず1コンテキストから呼び出すこと。 
 * (例: メインで使用するなら割り込みから呼び出してはならない) 
 *
 * メインと割り込みのどちらからも呼び出した場合、バッファ破壊が発生する。 
 * また、マルチタスク環境でもバッファ破壊が発生する。 
 * 当然、割り込みでしか使用していない場合でも多重割り込みであればNG。 
 * ⇒本来は割り込み禁止で操作するべきであるが、 
 *   数文字出力するだけでもUARTを1バイト落とす(@115200bps, 20MHz)という 
 *   現象が発生したため、割り込み許可のままとする。 
 */
void dprint(const char *fmt, ...)
{
    int i;
	int len;
	va_list arg;
	
	// ★割り込み禁止していないことに注意★
	
	arg = 0;	// 0代入は警告抑制用
	va_start(arg, fmt);
    len = vsprintf(g_LineBuff, fmt, arg);
    va_end(arg);
	
	// 1行バッファ範囲チェック
	ASSERT(len < sizeof(g_LineBuff));
	
	// FIFOへの格納
    for (i = 0; i < len; i++) {
    	if (Fifo_is_full(&g_TxFifo)) {
    		// 出し過ぎエラーの場合はここに来る。
			// エラーが起きたことをマークする。
			g_TxFifo.buf[g_TxFifo.writePos-1] = '$';
    		break;
    	}
    	Fifo_put(&g_TxFifo, g_LineBuff[i]);
    }
	
	// 送信開始していない場合は送信開始
	if (g_IsTxBusy == FALSE) {
		// FIFOが空で文字列""が指定された場合は
		// FIFOに何も格納されずに来るので確認が必要。
		if (!Fifo_is_empty(&g_TxFifo)) {
			uint8_t c;
			c = Fifo_get(&g_TxFifo);
			g_IsTxBusy = TRUE;
			R_UART2_Send(&c, 1);
		}
	}
}

void Debug_charSendendHandler(void)
{
	if (Fifo_is_empty(&g_TxFifo)) {
		g_IsTxBusy = FALSE;
	} else {
		uint8_t c = Fifo_get(&g_TxFifo);
		R_UART2_Send(&c, 1);
	}
}

/************************************************************
 *  private functions
 ************************************************************/
static __inline int Fifo_next(int x)
{
	return (x + 1) % FIFO_SIZE;
}

static __inline int Fifo_count(const Fifo *fifo)
{
	return (fifo->writePos - fifo->readPos + FIFO_SIZE) % FIFO_SIZE;
}

static __inline int Fifo_is_empty(const Fifo *fifo)
{
	return fifo->writePos == fifo->readPos;
}

static __inline int Fifo_is_full(const Fifo *fifo)
{
	// 以下の実装でも実現できるし、この方が早いけど、
	// Fifo_count()未使用警告を抑制したいので、
	// Fifo_count()を使用する実装とする。
	//   return Fifo_next(fifo->writePos) == fifo->readPos;
	return Fifo_count(fifo) == FIFO_SIZE - 1;
}

static void Fifo_init(Fifo *fifo)
{
	fifo->readPos = 0;
	fifo->writePos = 0;
	memset(fifo->buf, 0, FIFO_SIZE);
}

/**
 * FIFO書き出し
 * @note エラーチェックはしないので、
 * @note 事前にフルでないことを確認すること。
 */
static void Fifo_put(Fifo *fifo, uint8_t data)
{
	fifo->buf[fifo->writePos] = data;
	fifo->writePos = Fifo_next(fifo->writePos);
}

/**
 * FIFO読み込み
 * @note エラーチェックはしないので、
 * @note 事前に空でないことを確認すること。
 */
static uint8_t Fifo_get(Fifo *fifo)
{
	uint8_t data = fifo->buf[fifo->readPos];
	fifo->readPos = Fifo_next(fifo->readPos);
	return data;
}

/************************************************************
 *  単体テスト
 ************************************************************/
#if 0

void test_Fifo(void)
{
	static Fifo testFifo;

	//////////////////////////////////////////////////
	
	// 次(元：0)
	ASSERT(Fifo_next(0) == 1);
	
	// 次(元：FIFO_SIZE - 1)
	ASSERT(Fifo_next(FIFO_SIZE - 1) == 0);
	
	//////////////////////////////////////////////////
	
	// サイズ(0)
	Fifo_init(&testFifo);
	testFifo.readPos = 0;
	testFifo.writePos = 0;
	ASSERT(Fifo_count(&testFifo) == 0);
	
	// サイズ(上限)
	Fifo_init(&testFifo);
	testFifo.readPos = 0;
	testFifo.writePos = FIFO_SIZE - 1;
	ASSERT(Fifo_count(&testFifo) == FIFO_SIZE - 1);
	
	// サイズ(周回無し、中間値)
	Fifo_init(&testFifo);
	testFifo.readPos = 10;
	testFifo.writePos = 20;
	ASSERT(Fifo_count(&testFifo) == 10);
	
	// サイズ(周回有り、中間値)
	Fifo_init(&testFifo);
	testFifo.readPos = FIFO_SIZE - 1 - 5;
	testFifo.writePos = 4;
	ASSERT(Fifo_count(&testFifo) == 10);
	
	// サイズ(周回有り、上限)
	Fifo_init(&testFifo);
	testFifo.readPos = 0;
	testFifo.writePos = FIFO_SIZE - 1;
	ASSERT(Fifo_count(&testFifo) == FIFO_SIZE - 1);

	//////////////////////////////////////////////////
	
	// 空(初期位置)
	Fifo_init(&testFifo);
	testFifo.readPos = 0;
	testFifo.writePos = 0;
	ASSERT(Fifo_is_empty(&testFifo) != 0);
	
	// 空(上限)
	Fifo_init(&testFifo);
	testFifo.readPos = FIFO_SIZE - 1;
	testFifo.writePos = FIFO_SIZE - 1;
	ASSERT(Fifo_is_empty(&testFifo) != 0);
	
	// 空でない
	Fifo_init(&testFifo);
	testFifo.readPos = 0;
	testFifo.writePos = 1;
	ASSERT(Fifo_is_empty(&testFifo) == 0);
	
	//////////////////////////////////////////////////
	
	// フルでない(初期状態)
	Fifo_init(&testFifo);
	testFifo.readPos = 0;
	testFifo.writePos = 0;
	ASSERT(Fifo_is_full(&testFifo) == 0);
	
	// フル(readPos初期位置)
	Fifo_init(&testFifo);
	testFifo.readPos = 0;
	testFifo.writePos = FIFO_SIZE - 1;
	ASSERT(Fifo_is_full(&testFifo) != 0);
	
	// フル(readPos非初期位置)
	Fifo_init(&testFifo);
	testFifo.readPos = 1;
	testFifo.writePos = 0;
	ASSERT(Fifo_is_full(&testFifo) != 0);
	
	//////////////////////////////////////////////////
	
	// 書き出し(先頭)
	Fifo_init(&testFifo);
	testFifo.readPos = 0;
	testFifo.writePos = 0;
	Fifo_put(&testFifo, 0x55);
	ASSERT(testFifo.readPos == 0);
	ASSERT(testFifo.writePos == 1);
	ASSERT(testFifo.buf[0] == 0x55);
	
	// 書き出し(末尾)
	Fifo_init(&testFifo);
	testFifo.readPos = 1;	// 0の場合FIFOが壊れるのでダメ
	testFifo.writePos = FIFO_SIZE - 1;
	Fifo_put(&testFifo, 0x55);
	ASSERT(testFifo.readPos == 1);
	ASSERT(testFifo.writePos == 0);
	ASSERT(testFifo.buf[FIFO_SIZE - 1] == 0x55);
	
	//////////////////////////////////////////////////
	
	// 読み込み(先頭)
	Fifo_init(&testFifo);
	testFifo.readPos = 0;
	testFifo.writePos = 1;
	testFifo.buf[0] = 0x55;
	ASSERT(0x55 == Fifo_get(&testFifo));
	ASSERT(testFifo.readPos == 1);
	ASSERT(testFifo.writePos == 1);
	
	// 読み込み(末尾)
	Fifo_init(&testFifo);
	testFifo.readPos = FIFO_SIZE - 1;
	testFifo.writePos = 0;
	testFifo.buf[FIFO_SIZE - 1] = 0x55;
	ASSERT(0x55 == Fifo_get(&testFifo));
	ASSERT(testFifo.readPos == 0);
	ASSERT(testFifo.writePos == 0);
	
	NOP();	// break用
	
	while (1);
}

#endif
