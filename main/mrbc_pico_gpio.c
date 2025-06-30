/*
* mruby/c I/O APIガイド実装のためのGPIO制御関数をRubyから利用可能にする。
*
* GPIO実際のAPIはRubyコードにて実装される。
*
* APIガイドは下記を参照：
* - https://github.com/mruby/microcontroller-peripheral-interface-guide
*/
#include "mrbc_pico_gpio.h"
#include "hardware/gpio.h"
#include "mrubyc.h"

// #define GPIO_IN          0x00 (defined in the SDK)
// #define GPIO_OUT         0x01 (defined in the SDK)
// #define GPIO_IN_PULLUP   0x10
// #define GPIO_IN_PULLDOWN 0x20

void mrbc_pico_gpio_init(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  gpio_init(pin);
}

void mrbc_pico_gpio_set_dir(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  int out = GET_INT_ARG(2) != 0; // bool
  gpio_set_dir(pin, out);
}

void mrbc_pico_gpio_set_pulls(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  int pullup = GET_INT_ARG(2) != 0; // bool
  int pulldown = GET_INT_ARG(3) != 0; // bool
  gpio_set_pulls(pin, pullup, pulldown);
}

void mrbc_pico_gpio_put(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin   = GET_INT_ARG(1);
  int level = GET_INT_ARG(2);
  gpio_put(pin, level);
}

void mrbc_pico_gpio_get(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  SET_INT_RETURN(gpio_get(pin));
}

void mrbc_pico_gpio_gem_init(struct VM* vm)
{
  // Rubyのメソッド定義（Objectクラスに追加）
  mrbc_define_method(0, mrbc_class_object, "mrbc_gpio_init", mrbc_pico_gpio_init);
  mrbc_define_method(0, mrbc_class_object, "mrbc_gpio_set_dir", mrbc_pico_gpio_set_dir);
  mrbc_define_method(0, mrbc_class_object, "mrbc_gpio_set_pulls", mrbc_pico_gpio_set_pulls);
  mrbc_define_method(0, mrbc_class_object, "mrbc_gpio_put", mrbc_pico_gpio_put);
  mrbc_define_method(0, mrbc_class_object, "mrbc_gpio_get", mrbc_pico_gpio_get);

  // // Rubyクラス名を定義
  // mrbc_class *mrbc_class = mrbc_define_class(0, "GPIO", 0);

  // // Rubyのメソッド定義（GPIOクラスに追加）
  // mrbc_define_method(0, mrbc_class, "sample", mrbc_pico_gpio_sample);
}
