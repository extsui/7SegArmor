#ifndef FINGER_H_INCLUDED
#define FINGER_H_INCLUDED

/** 7SegFingerの個数 */
#define FINGER_NUM	(4)

/** 7SegFingerの7セグの個数 */
#define FINGER_7SEG_NUM	(8)

/** 7SegFingerのフレームタイプ */
#define FINGER_FRAME_TYPE_DISPLAY		(1)
#define FINGER_FRAME_TYPE_BRIGHTNESS	(2)

#define FINGER_FRAME_SIZE				(9)

/**
 * 7SegFingerモジュール初期化
 */
void Finger_init(void);

/**
 * 7SegFinger全表示設定
 *
 * FINGER_7SEG_NUM*FINGER_NUM分の全表示データを設定する。
 *
 * @param dispAllData [in] 全表示データ。要素数FINGER_7SEG_NUM*FINGER_NUMの配列。
 */
void Finger_setDisplayAll(const uint8_t *displayAll);

/**
 * 7SegFinger全輝度設定
 *
 * FINGER_7SEG_NUM*FINGER_NUM分の全輝度データを設定する。
 *
 * @param brightnessAllData [in] 全輝度データ。要素数FINGER_7SEG_NUM*FINGER_NUMの配列。
 */
void Finger_setBrightnessAll(const uint8_t *brightnessAll);

/**
 * 7SegFinger 1ms周期処理
 */
void Finger_proc(void);

/**
 * 7SegFingerデータ送信完了ハンドラ
 */
void Finger_sendEndHandler(void);

#endif /* FINGER_H_INCLUDED */
