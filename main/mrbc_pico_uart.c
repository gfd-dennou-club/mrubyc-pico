/*! @file
  @brief Raspberry Pi Pico向けmruby/c UARTの関数群

  UART（Universal Asynchronous Receiver-Transmitter）非同期シリアル通信をサポートするクラス．
  Ruby向けクラス・メソッドはRubyコードにて実装される．

  APIガイドは下記を参照:
  - https://github.com/mruby/microcontroller-peripheral-interface-guide

  pico-sdkのAPIは下記を参照:
  - https://www.raspberrypi.com/documentation/pico-sdk/hardware.html
*/

#include "mrbc_pico_uart.h"
#include "mrubyc.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <stdio.h>

/*! @brief mrbc_pico_uart_init(unit, baudrate) UARTインターフェースの初期化

  @param unit UARTユニット番号（0または1）
  @param baudrate ボーレート（bps単位）
  @return 実際に設定されたボーレート
*/
void mrbc_pico_uart_init(mrb_vm* vm, mrb_value* v, int argc)
{
  int unit = GET_INT_ARG(1);
  int baudrate = GET_INT_ARG(2);

  uart_inst_t *uart = (unit == 0) ? uart0 : uart1;
  uint actual_baud = uart_init(uart, baudrate);

  SET_INT_RETURN(actual_baud);
}

/*! @brief mrbc_pico_uart_set_hw_flow(unit, cts, rts) UARTハードウェアフロー制御の設定

  @param unit UARTユニット番号（0または1）
  @param cts CTS有効化フラグ（0: 無効，1: 有効）
  @param rts RTS有効化フラグ（0: 無効，1: 有効）
  @return void
*/
void mrbc_pico_uart_set_hw_flow(mrb_vm* vm, mrb_value* v, int argc)
{
  int unit = GET_INT_ARG(1);
  int cts = GET_INT_ARG(2) != 0;
  int rts = GET_INT_ARG(3) != 0;

  uart_inst_t *uart = (unit == 0) ? uart0 : uart1;
  uart_set_hw_flow(uart, cts, rts);
}

/*! @brief mrbc_pico_uart_deinit(unit) UARTインターフェースの無効化

  @param unit UARTユニット番号（0または1）
  @return void
*/
void mrbc_pico_uart_deinit(mrb_vm* vm, mrb_value* v, int argc)
{
  int unit = GET_INT_ARG(1);

  uart_inst_t *uart = (unit == 0) ? uart0 : uart1;
  uart_deinit(uart);
}

/*! @brief mrbc_pico_uart_set_baudrate(unit, baudrate) UARTボーレートの設定

  @param unit UARTユニット番号（0または1）
  @param baudrate ボーレート（bps単位）
  @return 実際に設定されたボーレート
*/
void mrbc_pico_uart_set_baudrate(mrb_vm* vm, mrb_value* v, int argc)
{
  int unit = GET_INT_ARG(1);
  int baudrate = GET_INT_ARG(2);

  uart_inst_t *uart = (unit == 0) ? uart0 : uart1;
  uint actual_baud = uart_set_baudrate(uart, baudrate);

  SET_INT_RETURN(actual_baud);
}

/*! @brief mrbc_pico_uart_set_format(unit, data_bits, stop_bits, parity) UARTフォーマットの設定

  @param unit UARTユニット番号（0または1）
  @param data_bits データビット数（5〜8）
  @param stop_bits ストップビット数（1または2）
  @param parity パリティビット（0: NONE，1: EVEN，2: ODD）
  @return void
*/
void mrbc_pico_uart_set_format(mrb_vm* vm, mrb_value* v, int argc)
{
  int unit = GET_INT_ARG(1);
  int data_bits = GET_INT_ARG(2);
  int stop_bits = GET_INT_ARG(3);
  int parity = GET_INT_ARG(4);

  uart_inst_t *uart = (unit == 0) ? uart0 : uart1;
  uart_parity_t uart_parity = (parity == 1) ? UART_PARITY_EVEN : (parity == 2) ? UART_PARITY_ODD : UART_PARITY_NONE;

  uart_set_format(uart, data_bits, stop_bits, uart_parity);
}

/*! @brief mrbc_pico_uart_is_readable(unit) UARTの読み取り可能チェック

  @param unit UARTユニット番号（0または1）
  @return 読み取り可能な場合は1，それ以外は0
*/
void mrbc_pico_uart_is_readable(mrb_vm* vm, mrb_value* v, int argc)
{
  int unit = GET_INT_ARG(1);

  uart_inst_t *uart = (unit == 0) ? uart0 : uart1;
  bool readable = uart_is_readable(uart);

  SET_INT_RETURN(readable ? 1 : 0);
}

/*! @brief mrbc_pico_uart_is_writable(unit) UARTの書き込み可能チェック

  @param unit UARTユニット番号（0または1）
  @return 書き込み可能な場合は1，それ以外は0
*/
void mrbc_pico_uart_is_writable(mrb_vm* vm, mrb_value* v, int argc)
{
  int unit = GET_INT_ARG(1);

  uart_inst_t *uart = (unit == 0) ? uart0 : uart1;
  bool writable = uart_is_writable(uart);

  SET_INT_RETURN(writable ? 1 : 0);
}

/*! @brief mrbc_pico_uart_getc(unit) UARTから1文字読み取り（ブロッキング）

  @param unit UARTユニット番号（0または1）
  @return 読み取った文字コード
*/
void mrbc_pico_uart_getc(mrb_vm* vm, mrb_value* v, int argc)
{
  int unit = GET_INT_ARG(1);

  uart_inst_t *uart = (unit == 0) ? uart0 : uart1;
  uint8_t ch = uart_getc(uart);

  SET_INT_RETURN(ch);
}

/*! @brief mrbc_pico_uart_putc_raw(unit, c) UARTに1文字書き込み（ブロッキング）

  @param unit UARTユニット番号（0または1）
  @param c 書き込む文字コード
  @return void
*/
void mrbc_pico_uart_putc_raw(mrb_vm* vm, mrb_value* v, int argc)
{
  int unit = GET_INT_ARG(1);
  int c = GET_INT_ARG(2);

  uart_inst_t *uart = (unit == 0) ? uart0 : uart1;
  uart_putc_raw(uart, (char)(c & 0xFF));
}

/** @brief C関数のRubyへの公開

  @param vm mruby/c VM
*/
void mrbc_pico_uart_gem_init(struct VM* vm)
{
  // pico-sdk UARTメソッドのRubyポーティング
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_uart_init", mrbc_pico_uart_init);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_uart_set_hw_flow", mrbc_pico_uart_set_hw_flow);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_uart_deinit", mrbc_pico_uart_deinit);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_uart_set_baudrate", mrbc_pico_uart_set_baudrate);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_uart_set_format", mrbc_pico_uart_set_format);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_uart_is_readable", mrbc_pico_uart_is_readable);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_uart_is_writable", mrbc_pico_uart_is_writable);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_uart_getc", mrbc_pico_uart_getc);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_uart_putc_raw", mrbc_pico_uart_putc_raw);
}
