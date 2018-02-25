#ifndef ARMOR_H_INCLUDED
#define ARMOR_H_INCLUDED

void Armor_init(void);

void Armor_mainLoop(void);

/**
 * 1ms周期ハンドラ
 */
void Armor_1msCyclicHandler(void);

/**
 * 下流側受信終了ハンドラ
 */
void Armor_slaveReceiveendHandler(void);

/**
 * LATCH立ち上がりハンドラ
 */
void Armor_latchHandler(void);

#endif /* ARMOR_H_INCLUDED */
