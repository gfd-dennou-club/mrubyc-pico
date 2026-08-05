#ifndef MRBWRITE_H
#define MRBWRITE_H

#include <stdint.h>

enum mrbwrite_command_t {
  MRBWRITE_ILLEGAL = -2, // 正しくないコマンド
  MRBWRITE_TIMEOUT = -1, // タイムアウト
  MRBWRITE_COMMAND_MODE = 0, // コマンドモード (CR+LF)
  MRBWRITE_RESET = 1, // ソフトウェアリセット
  MRBWRITE_EXECUTE = 2, // mrubyプログラム実行
  MRBWRITE_WRITE = 3, // タスクのmrubyバイトコード書き込み
  MRBWRITE_WRITE_LIB = 4, // ライブラリのmrubyバイトコード書き込み
  MRBWRITE_CLEAR = 5, // 書き込み済みバイトコードの消去
  MRBWRITE_HELP = 6, // コマンド一覧表示（人間用）
  MRBWRITE_VERSION = 7, // バージョン表示
  MRBWRITE_SHOWPROG = 8, // 書き込み済みプログラムサイズ表示（人間用）
  MRBWRITE_VERIFY = 9 // バイトコード検証 [DEPRECATED] mrbwrite v1.3で削除
};

// mrbwrite_get_cmd(timeout_us, size, crc) ユーザー入力のコマンド番号取得
int mrbwrite_get_cmd(uint32_t timeout_us, uint32_t *size, int32_t *crc);

// バイトコードのヘキサダンプ表示
void mrbwrite_showprog(const char *filename, uint8_t* buffer, uint32_t size);

// バイトコードのCRC16計算
uint16_t mrbwrite_crc16(const uint8_t *buffer, uint32_t size);

#endif // MRBWRITE_H
