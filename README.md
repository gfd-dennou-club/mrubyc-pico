# mrubyc-pico

Raspberry Pi Pico / Pico 2向けmruby/cファームウェア・クラスライブラリです．

クラスライブラリは [mruby, mruby/c 共通I/O APIガイドライン](https://github.com/mruby/microcontroller-peripheral-interface-guide) に準拠します．

ボードへの電源の投入後，BOOTSELボタンが押下されるまでmrbwriteを待機します．
このときCR+LFがボードのUSB（UART）へ送信された場合にコマンド受付モードに入ります．

mrbwriteで書き込まれたRubyは，電源投入後のBOOTSELボタン押下で実行されます．

Ruby実行中，または終了後にボードのUSB（UART）へBreak信号が送信された場合，ソフトリセットされます。

## 対応ボード

| ボード | チップ | ビルドターゲット |
|--------|--------|------------------|
| Raspberry Pi Pico | RP2040 | `make pico` |
| Raspberry Pi Pico 2 | RP2350 | `make pico2` |

## クラスライブラリ

| クラス | 機能 | 共通I/O API |
|--------|------|---|
| GPIO | デジタル入出力 | ○ |
| PWM | PWM出力 | ○ |
| ADC | アナログ入力 | ○ |
| I2C | I2C通信 | ○ |
| UART | UART通信 | ○ |
| BOOTSEL | BOOTSELボタン読み取り | - |

## ビルド

### 前提

- cmake
- gcc-arm-none-eabi
- ruby（CRuby 3.4+）

### 手順

```bash
# 初回
git clone --recurse-submodules https://github.com/gfd-dennou-club/mrubyc-pico
cd mrubyc-pico
make setup   # mrubyコンパイラ（mrbc）のビルド

# ファームウェア変更のたび（利用するボードのコマンドを実行）
make pico    # => build/pico/main.uf2
make pico2   # => build/pico2/main.uf2
make all     # => 上記すべて
```

ビルド成果物は`build/{pico,pico2}/main.uf2`に出力されます．

Raspberry Pi PicoボードのBOOTSELボタンを押しながらUSBケーブルでPCと接続すると，USBメモリのようにファイルを書き込めるようになります．

上記のUF2ファイルをドラッグ＆ドロップなどで配置します．
（書き込み後は自動でRaspberry Pi Picoは再起動します）

### Ruby プログラムの編集

`src/master.rb`がメインファイルです．`mrblib/`以下にクラス・メソッドを定義できます．

## Depends on

- [mruby/c (mrubyc/mrubyc)](https://github.com/mrubyc/mrubyc) : mruby 仮想マシンの軽量実装
- [mruby (mruby/mruby)](https://github.com/mruby/mruby) : mrbc コンパイラの提供元
- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)
- [LittleFS](https://github.com/littlefs-project/littlefs) : フラッシュファイルシステム

## Acknowledgement

Originally forked from [picoruby/mrubyc-pico](https://github.com/picoruby/mrubyc-pico)
and [aikawaYO/mrubyc-pico](https://github.com/aikawaYO/mrubyc-pico).

## License

See [LICENSE](LICENSE).
