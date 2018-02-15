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
 * コマンド処理
 *
 * コマンド受信が完了していた場合にコマンド処理を行う。
 * 本関数はメインループで定期的にポーリングすること。
 */
void Command_proc(void);

/**
 * 1バイト受信ハンドラ
 */
void Command_receivedHandler(void);

/**
 * スレーブ送信完了ハンドラ
 */
void Command_slaveSendendHandler(void);

#endif /* COMMAND_H_INCLUDED */
