cmake_minimum_required(VERSION 3.16)

# picotoolのインストール先（ない場合はダウンロード）
set(PICOTOOL_FETCH_FROM_GIT_PATH ${CMAKE_SOURCE_DIR}/components/picotool CACHE PATH "")
# pico-sdkのビルド設定
set(PICO_SDK_PATH ${CMAKE_SOURCE_DIR}/components/pico-sdk CACHE STRING "")
set(PICO_PLATFORM rp2040 CACHE STRING "")
set(PICO_BOARD pico CACHE STRING "")
set(PICO_COMPILER pico_arm_cortex_m0plus_gcc CACHE STRING "")

# Pico SDKの導入およびプロジェクト設定
include(components/pico-sdk/pico_sdk_init.cmake)
project(main)

# Pico SDKの初期化
pico_sdk_init()

# mruby/cのコンパイルオプションの設定
add_compile_options(-DMRBC_USE_HAL_RP2040 -DMRBC_REQUIRE_32BIT_ALIGNMENT -DMAX_REGS_SIZE=256 -DMAX_SYMBOLS_COUNT=500)

# mainの構成ファイル
add_executable(main
  main/main.c
  main/mrbc_pico_gpio.c
  main/mrbc_pico_bootsel.c
  main/bootsel.c
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
target_link_libraries(main pico_stdlib)

# mainでincludeされるファイル
target_include_directories(main PUBLIC
  components/mrubyc/src
  components/mrubyc/hal/rp2040
  components/littlefs
)

# mruby/c組み込みのRubyクラスのコンパイル（components/mrubyc/mrblib/*.rb）
# コンパイル結果はcomponents/mrubyc/src/mrblib.cへ出力される
add_dependencies(main mrubyc)
add_custom_target(mrubyc
  COMMAND make distclean all
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/components/mrubyc/mrblib
)

# 独自定義のRubyクラスのコンパイル（mrblib/*.rb）
# コンパイル結果はbuild/mrblib.cへ出力される
file(GLOB MRBLIB_RUBY_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/mrblib/*.rb")
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mrblib.c
  COMMAND mrbc -B myclass_bytecode --remove-lv -o ${CMAKE_CURRENT_BINARY_DIR}/mrblib.c ${MRBLIB_RUBY_SOURCES}
  DEPENDS ${MRBLIB_RUBY_SOURCES}
)

# UARTをUSBに向ける
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

# make後にファームウェアである.uf2を出力する
pico_add_extra_outputs(main)
