#include "mrbwrite.h"
#include "pico/stdlib.h" 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
int mrbwrite_get_cmd (uint32_t timeout_us, uint32_t *size) {
  const uint32_t buffer_size = 16; // コマンドバッファのサイズ
  uint8_t buffer[buffer_size];

  // コマンド入力の受付
  for (int i = 0; i < buffer_size; i++) {
    int input = getchar_timeout_us(timeout_us);
    if (input == PICO_ERROR_TIMEOUT)
      return MRBWRITE_TIMEOUT;
    const uint8_t ch = (uint8_t)(input & 0xFF);
    buffer[i] = ch;
    if (ch == 0x0a) { // LF (0x0a) が入力されたらコマンド入力終了
      break;
    }
  }

  // コマンドを返す
  uint32_t dummy = 0;
  if (memcmp(buffer, "\r\n", 2) == 0) {
    return MRBWRITE_COMMAND_MODE;
  } else if (memcmp(buffer, "reset\r\n", 7) == 0) {
    return MRBWRITE_RESET;
  } else if (memcmp(buffer, "execute\r\n", 9) == 0) {
    return MRBWRITE_EXECUTE;
  } else if (sscanf(buffer, "write %d\r\n", (size == NULL ? &dummy : size)) == 1) {
    return MRBWRITE_WRITE;
  } else if (memcmp(buffer, "clear\r\n", 7) == 0) {
    return MRBWRITE_CLEAR;
  } else if (memcmp(buffer, "help\r\n", 6) == 0) {
    return MRBWRITE_HELP;
  } else if (memcmp(buffer, "version\r\n", 9) == 0) {
    return MRBWRITE_VERSION;
  } else if (memcmp(buffer, "showprog\r\n", 10) == 0) {
    return MRBWRITE_SHOWPROG;
  } else if (memcmp(buffer, "verify\r\n", 8) == 0) {
    return MRBWRITE_VERIFY;
  }

  return MRBWRITE_ILLEGAL;
}

/*
* 指定されたファイルのバイトコードの表示。
*
* @param filename 表示対象のファイル名。
* @param buffer バイトコードデータが格納されたバッファのポインタ。
* @param size バイトコードのサイズ（バイト単位）。
*/
void mrbwrite_showprog(const char *filename, uint8_t* buffer, uint32_t size) {  
  // ファイル名とヘッダーを表示
  printf("**** %s ****\r\n", filename);
  printf("01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\r\n");

  // 16バイトごとにヘキサダンプを表示
  for (uint32_t i = 0; i < size; i += 16) {
    printf("%08x: ", i);

    // バイト値をヘキサで表示
    for (int j = 0; j < 16; j++) {
      if (i + j < size) {
        printf("%02x ", buffer[i + j]);
      } else {
        printf("   ");
      }
    }

    // ASCII文字として表示可能な文字を表示
    printf(" | ");
    for (int j = 0; j < 16 && i + j < size; j++) {
      char c = (char)buffer[i + j];
      printf("%c", (c >= 32 && c <= 126) ? c : '.');
    }
    printf(" |\r\n");
  }
}
