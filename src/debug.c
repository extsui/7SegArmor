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
// FIFO�T�C�Y
#define FIFO_SIZE	(256)

/** FIFO�\����(�Œ�T�C�YFIFO) */
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
 * �f�o�b�O�pprintf()
 * @note 1��̌Ăяo����1�s(80����)���z���Ȃ����ƁB
 * @note �A���ŌĂяo���߂���ƃI�[�o�[���������͏o�͂���Ȃ��B
 * @warning
 *
 * ���������Ӂ�����
 * �{�֐��͕K��1�R���e�L�X�g����Ăяo�����ƁB 
 * (��: ���C���Ŏg�p����Ȃ犄�荞�݂���Ăяo���Ă͂Ȃ�Ȃ�) 
 *
 * ���C���Ɗ��荞�݂̂ǂ��炩����Ăяo�����ꍇ�A�o�b�t�@�j�󂪔�������B 
 * �܂��A�}���`�^�X�N���ł��o�b�t�@�j�󂪔�������B 
 * ���R�A���荞�݂ł����g�p���Ă��Ȃ��ꍇ�ł����d���荞�݂ł����NG�B 
 * �˖{���͊��荞�݋֎~�ő��삷��ׂ��ł��邪�A 
 *   �������o�͂��邾���ł�UART��1�o�C�g���Ƃ�(@115200bps, 20MHz)�Ƃ��� 
 *   ���ۂ������������߁A���荞�݋��̂܂܂Ƃ���B 
 */
void dprint(const char *fmt, ...)
{
    int i;
	int len;
	va_list arg;
	
	// �����荞�݋֎~���Ă��Ȃ����Ƃɒ��Ӂ�
	
	arg = 0;	// 0����͌x���}���p
	va_start(arg, fmt);
    len = vsprintf(g_LineBuff, fmt, arg);
    va_end(arg);
	
	// 1�s�o�b�t�@�͈̓`�F�b�N
	ASSERT(len < sizeof(g_LineBuff));
	
	// FIFO�ւ̊i�[
    for (i = 0; i < len; i++) {
    	if (Fifo_is_full(&g_TxFifo)) {
    		// �o���߂��G���[�̏ꍇ�͂����ɗ���B
			// �G���[���N�������Ƃ��}�[�N����B
			g_TxFifo.buf[g_TxFifo.writePos-1] = '$';
    		break;
    	}
    	Fifo_put(&g_TxFifo, g_LineBuff[i]);
    }
	
	// ���M�J�n���Ă��Ȃ��ꍇ�͑��M�J�n
	if (g_IsTxBusy == FALSE) {
		// FIFO����ŕ�����""���w�肳�ꂽ�ꍇ��
		// FIFO�ɉ����i�[���ꂸ�ɗ���̂Ŋm�F���K�v�B
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
	// �ȉ��̎����ł������ł��邵�A���̕����������ǁA
	// Fifo_count()���g�p�x����}���������̂ŁA
	// Fifo_count()���g�p��������Ƃ���B
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
 * FIFO�����o��
 * @note �G���[�`�F�b�N�͂��Ȃ��̂ŁA
 * @note ���O�Ƀt���łȂ����Ƃ��m�F���邱�ƁB
 */
static void Fifo_put(Fifo *fifo, uint8_t data)
{
	fifo->buf[fifo->writePos] = data;
	fifo->writePos = Fifo_next(fifo->writePos);
}

/**
 * FIFO�ǂݍ���
 * @note �G���[�`�F�b�N�͂��Ȃ��̂ŁA
 * @note ���O�ɋ�łȂ����Ƃ��m�F���邱�ƁB
 */
static uint8_t Fifo_get(Fifo *fifo)
{
	uint8_t data = fifo->buf[fifo->readPos];
	fifo->readPos = Fifo_next(fifo->readPos);
	return data;
}

/************************************************************
 *  �P�̃e�X�g
 ************************************************************/
#if 0

void test_Fifo(void)
{
	static Fifo testFifo;

	//////////////////////////////////////////////////
	
	// ��(���F0)
	ASSERT(Fifo_next(0) == 1);
	
	// ��(���FFIFO_SIZE - 1)
	ASSERT(Fifo_next(FIFO_SIZE - 1) == 0);
	
	//////////////////////////////////////////////////
	
	// �T�C�Y(0)
	Fifo_init(&testFifo);
	testFifo.readPos = 0;
	testFifo.writePos = 0;
	ASSERT(Fifo_count(&testFifo) == 0);
	
	// �T�C�Y(���)
	Fifo_init(&testFifo);
	testFifo.readPos = 0;
	testFifo.writePos = FIFO_SIZE - 1;
	ASSERT(Fifo_count(&testFifo) == FIFO_SIZE - 1);
	
	// �T�C�Y(���񖳂��A���Ԓl)
	Fifo_init(&testFifo);
	testFifo.readPos = 10;
	testFifo.writePos = 20;
	ASSERT(Fifo_count(&testFifo) == 10);
	
	// �T�C�Y(����L��A���Ԓl)
	Fifo_init(&testFifo);
	testFifo.readPos = FIFO_SIZE - 1 - 5;
	testFifo.writePos = 4;
	ASSERT(Fifo_count(&testFifo) == 10);
	
	// �T�C�Y(����L��A���)
	Fifo_init(&testFifo);
	testFifo.readPos = 0;
	testFifo.writePos = FIFO_SIZE - 1;
	ASSERT(Fifo_count(&testFifo) == FIFO_SIZE - 1);

	//////////////////////////////////////////////////
	
	// ��(�����ʒu)
	Fifo_init(&testFifo);
	testFifo.readPos = 0;
	testFifo.writePos = 0;
	ASSERT(Fifo_is_empty(&testFifo) != 0);
	
	// ��(���)
	Fifo_init(&testFifo);
	testFifo.readPos = FIFO_SIZE - 1;
	testFifo.writePos = FIFO_SIZE - 1;
	ASSERT(Fifo_is_empty(&testFifo) != 0);
	
	// ��łȂ�
	Fifo_init(&testFifo);
	testFifo.readPos = 0;
	testFifo.writePos = 1;
	ASSERT(Fifo_is_empty(&testFifo) == 0);
	
	//////////////////////////////////////////////////
	
	// �t���łȂ�(�������)
	Fifo_init(&testFifo);
	testFifo.readPos = 0;
	testFifo.writePos = 0;
	ASSERT(Fifo_is_full(&testFifo) == 0);
	
	// �t��(readPos�����ʒu)
	Fifo_init(&testFifo);
	testFifo.readPos = 0;
	testFifo.writePos = FIFO_SIZE - 1;
	ASSERT(Fifo_is_full(&testFifo) != 0);
	
	// �t��(readPos�񏉊��ʒu)
	Fifo_init(&testFifo);
	testFifo.readPos = 1;
	testFifo.writePos = 0;
	ASSERT(Fifo_is_full(&testFifo) != 0);
	
	//////////////////////////////////////////////////
	
	// �����o��(�擪)
	Fifo_init(&testFifo);
	testFifo.readPos = 0;
	testFifo.writePos = 0;
	Fifo_put(&testFifo, 0x55);
	ASSERT(testFifo.readPos == 0);
	ASSERT(testFifo.writePos == 1);
	ASSERT(testFifo.buf[0] == 0x55);
	
	// �����o��(����)
	Fifo_init(&testFifo);
	testFifo.readPos = 1;	// 0�̏ꍇFIFO������̂Ń_��
	testFifo.writePos = FIFO_SIZE - 1;
	Fifo_put(&testFifo, 0x55);
	ASSERT(testFifo.readPos == 1);
	ASSERT(testFifo.writePos == 0);
	ASSERT(testFifo.buf[FIFO_SIZE - 1] == 0x55);
	
	//////////////////////////////////////////////////
	
	// �ǂݍ���(�擪)
	Fifo_init(&testFifo);
	testFifo.readPos = 0;
	testFifo.writePos = 1;
	testFifo.buf[0] = 0x55;
	ASSERT(0x55 == Fifo_get(&testFifo));
	ASSERT(testFifo.readPos == 1);
	ASSERT(testFifo.writePos == 1);
	
	// �ǂݍ���(����)
	Fifo_init(&testFifo);
	testFifo.readPos = FIFO_SIZE - 1;
	testFifo.writePos = 0;
	testFifo.buf[FIFO_SIZE - 1] = 0x55;
	ASSERT(0x55 == Fifo_get(&testFifo));
	ASSERT(testFifo.readPos == 0);
	ASSERT(testFifo.writePos == 0);
	
	NOP();	// break�p
	
	while (1);
}

#endif
