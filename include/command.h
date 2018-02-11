#ifndef COMMAND_H_INCLUDED
#define COMMAND_H_INCLUDED

#include "r_cg_macrodriver.h" /* uint8_t */

/** 7SegArmorマンド */
#define ARMOR_CMD_GET_MODE			(0)	// 固定長
#define ARMOR_CMD_SET_DISPLAY		(1)	// 可変長
#define ARMOR_CMD_SET_BRIGHTNESS	(2)	// 可変長

/**
 * コマンドモジュール初期化
 */
void Command_init(void);

/**
 * 1バイト受信ハンドラ
 */
void Command_receivedHandler(void);

#endif /* COMMAND_H_INCLUDED */
