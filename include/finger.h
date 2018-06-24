#ifndef FINGER_H_INCLUDED
#define FINGER_H_INCLUDED

/** 7SegFingerの個数 */
#define FINGER_NUM	(4)

/** 7SegFingerコマンドサイズ */
#define FINGER_CMD_SIZE	(9)

/**
 * 7SegFingerモジュール初期化
 */
void Finger_init(void);

/**
 * 7SegFingerコマンド
 * 
 * 7SegFingerコマンドを直接設定する。
 * コマンドIDを含めた固定長9バイト全てを7SegFingerに送信する。
 * コマンド内容の解釈は7SegFinger自体に任せる。
 *
 * @param [in] index 7SegFinger番号(0-3)
 * @param [in] cmd 7SegFingerコマンド
 */
void Finger_setCommand(uint8_t index, const uint8_t *cmd);

/**
 * 7SegFinger更新処理
 *
 * 現在の送信予定のコマンドを7SegFingerそれぞれに送信開始する。
 */
void Finger_update(void);

/**
 * 7SegFingerデータ送信完了ハンドラ
 */
void Finger_sendEndHandler(void);

#endif /* FINGER_H_INCLUDED */
