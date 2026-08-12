/*! @file
  @brief mruby/cメインエントリポイント

  ボードの初期化，ファイルシステムの初期化，mrbwriteコマンドモードの処理，
  mruby/cプログラムを実行する．
*/

//*********************************************
// main関数で使われるライブラリ
//*********************************************
#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include <stdio.h>
#include <stdlib.h>

//*********************************************
// main関数で使われるユーティリティの前方宣言
//*********************************************
int mrbwrite_cmd_mode();

//*********************************************
// ENABLE LIBRARY written by C (Extensions for Ruby)
//*********************************************
#include "mrubyc.h"
#include "mrbc_pico_gpio.h"
#include "mrbc_pico_pwm.h"
#include "mrbc_pico_adc.h"
#include "mrbc_pico_i2c.h"
#include "mrbc_pico_uart.h"
#include "mrbc_pico_bootsel.h"
#include "mrbc_pico_break.h"

#if PICO_CYW43_SUPPORTED
#include "mrbc_pico_cyw43.h"
#endif

//*********************************************
// ENABLE LIBRARY written by C (Utilities)
//*********************************************
#include "bootsel.h"
#include "mrbwrite.h"
#include "vfs.h"

#define MEMORY_SIZE (1024 * 128)
static uint8_t memory_pool[MEMORY_SIZE];

/** @brief プログラムのメイン関数

  ボードの初期化，ファイルシステムの初期化，mrbwriteコマンドモードの処理，
  mruby/cプログラムを実行する．

  @return プログラム終了コード
*/
int main() {
  //************************************
  // ボードの初期化
  //************************************
  stdio_init_all();

  //************************************
  // ファイルシステム初期化
  //************************************
  if (vfs_mount() < 0) {
    printf("ERROR: VFS mount failed\r\n");
    return -1;
  }

  //************************************
  // mrbcwrite モード
  //************************************
  // mrbwriteコマンドモードの待機．
  // a. Enter (CR+LF) が打鍵された場合はコマンドモードに入る．
  // b. BOOTSELボタンが押された場合は実行モードに入る．
  printf("Kani-Board, Please push Enter key to mrbwite mode\r\n");
  while (!bootsel_get()) {
    int cmd = mrbwrite_get_cmd(10 * 1000, NULL, NULL); // 10ミリ秒のタイムアウト
    if (cmd == MRBWRITE_COMMAND_MODE) {
      printf("+OK mruby/c\r\n");
      while (mrbwrite_cmd_mode());
      break; // mrbwriteコマンドモードを抜けた場合は即座に実行モードに入る
    }
  }
  // 書き込みモード終了
  printf("Kani-Board, End mrbwrite mode\r\n");
  printf("Kani-Board, mruby/c v4.0 start\r\n");

  //************************************
  // Rubyコード読み込み
  //************************************
  char filename[16];
  uint32_t size = 0;

  // ライブラリバイトコードの読み込み
  uint32_t lib_count = 0;
  uint8_t **lib_bytecode = NULL;
  while (1) {
    snprintf(filename, sizeof(filename), "lib_%02d.mrbc", lib_count + 1);
    if (vfs_stat_size(filename, &size) < 0 || size == 0) {
      break;
    }
    lib_bytecode = realloc(lib_bytecode, (lib_count + 1) * sizeof(uint8_t *));
    lib_bytecode[lib_count] = calloc(size, sizeof(uint8_t));
    if (lib_bytecode[lib_count] != NULL) {
      vfs_read(filename, lib_bytecode[lib_count], size);
    }
    lib_count++;
  }

  // タスクバイトコードの読み込み
  uint32_t task_count = 0;
  uint8_t **task_bytecode = NULL;
  while (1) {
    snprintf(filename, sizeof(filename), "task_%02d.mrbc", task_count + 1);
    if (vfs_stat_size(filename, &size) < 0 || size == 0) {
      break;
    }
    task_bytecode = realloc(task_bytecode, (task_count + 1) * sizeof(uint8_t *));
    task_bytecode[task_count] = calloc(size, sizeof(uint8_t));
    if (task_bytecode[task_count] != NULL) {
      vfs_read(filename, task_bytecode[task_count], size);
    }
    task_count++;
  }

  //************************************
  // ファイルシステムアンマウント
  //************************************
  vfs_unmount();

  //***************************************
  // Ruby ライブラリ初期化
  //***************************************
  mrbc_init(memory_pool, MEMORY_SIZE);

  mrbc_pico_gpio_gem_init(0);
  mrbc_pico_pwm_gem_init(0);
  mrbc_pico_adc_gem_init(0);
  mrbc_pico_i2c_gem_init(0);
  mrbc_pico_uart_gem_init(0);
  mrbc_pico_bootsel_gem_init(0);
  mrbc_pico_break_gem_init(0);

#if PICO_CYW43_SUPPORTED
  mrbc_pico_cyw43_gem_init(0);
#endif

  // Ruby 側のクラス・メソッド定義
  extern const uint8_t myclass_bytecode[];
  mrbc_run_mrblib(myclass_bytecode);

  // ユーザ独自のライブラリのクラス・メソッド定義
  for (uint32_t i = 0; i < lib_count; i++) {
    if (lib_bytecode[i] != NULL) {
      mrbc_run_mrblib(lib_bytecode[i]);
    }
  }

  //***************************************
  // Ruby 実行
  //***************************************
  if (task_count > 0) {
    for (uint32_t i = 0; i < task_count; i++) {
      if (task_bytecode[i] != NULL) {
        mrbc_create_task(task_bytecode[i], 0);
      }
    }
    mrbc_run();
  } else {
    printf("Not task exists.\r\n");
  }

  // バイトコードの解放
  for (uint32_t i = 0; i < task_count; i++) {
    free(task_bytecode[i]);
  }
  free(task_bytecode);
  for (uint32_t i = 0; i < lib_count; i++) {
    free(lib_bytecode[i]);
  }
  free(lib_bytecode);

  return 0;
}

