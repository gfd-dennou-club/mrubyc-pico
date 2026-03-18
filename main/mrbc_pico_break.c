/*! @file
  @brief Break信号によるリブート機能

  USB CDCのBreak信号を受信した際に，ウォッチドッグタイマーを使ってソフトリセットする．
*/

#include "mrbc_pico_break.h"
#include "tusb.h"
#include "hardware/watchdog.h"

static int mode = 0;  // 0: 未初期化，1: 有効，2: 再起動実行済

/*! @brief TinyUSBのBreak信号受信コールバック

  USB CDCでBreak信号を受信した際にTinyUSBから呼び出される．
  （TinyUSBのweakシンボルを上書きしてコールバック登録）

  ウォッチドッグタイマを使って100ms後に再起動する．

  @param itf CDCインターフェース番号（未使用）
  @param duration_ms Break信号の持続時間（未使用）
*/
void tud_cdc_send_break_cb(uint8_t itf, uint16_t duration_ms) {
  (void)itf;
  (void)duration_ms;
  if (mode == 1) {
    mode = 2;
    watchdog_reboot(0, 0, 100);
  }
}

/*! @brief Break信号によるリブート機能の初期化

  @param vm mruby/c VMインスタンス（未使用）
*/
void mrbc_pico_break_init(struct VM* vm) {
  (void)vm;

  // 有効化（0 → 1）
  mode = 1;
}
