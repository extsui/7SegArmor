#ifndef COMMAND_H_INCLUDED
#define COMMAND_H_INCLUDED

#include "r_cg_macrodriver.h" /* uint8_t */

/** 7SegArmor�}���h */
#define ARMOR_CMD_GET_MODE			(0)	// �Œ蒷
#define ARMOR_CMD_SET_DISPLAY		(1)	// �ϒ�
#define ARMOR_CMD_SET_BRIGHTNESS	(2)	// �ϒ�

/**
 * �R�}���h���W���[��������
 */
void Command_init(void);

/**
 * �R�}���h����
 *
 * �R�}���h��M���������Ă����ꍇ�ɃR�}���h�������s���B
 * �{�֐��̓��C�����[�v�Œ���I�Ƀ|�[�����O���邱�ƁB
 */
void Command_proc(void);

/**
 * 1�o�C�g��M�n���h��
 */
void Command_receivedHandler(void);

/**
 * �X���[�u���M�����n���h��
 */
void Command_slaveSendendHandler(void);

#endif /* COMMAND_H_INCLUDED */