// ************************************
// main関数で使われるユーティリティ
// ************************************

/** @brief mrbwriteコマンドモードのメイン処理

  ユーザからのコマンドを受け付け，適切な処理を実行する．
  各コマンドに対してバイトコードの書き込み，読み込み，リセットなどを実行する．

  @return 継続時は1，終了時は0を返却する
*/
int mrbwrite_cmd_mode() {
  // writeコマンドで使用するバッファ
  uint32_t buffer_size = 0;
  uint8_t *buffer = NULL;
  int32_t crc = -1;
  // バッファの初期化とコマンド入力の受付
  int cmd = mrbwrite_get_cmd((uint32_t)3600 * 1000 * 1000, &buffer_size, &crc); // 1時間のタイムアウト

  // コマンドエラーでは処理継続
  if (cmd == MRBWRITE_ILLEGAL) {
    printf("-ERR Illegal command.\r\n");
  }
  // タイムアウトでは処理継続
  if (cmd == MRBWRITE_TIMEOUT) {
    printf("-ERR Timeout, exiting command mode.\r\n");
  }
  // コマンドモード継続のメッセージ
  if (cmd == MRBWRITE_COMMAND_MODE) {
    printf("+OK mruby/c\r\n");
  }
  // リセットコマンドの処理
  if (cmd == MRBWRITE_RESET) {
    printf("+OK\r\n");
    watchdog_reboot(0, 0, 100);
    return 0;
  }

  // 実行コマンドの処理
  if (cmd == MRBWRITE_EXECUTE) {
    printf("+OK\r\n");
    return 0;
  }
  // バイトコード書き込みコマンドの処理
  if (cmd == MRBWRITE_WRITE) {
    // 書き込み先の決定
    char filename[16];
    uint32_t size = 0;
    for (int i = 1; ; i++) {
      snprintf(filename, sizeof(filename), "task_%02d.mrbc", i);
      if (vfs_stat_size(filename, &size) < 0) {
        break;
      }
    }

    buffer = calloc(buffer_size, sizeof(uint8_t));
    uint32_t read_count = 0;
    printf("+OK Write bytecode\r\n");

    // バイトコードの連続読み込み
    for (; read_count < buffer_size; read_count++) {
      int input = getchar_timeout_us(60 * 1000 * 1000);
      if (input == PICO_ERROR_TIMEOUT) {
        break;
      }
      buffer[read_count] = (uint8_t)(input & 0xFF);
    }

    // 書き込み結果の判定とファイル保存
    if (read_count != buffer_size) {
      printf("-ERR Timeout while reading bytecode. Expected %d bytes, got %d bytes.\r\n", buffer_size, read_count);
    } else if (crc >= 0 && mrbwrite_crc16(buffer, buffer_size) != (uint16_t)crc) {
      printf("-ERR CRC mismatch.\r\n");
    } else {
      vfs_write(filename, buffer, buffer_size);
      printf("+DONE\r\n");
    }
    free(buffer);
  }
  // ライブラリバイトコード書き込みコマンドの処理
  if (cmd == MRBWRITE_WRITE_LIB) {
    // 書き込み先の決定
    char filename[16];
    uint32_t size = 0;
    for (int i = 1; ; i++) {
      snprintf(filename, sizeof(filename), "lib_%02d.mrbc", i);
      if (vfs_stat_size(filename, &size) < 0) {
        break;
      }
    }

    buffer = calloc(buffer_size, sizeof(uint8_t));
    uint32_t read_count = 0;
    printf("+OK Write bytecode\r\n");

    // バイトコードの連続読み込み
    for (; read_count < buffer_size; read_count++) {
      int input = getchar_timeout_us(60 * 1000 * 1000);
      if (input == PICO_ERROR_TIMEOUT) {
        break;
      }
      buffer[read_count] = (uint8_t)(input & 0xFF);
    }

    // 書き込み結果の判定とファイル保存
    if (read_count != buffer_size) {
      printf("-ERR Timeout while reading bytecode. Expected %d bytes, got %d bytes.\r\n", buffer_size, read_count);
    } else if (crc >= 0 && mrbwrite_crc16(buffer, buffer_size) != (uint16_t)crc) {
      printf("-ERR CRC mismatch.\r\n");
    } else {
      vfs_write(filename, buffer, buffer_size);
      printf("+DONE\r\n");
    }
    free(buffer);
  }
  // ファイル消去コマンドの処理
  if (cmd == MRBWRITE_CLEAR) {
    if (vfs_format() < 0) {
      printf("-ERR Format failed.\r\n");
    } else {
      printf("+OK\r\n");
    }
  }

  // ヘルプコマンドの処理
  if (cmd == MRBWRITE_HELP) {
    printf("+OK\r\n");
    printf("Commands:\r\n");
    printf("  version\r\n");
    printf("  write\r\n");
    printf("  write_lib\r\n");
    printf("  showprog\r\n");
    printf("  clear\r\n");
    printf("  reset\r\n");
    printf("  execute\r\n");
    printf("  verify\r\n");
    printf("+DONE\r\n");
  }

  // バージョン表示コマンドの処理
  if (cmd == MRBWRITE_VERSION) {
    printf("+OK mruby/c v4.0 RITE0400 MRBW1.2\r\n");
  }
  // プログラム表示コマンドの処理
  if (cmd == MRBWRITE_SHOWPROG) {
    char filename[16];
    for (int i = 1; ; i++) {
      snprintf(filename, sizeof(filename), "lib_%02d.mrbc", i);
      if (vfs_stat_size(filename, &buffer_size) < 0) {
        break;
      }
      buffer = calloc(buffer_size, sizeof(uint8_t));
      if (buffer != NULL && vfs_read(filename, buffer, buffer_size) > 0) {
        mrbwrite_showprog(filename, buffer, buffer_size);
        free(buffer);
      }
    }
    for (int i = 1; ; i++) {
      snprintf(filename, sizeof(filename), "task_%02d.mrbc", i);
      if (vfs_stat_size(filename, &buffer_size) < 0) {
        break;
      }
      buffer = calloc(buffer_size, sizeof(uint8_t));
      if (buffer != NULL && vfs_read(filename, buffer, buffer_size) > 0) {
        mrbwrite_showprog(filename, buffer, buffer_size);
        free(buffer);
      }
    }
    printf("+DONE\r\n");
  }

  // バイトコード検証コマンドの処理 [DEPRECATED] mrbwrite v1.3で削除
  if (cmd == MRBWRITE_VERIFY) {
    uint8_t crc = 0x00;
    if (vfs_crc8("task_01.mrbc", &crc) >= 0) {
      printf("+OK %02x\r\n", crc);
    } else {
      printf("-ERR Not task_01.mrbc exists.\r\n");
    }
  }
  return 1;
}
