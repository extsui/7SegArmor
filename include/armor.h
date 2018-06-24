#ifndef ARMOR_H_INCLUDED
#define ARMOR_H_INCLUDED

void Armor_init(void);

/**
 * 1ms��������
 */
void Armor_proc(void);

/**
 * UART����̎�M����
 *
 * �X���[�u��M�����n���h����UART�ŁB
 */
void Armor_uartReceiveend(const uint8_t *armorCmd);

/**
 * UART�����LATCH
 *
 * LATCH�����オ��n���h����UART�ŁB
 */
void Armor_uartLatch(void);

/**
 * �}�X�^���M�����n���h��
 *
 * �}�X�^�Ƃ��ẴX���[�u�ւ̃R�}���h���M�����������ۂɌĂяo�����B
 */
void Armor_masterSendendHandler(void);

/**
 * �X���[�u��M�����n���h��
 *
 * �X���[�u�Ƃ��Ẵ}�X�^����̃R�}���h��M�����������ۂɌĂяo�����B
 */
void Armor_slaveReceiveendHandler(void);

/**
 * LATCH�����オ��n���h��
 */
void Armor_latchHandler(void);

#endif /* ARMOR_H_INCLUDED */
