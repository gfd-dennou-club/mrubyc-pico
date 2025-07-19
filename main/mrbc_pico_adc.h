#ifndef MRBC_PICO_ADC_H
#define MRBC_PICO_ADC_H

struct VM; // #include "mrubyc.h"

// C関数をRubyへ公開する
void mrbc_pico_adc_gem_init(struct VM* vm);

#endif // MRBC_PICO_ADC_H
