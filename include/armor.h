#ifndef ARMOR_H_INCLUDED
#define ARMOR_H_INCLUDED

void Armor_init(void);

/**
 * 1ms周期処理
 */
void Armor_proc(void);

/**
 * UARTからの受信完了
 *
 * スレーブ受信完了ハンドラのUART版。
 */
void Armor_uartReceiveend(const uint8_t *armorCmd);

/**
 * UARTからのLATCH
 *
 * LATCH立ち上がりハンドラのUART版。
 */
void Armor_uartLatch(void);

/**
 * マスタ送信完了ハンドラ
 *
 * マスタとしてのスレーブへのコマンド送信が完了した際に呼び出される。
 */
void Armor_masterSendendHandler(void);

/**
 * スレーブ受信完了ハンドラ
 *
 * スレーブとしてのマスタからのコマンド受信が完了した際に呼び出される。
 */
void Armor_slaveReceiveendHandler(void);

/**
 * LATCH立ち上がりハンドラ
 */
void Armor_latchHandler(void);

#endif /* ARMOR_H_INCLUDED */
