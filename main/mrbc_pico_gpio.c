/*! @file
  @brief Raspberry Pi Pico向けmruby/c GPIOの関数群

  GPIO_IN          : 0x00 (defined in the SDK)
  GPIO_OUT         : 0x01 (defined in the SDK)
  GPIO_IN_PULLUP   : 0x10
  GPIO_IN_PULLDOWN : 0x20

  mruby/c I/O APIガイド実装のためのGPIO制御関数をRubyから利用可能にする．
  Ruby向けクラス・メソッドはRubyコードにて実装される．

  APIガイドは下記を参照:
  - https://github.com/mruby/microcontroller-peripheral-interface-guide

  pico-sdkのAPIは下記を参照:
  - https://www.raspberrypi.com/documentation/pico-sdk/hardware.html
*/
#include "mrbc_pico_gpio.h"
#include "mrubyc.h"
#include "hardware/gpio.h"

/*! @brief mrbc_pico_gpio_init(pin) GPIOピンの初期化

  @param pin GPIOピン番号
  @return void
*/
void mrbc_pico_gpio_init(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  gpio_init(pin);
}

/*! @brief mrbc_pico_gpio_set_dir(pin, out) GPIOピンの入出力方向の設定

  @param pin GPIOピン番号
  @param out 出力方向の設定（0: 入力，1: 出力）
  @return void
*/
void mrbc_pico_gpio_set_dir(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  int out = GET_INT_ARG(2) != 0; // bool
  gpio_set_dir(pin, out);
}

/*! @brief mrbc_pico_gpio_set_pulls(pin, pullup, pulldown) GPIOピンのプルアップ／ダウン抵抗の設定

  @param pin GPIOピン番号
  @param pullup プルアップ抵抗の有効化（0: 無効，1: 有効）
  @param pulldown プルダウン抵抗の有効化（0: 無効，1: 有効）
  @return void
*/
void mrbc_pico_gpio_set_pulls(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  int pullup = GET_INT_ARG(2) != 0; // bool
  int pulldown = GET_INT_ARG(3) != 0; // bool
  gpio_set_pulls(pin, pullup, pulldown);
}

/*! @brief mrbc_pico_gpio_put(pin, level) GPIOピンの出力レベルの設定

  @param pin GPIOピン番号
  @param level 出力レベル（0: Low，1: High）
  @return void
*/
void mrbc_pico_gpio_put(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin   = GET_INT_ARG(1);
  int level = GET_INT_ARG(2);
  gpio_put(pin, level);
}

/*! @brief mrbc_pico_gpio_get(pin) GPIOピンの入力レベルの取得

  @param pin GPIOピン番号
  @return GPIOから読み取られた値（0: Low，1: High）
*/
void mrbc_pico_gpio_get(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  SET_INT_RETURN(gpio_get(pin));
}

/*! @brief mrbc_pico_gpio_set_function(pin, mode) GPIOピンの機能の設定

  PWMなどの特定機能を使用する際に必要．

  @param pin GPIOピン番号
  @param mode 機能モード
  @return void
*/
void mrbc_pico_gpio_set_function(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  int mode = GET_INT_ARG(2);
  gpio_set_function(pin, mode);
}

/** @brief C関数のRubyへの公開

  @param vm mruby/c VM
*/
void mrbc_pico_gpio_gem_init(struct VM* vm)
{
  // Rubyのメソッド定義（Objectクラスに追加）
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_gpio_init", mrbc_pico_gpio_init);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_gpio_set_dir", mrbc_pico_gpio_set_dir);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_gpio_set_pulls", mrbc_pico_gpio_set_pulls);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_gpio_put", mrbc_pico_gpio_put);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_gpio_get", mrbc_pico_gpio_get);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_gpio_set_function", mrbc_pico_gpio_set_function);
}
