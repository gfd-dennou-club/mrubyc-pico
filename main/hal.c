/*! @file
  @brief mruby/cのsleepをコアスリープにする対応

  MRBC_NO_TIMERが定義されるとWFIモードになるためsleep中にもPWMが動作する．
  このときデフォルト（UART出力）のhal_write()，hal_flush()が未定義になるため，
  MRBC_NO_TIMERが定義されている場合もデフォルトのそれらが定義されるようにする．
*/
#include <stdio.h>

#ifdef MRBC_NO_TIMER

// ====================================================================
// BEGIN: mruby/c rp2040 HAL
//
// The original source is available at:
//   Repository : https://github.com/mrubyc/mrubyc
//   URL : https://github.com/mrubyc/mrubyc/blob/ec0125a4f335500fbdb3c7d6ca8eeecd1adbcf04/hal/rp2040/hal.c
//   LICENSE : BSD-3-Clause (https://github.com/mrubyc/mrubyc/blob/ec0125a4f335500fbdb3c7d6ca8eeecd1adbcf04/LICENSE)
//
//   BSD 3-Clause License
//
//   Copyright (c) 2015- Kyushu Institute of Technology All right reserved.
//   Copyright (c) 2015- Shimane IT Open-Innovation Center All right reserved.
//
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright notice, this
//      list of conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.
//
//   3. Neither the name of the copyright holder nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// ====================================================================
/*!@brief
  Write

  @param  fd    dummy, but 1.
  @param  buf   pointer of buffer.
  @param  nbytes        output byte length.

  Memo: Steps to use uart_putc_raw() with hal_write.
  1. Write in main function↓
    uart_init(uart0,115200);
    gpio_set_function(0,GPIO_FUNC_UART);
    gpio_set_function(1,GPIO_FUNC_UART);

  2. Comment out the putchar for hal_write.
  3. Uncomment uart_putc_raw for hal_write.
*/
int hal_write(int fd, const void *buf, int nbytes)
{
  int i = nbytes;
  const uint8_t *p = buf;

  while( --i >= 0 ) {
    putchar( *p++ );
    // uart_putc_raw(uart0, *p++ );
  }

  return nbytes;
}

//================================================================
/*!@brief
  Flush write buffer

  @param  fd    dummy, but 1.
*/
int hal_flush(int fd) {
  return 0;
}
// ====================================================================
// END: mruby/c rp2040 HAL
// ====================================================================

#endif // MRBC_NO_TIMER
