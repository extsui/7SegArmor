#include "r_cg_macrodriver.h"
#include "r_cg_userdefine.h"
#include "r_cg_serial.h"
#include "debug.h"
#include <stdio.h>
#include <stdarg.h>

/************************************************************
 *  define
 ************************************************************/
// FIFO�T�C�Y
#define FIFO_SIZE	(256)

/** FIFO�\���� */
typedef struct {
  int readPos;			/**< �ǂݍ��݈ʒu */
  int writePos;			/**< �������݈ʒu */
  char buf[FIFO_SIZE];	/**< �o�b�t�@�{�� */
} Fifo;

/************************************************************
 *  global variables
 ************************************************************/
/** 1�s�o�b�t�@(1��̌Ăяo���̏o�͏��������) */
static char g_LineBuff[80];

/** ���MFIFO */
static Fifo g_TxFifo;

/** ���M���� */
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
 * �f�o�b�O�pprintf()
 * @note 1��̌Ăяo����1�s(80����)���z���Ȃ����ƁB
 * @note �A���ŌĂяo���߂���ƃI�[�o�[���������͏o�͂���Ȃ��B
 */
void dprint(const char *fmt, ...)
{
    int i;
	int len;
	va_list arg;
	
	// MEMO: ���荞�݋֎~�ő��삷��B
	// ���荞�݃n���h�����̏ꍇ��EI()�����
	// ���d���荞�݂�����悤�ɂȂ�̂Œ��ӁB
	// TODO: WORKAROUND:
	// ��L�̌��O������̂Ŋ��荞�݃n���h�����p��ʂɗp�ӂ���ׂ��B
	// �����荞�݃��x�����S�������ꍇ�͑��d���荞�݂��Ȃ��̂Ŗ��Ȃ��B
	DI();
	
	arg = 0;	// 0����͌x���}���p
	va_start(arg, fmt);
    len = vsprintf(g_LineBuff, fmt, arg);
    va_end(arg);
	
	// 1�s�o�b�t�@�͈̓`�F�b�N
	ASSERT(len < sizeof(g_LineBuff));
	
	// FIFO�ւ̊i�[
    for (i = 0; i < len; i++) {
    	if (FIFO_IS_FULL(&g_TxFifo)) {
    		// �o���߂��G���[�̏ꍇ�͂����ɗ���B
			// �G���[���N�������Ƃ��}�[�N����B
			g_TxFifo.buf[g_TxFifo.writePos-1] = '$';
    		break;
    	}
    	Fifo_put(&g_TxFifo, g_LineBuff[i]);
    }
	
	// ���M�J�n���Ă��Ȃ��ꍇ�͑��M�J�n
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
