#ifndef ARMOR_H_INCLUDED
#define ARMOR_H_INCLUDED

void Armor_init(void);

void Armor_mainLoop(void);

/**
 * 1ms�����n���h��
 */
void Armor_1msCyclicHandler(void);

/**
 * LATCH�����オ��n���h��
 */
void Armor_latchHandler(void);

#endif /* ARMOR_H_INCLUDED */
