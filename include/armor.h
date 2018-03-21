#ifndef ARMOR_H_INCLUDED
#define ARMOR_H_INCLUDED

void Armor_init(void);

/**
 * 1ms周期処理
 */
void Armor_proc(void);

/**
 * 下流側受信終了ハンドラ
 */
void Armor_slaveReceiveendHandler(void);

/**
 * LATCH立ち上がりハンドラ
 */
void Armor_latchHandler(void);

#endif /* ARMOR_H_INCLUDED */
