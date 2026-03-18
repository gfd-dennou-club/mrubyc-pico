#ifndef MRBC_PICO_BREAK_H
#define MRBC_PICO_BREAK_H

struct VM; // #include "mrubyc.h"

// Break信号によるリブート機能を初期化する
void mrbc_pico_break_init(struct VM* vm);

#endif // MRBC_PICO_BREAK_H
