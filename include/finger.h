#ifndef FINGER_H_INCLUDED
#define FINGER_H_INCLUDED

/** 7SegFinger�̌� */
#define FINGER_NUM	(4)

/** 7SegFinger�R�}���h�T�C�Y */
#define FINGER_CMD_SIZE	(9)

/**
 * 7SegFinger���W���[��������
 */
void Finger_init(void);

/**
 * 7SegFinger�R�}���h
 * 
 * 7SegFinger�R�}���h�𒼐ڐݒ肷��B
 * �R�}���hID���܂߂��Œ蒷9�o�C�g�S�Ă�7SegFinger�ɑ��M����B
 * �R�}���h���e�̉��߂�7SegFinger���̂ɔC����B
 *
 * @param [in] index 7SegFinger�ԍ�(0-3)
 * @param [in] cmd 7SegFinger�R�}���h
 */
void Finger_setCommand(uint8_t index, const uint8_t *cmd);

/**
 * 7SegFinger�X�V����
 *
 * ���݂̑��M�\��̃R�}���h��7SegFinger���ꂼ��ɑ��M�J�n����B
 */
void Finger_update(void);

/**
 * 7SegFinger�f�[�^���M�����n���h��
 */
void Finger_sendEndHandler(void);

#endif /* FINGER_H_INCLUDED */
