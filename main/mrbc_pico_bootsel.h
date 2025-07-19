#ifndef MRBC_PICO_BOOTSEL_H
#define MRBC_PICO_BOOTSEL_H

struct VM; // #include "mrubyc.h"

// C関数をRubyへ公開する
void mrbc_pico_bootsel_gem_init(struct VM* vm);

#endif // MRBC_PICO_BOOTSEL_H
