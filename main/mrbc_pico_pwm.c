/*! @file
  @brief Raspberry Pi Pico向けmruby/c PWMの関数群

  PWM（Pulse Width Modulation）制御機能をRubyから利用可能にする．
  mruby/c I/O APIガイド実装のためのPWM制御関数をRubyから利用可能にする．
  PWM実際のAPIはRubyコードにて実装される．

  APIガイドは下記を参照:
  - https://github.com/mruby/microcontroller-peripheral-interface-guide

  pico-sdkのAPIは下記を参照:
  - https://www.raspberrypi.com/documentation/pico-sdk/hardware.html
*/
#include "mrbc_pico_pwm.h"
#include "mrubyc.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"

/*! @brief mrbc_pico_pwm_gpio_to_slice_num(pin) GPIOピンに対応するPWMスライス番号の取得

  @param pin GPIOピン番号
  @return PWMスライス番号
*/
void mrbc_pico_pwm_gpio_to_slice_num(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  uint slice_num = pwm_gpio_to_slice_num(pin);
  SET_INT_RETURN(slice_num);
}

/*! @brief mrbc_pico_pwm_set_enabled(slice_num, enabled) PWMスライスの有効/無効の設定

  @param slice_num PWMスライス番号
  @param enabled 有効化フラグ（0: 無効，1: 有効）
  @return void
*/
void mrbc_pico_pwm_set_enabled(mrb_vm* vm, mrb_value* v, int argc)
{
  int slice_num = GET_INT_ARG(1);
  int enabled = GET_INT_ARG(2) != 0; // bool
  pwm_set_enabled(slice_num, enabled);
}

/*! @brief mrbc_pico_pwm_set_clkdiv(slice_num, clkdiv) PWMクロック分周比の設定

  @param slice_num PWMスライス番号
  @param clkdiv クロック分周比
  @return void
*/
void mrbc_pico_pwm_set_clkdiv(mrb_vm* vm, mrb_value* v, int argc)
{
  int slice_num = GET_INT_ARG(1);
  float clkdiv = GET_FLOAT_ARG(2);
  pwm_set_clkdiv(slice_num, clkdiv);
}

/*! @brief mrbc_pico_pwm_set_wrap(slice_num, wrap) PWMカウンタ最大値の設定

  @param slice_num PWMスライス番号
  @param wrap カウンタ最大値
  @return void
*/
void mrbc_pico_pwm_set_wrap(mrb_vm* vm, mrb_value* v, int argc)
{
  int slice_num = GET_INT_ARG(1);
  int wrap = GET_INT_ARG(2);
  pwm_set_wrap(slice_num, wrap);
}

/*! @brief mrbc_pico_pwm_set_gpio_level(pin, level) PWMデューティ比の設定

  Highになる期間を設定し，デューティ比を決定する．

  @param pin GPIOピン番号
  @param level High期間の値
  @return void
*/
void mrbc_pico_pwm_set_gpio_level(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  int level = GET_INT_ARG(2);
  pwm_set_gpio_level(pin, level);
}

/*! @brief mrbc_pico_clock_get_hz(clk_index) クロック周波数の取得

  @param clk_index クロックの種類
  @return クロックの周波数（Hz）
*/
void mrbc_pico_clock_get_hz(mrb_vm* vm, mrb_value* v, int argc)
{
  int clk_index = GET_INT_ARG(1);
  uint32_t frequency = clock_get_hz(clk_index);
  SET_INT_RETURN(frequency);
}

/** @brief C関数のRubyへの公開

  @param vm mruby/c VM
*/
void mrbc_pico_pwm_gem_init(struct VM* vm)
{
  // Rubyのメソッド定義（Objectクラスに追加）
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_pwm_gpio_to_slice_num", mrbc_pico_pwm_gpio_to_slice_num);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_pwm_set_enabled", mrbc_pico_pwm_set_enabled);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_pwm_set_clkdiv", mrbc_pico_pwm_set_clkdiv);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_pwm_set_wrap", mrbc_pico_pwm_set_wrap);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_pwm_set_gpio_level", mrbc_pico_pwm_set_gpio_level);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_clock_get_hz", mrbc_pico_clock_get_hz);
}
