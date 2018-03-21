#ifndef FINGER_H_INCLUDED
#define FINGER_H_INCLUDED

/** 7SegFinger�̌� */
#define FINGER_NUM	(4)

/** 7SegFinger��7�Z�O�̌� */
#define FINGER_7SEG_NUM	(8)

/** 7SegFinger�̃t���[���^�C�v */
#define FINGER_FRAME_TYPE_DISPLAY		(1)
#define FINGER_FRAME_TYPE_BRIGHTNESS	(2)

#define FINGER_FRAME_SIZE				(9)

/**
 * 7SegFinger���W���[��������
 */
void Finger_init(void);

/**
 * 7SegFinger�S�\���ݒ�
 *
 * FINGER_7SEG_NUM*FINGER_NUM���̑S�\���f�[�^��ݒ肷��B
 *
 * @param dispAllData [in] �S�\���f�[�^�B�v�f��FINGER_7SEG_NUM*FINGER_NUM�̔z��B
 */
void Finger_setDisplayAll(const uint8_t *displayAll);

/**
 * 7SegFinger�S�P�x�ݒ�
 *
 * FINGER_7SEG_NUM*FINGER_NUM���̑S�P�x�f�[�^��ݒ肷��B
 *
 * @param brightnessAllData [in] �S�P�x�f�[�^�B�v�f��FINGER_7SEG_NUM*FINGER_NUM�̔z��B
 */
void Finger_setBrightnessAll(const uint8_t *brightnessAll);

/**
 * 7SegFinger 1ms��������
 */
void Finger_proc(void);

/**
 * 7SegFinger�f�[�^���M�����n���h��
 */
void Finger_sendEndHandler(void);

#endif /* FINGER_H_INCLUDED */
