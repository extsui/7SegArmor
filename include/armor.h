#ifndef ARMOR_H_INCLUDED
#define ARMOR_H_INCLUDED

void Armor_init(void);

/**
 * 1ms��������
 */
void Armor_proc(void);

/**
 * ��������M�I���n���h��
 */
void Armor_slaveReceiveendHandler(void);

/**
 * LATCH�����オ��n���h��
 */
void Armor_latchHandler(void);

#endif /* ARMOR_H_INCLUDED */
