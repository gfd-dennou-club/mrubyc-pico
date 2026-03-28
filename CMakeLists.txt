# ====================================================================
#  mrubyc-picoプロジェクトのCMakeビルド設定
#
#  Raspberry Pi Pico向けmruby/c実行環境のビルド設定を定義する．
# ====================================================================

cmake_minimum_required(VERSION 3.16)

# picotoolのインストール先（ない場合はダウンロード）
set(PICOTOOL_FETCH_FROM_GIT_PATH ${CMAKE_SOURCE_DIR}/components/picotool CACHE PATH "")

# pico-sdkのビルド設定
set(PICO_SDK_PATH ${CMAKE_SOURCE_DIR}/components/pico-sdk CACHE STRING "")

# Pico SDKの導入およびプロジェクト設定
include(components/pico-sdk/pico_sdk_init.cmake)
project(main)

# Pico SDKの初期化
pico_sdk_init()

# mruby/cのコンパイルオプションの設定
add_compile_options(-DMRBC_NO_TIMER -DMRBC_USE_HAL_RP2040 -DMRBC_REQUIRE_32BIT_ALIGNMENT -DMAX_REGS_SIZE=256 -DMAX_SYMBOLS_COUNT=500)

# mainターゲットの作成
add_executable(main)

# mainの構成ファイル
target_sources(main PUBLIC
  main/main.c
  main/mrbc_pico_gpio.c
  main/mrbc_pico_pwm.c
  main/mrbc_pico_adc.c
  main/mrbc_pico_i2c.c
  main/mrbc_pico_uart.c
  main/mrbc_pico_bootsel.c
  main/mrbc_pico_break.c
  main/bootsel.c
  main/hal.c
  main/mrbwrite.c
  main/vfs.c
  ${CMAKE_CURRENT_BINARY_DIR}/mrblib.c
  components/mrubyc/src/alloc.c
  components/mrubyc/src/c_array.c
  components/mrubyc/src/c_hash.c
  components/mrubyc/src/c_math.c
  components/mrubyc/src/c_numeric.c
  components/mrubyc/src/c_object.c
  components/mrubyc/src/c_proc.c
  components/mrubyc/src/c_range.c
  components/mrubyc/src/c_string.c
  components/mrubyc/src/class.c
  components/mrubyc/src/console.c
  components/mrubyc/src/error.c
  components/mrubyc/src/global.c
  components/mrubyc/src/keyvalue.c
  components/mrubyc/src/load.c
  components/mrubyc/src/mrblib.c
  components/mrubyc/src/rrt0.c
  components/mrubyc/src/symbol.c
  components/mrubyc/src/value.c
  components/mrubyc/src/vm.c
  components/mrubyc/hal/rp2040/hal.c
  components/littlefs/lfs.c
  components/littlefs/lfs_util.c
)

# Pico SDKのライブラリをリンク
target_link_libraries(main
  pico_stdlib
  hardware_pwm
  hardware_adc
  hardware_i2c
  hardware_uart
)

# mainでincludeされるファイル
target_include_directories(main PUBLIC
  components/mrubyc/src
  components/mrubyc/hal/rp2040
  components/littlefs
)

# mrubyコンパイラのパス
set(MRBC ${CMAKE_SOURCE_DIR}/components/mruby/build/host/bin/mrbc)

# mruby/c組み込みのRubyクラスのコンパイル（出力先: components/mrubyc/src/mrblib.c）
add_dependencies(main mrubyc)
add_custom_target(mrubyc
  COMMAND make distclean all MRBC=${MRBC}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/components/mrubyc/mrblib
)

# 独自定義のRubyクラスのコンパイル（出力先: build/mrblib.c）
file(GLOB MRBLIB_RUBY_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/mrblib/*.rb")
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mrblib.c
  COMMAND ${MRBC} -B myclass_bytecode --remove-lv -o ${CMAKE_CURRENT_BINARY_DIR}/mrblib.c ${MRBLIB_RUBY_SOURCES}
  DEPENDS ${MRBLIB_RUBY_SOURCES}
)

# UARTをUSBに向ける
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

# make後に.uf2ファイルを出力
pico_add_extra_outputs(main)
