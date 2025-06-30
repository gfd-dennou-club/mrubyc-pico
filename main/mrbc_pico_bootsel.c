#include "mrbc_pico_bootsel.h"
#include "bootsel.h"
#include "mrubyc.h"

void mrbc_pico_bootsel_get(mrb_vm* vm, mrb_value* v, int argc) {
  SET_INT_RETURN(bootsel_get());
}

void mrbc_pico_bootsel_gem_init(struct VM* vm)
{
  // Rubyのメソッド定義（Objectクラスに追加）
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_bootsel_get", mrbc_pico_bootsel_get);
}
// ====================================================================
// END: Our original code
// ====================================================================
