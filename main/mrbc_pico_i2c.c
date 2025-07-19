/*! @file
  @brief Raspberry Pi Pico向けmruby/c I2Cの関数群

  I2Cバスをサポートするクラス．7ビットアドレスのマスターデバイスとして動作する．

  APIガイドは下記を参照:
  - https://github.com/mruby/microcontroller-peripheral-interface-guide

  pico-sdkのAPIは下記を参照:
  - https://www.raspberrypi.com/documentation/pico-sdk/hardware.html
*/

#include "mrbc_pico_i2c.h"
#include "mrubyc.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdio.h>

/*! @brief mrbc_pico_i2c_init(unit, frequency) I2Cインターフェースの初期化

  @param unit I2Cユニット番号（0または1）
  @param frequency I2C周波数（Hz単位）
  @return 実際に設定された周波数
*/
void mrbc_pico_i2c_init(mrb_vm* vm, mrb_value* v, int argc)
{
  int unit = GET_INT_ARG(1);
  int frequency = GET_INT_ARG(2);

  i2c_inst_t *i2c = (unit == 0) ? i2c0 : i2c1;
  uint actual_freq = i2c_init(i2c, frequency);

  SET_INT_RETURN(actual_freq);
}

/*! @brief mrbc_pico_i2c_deinit(unit) I2Cインターフェースの無効化

  @param unit I2Cユニット番号（0または1）
  @return void
*/
void mrbc_pico_i2c_deinit(mrb_vm* vm, mrb_value* v, int argc)
{
  int unit = GET_INT_ARG(1);

  i2c_inst_t *i2c = (unit == 0) ? i2c0 : i2c1;
  i2c_deinit(i2c);
}

/*! @brief mrbc_pico_i2c_write_blocking(unit, addr, str, nostop) I2Cデバイスへのバイト列の書き込み

  @param unit I2Cユニット番号（0または1）
  @param addr 7ビットI2Cアドレス
  @param str 書き込むデータ（バイト列をStringとして）
  @param nostop ストップコンディションを送信しない場合は1
  @return 書き込んだバイト数．エラーの場合は負の値
*/
void mrbc_pico_i2c_write_blocking(mrb_vm* vm, mrb_value* v, int argc)
{
  int unit = GET_INT_ARG(1);
  int addr = GET_INT_ARG(2);
  mrb_value str = GET_ARG(3);
  int nostop = GET_INT_ARG(4);

  uint8_t *buf = (uint8_t *)mrbc_string_cstr(&str);
  int len = mrbc_string_size(&str);
  if (len <= 0) {
    SET_INT_RETURN(0);
    return;
  }

  i2c_inst_t *i2c = (unit == 0) ? i2c0 : i2c1;
  int result = i2c_write_blocking(i2c, addr, buf, len, nostop);
  SET_INT_RETURN(result);
}

/*! @brief mrbc_pico_i2c_read_blocking(unit, addr, len, nostop) I2Cデバイスからのバイト列の読み込み

  @param unit I2Cユニット番号（0または1）
  @param addr 7ビットI2Cアドレス
  @param len 読み込むバイト数
  @param nostop ストップコンディションを送信しない場合は1
  @return 読み込んだデータ（バイト列をStringとして）．エラーの場合は空文字列
*/
void mrbc_pico_i2c_read_blocking(mrb_vm* vm, mrb_value* v, int argc)
{
  int unit = GET_INT_ARG(1);
  int addr = GET_INT_ARG(2);
  int len = GET_INT_ARG(3);
  int nostop = GET_INT_ARG(4);

  if (len <= 0) {
    SET_RETURN(mrbc_string_new(vm, "", 0));
    return;
  }

  // 先にStringを確保し、内部バッファに直接読み込む
  mrb_value rval = mrbc_string_new(vm, NULL, len);
  if (rval.string == NULL) {
    SET_RETURN(mrbc_string_new(vm, "", 0));
    return;
  }
  uint8_t *buf = (uint8_t *)mrbc_string_cstr(&rval);

  i2c_inst_t *i2c = (unit == 0) ? i2c0 : i2c1;
  int result = i2c_read_blocking(i2c, addr, buf, len, nostop);

  if (result < 0) {
    SET_RETURN(mrbc_string_new(vm, "", 0));
    return;
  }
  SET_RETURN(rval);
}

/** @brief C関数のRubyへの公開

  @param vm mruby/c VM
*/
void mrbc_pico_i2c_gem_init(struct VM* vm)
{
  // pico-sdk I2CメソッドのRubyポーティング
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_i2c_init", mrbc_pico_i2c_init);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_i2c_deinit", mrbc_pico_i2c_deinit);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_i2c_write_blocking", mrbc_pico_i2c_write_blocking);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_i2c_read_blocking", mrbc_pico_i2c_read_blocking);
}
