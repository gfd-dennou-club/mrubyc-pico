/*! @file
  @brief Raspberry Pi Pico向けmruby/c BOOTSELボタンの関数群

  Raspberry Pi PicoのBOOTSELボタンの状態を取得する機能をRubyから利用可能にする．
  BOOTSELボタンはmruby/cの標準APIではないため，pico-sdkの実装に依存する．

  pico-sdkのAPIは下記を参照:
  - https://www.raspberrypi.com/documentation/pico-sdk/hardware.html
*/
#include "mrbc_pico_bootsel.h"
#include "bootsel.h"
#include "mrubyc.h"

/*! @brief mrbc_pico_bootsel_get() BOOTSELボタンの状態の取得

  @return BOOTSELボタンの状態（0: 押されていない，1: 押されている）
*/
void mrbc_pico_bootsel_get(mrb_vm* vm, mrb_value* v, int argc) {
  SET_INT_RETURN(bootsel_get());
}

/** @brief C関数のRubyへの公開

  @param vm mruby/c VM
*/
void mrbc_pico_bootsel_gem_init(struct VM* vm)
{
  // Rubyのメソッド定義（Objectクラスに追加）
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_bootsel_get", mrbc_pico_bootsel_get);
}
