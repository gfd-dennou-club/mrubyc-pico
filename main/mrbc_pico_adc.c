/*! @file
  @brief Raspberry Pi Pico向けmruby/c ADCの関数群

  ADC（Analog to Digital Converter）制御機能をRubyから利用可能にする．
  mruby/c I/O APIガイド実装のためのADC制御関数をRubyから利用可能にする．
  ADC実際のAPIはRubyコードにて実装される．

  APIガイドは下記を参照:
  - https://github.com/mruby/microcontroller-peripheral-interface-guide

  pico-sdkのAPIは下記を参照:
  - https://www.raspberrypi.com/documentation/pico-sdk/hardware.html
*/
#include "mrbc_pico_adc.h"
#include "mrubyc.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include <stdio.h>

/*! @brief mrbc_pico_adc_gpio_init(pin) ADC用GPIOピンの初期化

  @param pin GPIOピン番号
  @return void
*/
void mrbc_pico_adc_gpio_init(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  adc_gpio_init(pin);
}

/*! @brief mrbc_pico_adc_select_input(input) ADC入力の選択

  * RP2040 input=0-​3 => GPIO=26-29
  * RP2350 input=0-​7 => GPIO=40-​47

  @param input ADC入力番号
  @return void
*/
void mrbc_pico_adc_select_input(mrb_vm* vm, mrb_value* v, int argc)
{
  int input = GET_INT_ARG(1);
  adc_select_input(input);
}

/*! @brief mrbc_pico_adc_read() ADCからの値の読み取り

  @return ADCの生の値（12bit=0-4095）
*/
void mrbc_pico_adc_read(mrb_vm* vm, mrb_value* v, int argc)
{
  SET_INT_RETURN(adc_read());
}

/*! @brief mrbc_pico_adc_set_temp_sensor_enabled(enabled) 温度センサーの有効化/無効化

  @param enabled 有効化フラグ（0: 無効，1: 有効）
  @return void
*/
void mrbc_pico_adc_set_temp_sensor_enabled(mrb_vm* vm, mrb_value* v, int argc)
{
  int enabled = GET_INT_ARG(1) != 0; // bool
  adc_set_temp_sensor_enabled(enabled);
}

/** @brief C関数のRubyへの公開

  @param vm mruby/c VM
*/
void mrbc_pico_adc_gem_init(struct VM* vm)
{
  adc_init();

  // pico-sdk ADCメソッドのRubyポーティング
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_adc_gpio_init", mrbc_pico_adc_gpio_init);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_adc_select_input", mrbc_pico_adc_select_input);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_adc_read", mrbc_pico_adc_read);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_adc_set_temp_sensor_enabled", mrbc_pico_adc_set_temp_sensor_enabled);
}
