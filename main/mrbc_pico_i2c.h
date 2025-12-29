#ifndef MRBC_PICO_I2C_H
#define MRBC_PICO_I2C_H

struct VM; // #include "mrubyc.h"

// C関数をRubyへ公開する
void mrbc_pico_i2c_gem_init(struct VM* vm);

#endif // MRBC_PICO_I2C_H
