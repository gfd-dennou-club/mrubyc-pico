#ifndef MRBWRITE_H
#define MRBWRITE_H

#include <stdint.h>

enum mrbwrite_command_t {
  MRBWRITE_ILLEGAL = -2, // 正しくないコマンド
  MRBWRITE_TIMEOUT = -1, // タイムアウト
  MRBWRITE_COMMAND_MODE = 0, // コマンドモード (CR+LF)
  MRBWRITE_RESET = 1, // ソフトウェアリセット
  MRBWRITE_EXECUTE = 2, // mrubyプログラム実行
  MRBWRITE_WRITE = 3, // mrubyバイトコード書き込み
  MRBWRITE_CLEAR = 4, // 書き込み済みバイトコードの消去
  MRBWRITE_HELP = 5, // コマンド一覧表示（人間用）
  MRBWRITE_VERSION = 6, // バージョン表示
  MRBWRITE_SHOWPROG = 7, // 書き込み済みプログラムサイズ表示（人間用）
  MRBWRITE_VERIFY = 8 // バイトコード検証
};

/*
* ユーザ入力のコマンドを返す。
*
* @param timeout_us タイムアウト時間（マイクロ秒）。
* @param size writeコマンドのときに書き込みサイズが設定される。呼び出し側で利用しない場合はNULLを指定可能。
* @return 下記のコマンド番号。
*     -2: 正しくないコマンド
*     -1: タイムアウト
*     0: (CR+LF)  - コマンドモード
*     1: reset    - ソフトウェアリセット
*     2: execute  - mrubyプログラム実行
*     3: write    - mrubyバイトコード書き込み
*     4: clear    - 書き込み済みバイトコードの消去
*     5: help     - コマンド一覧表示（人間用）
*     6: version  - バージョン表示
*     7: showprog - 書き込み済みプログラムサイズ表示（人間用）
*     8: verify   - バイトコード検証
*/
int mrbwrite_get_cmd(uint32_t timeout_us, uint32_t *size);

/*
* 指定されたファイルのバイトコードを表示する。
*
* @param filename 表示対象のファイル名。
* @param buffer バイトコードデータが格納されたバッファのポインタ。
* @param size バイトコードのサイズ（バイト単位）。
*/
void mrbwrite_showprog(const char *filename, uint8_t* buffer, uint32_t size);

#endif // MRBWRITE_H
