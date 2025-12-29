# I2C（Inter-Integrated Circuit）クラス
#
# I2Cバスをサポートするクラス．7ビットアドレスのマスターデバイスとして動作する．
#
# APIガイドは下記を参照:
# - https://github.com/mruby/microcontroller-peripheral-interface-guide
#
# @example 基本的な使い方
#   # デフォルトの設定でI2Cオブジェクトを生成
#   i2c = I2C.new()
#
#   # デバイスアドレス0x45から3バイト読み込み
#   data = i2c.read(0x45, 3)
#
#   # デバイスアドレス0x45に2バイト書き込み
#   i2c.write(0x45, 0x30, 0xa2)
class I2C
  # GPIOの機能定数（I2C用）
  GPIO_FUNC_I2C = 3 # C: enum gpio_function_rp2040 { GPIO_FUNC_I2C = 3 }

  attr_reader :unit, :frequency, :scl_pin, :sda_pin

  # 指定された物理ユニットでI2Cオブジェクトの生成
  #
  # @param id [Integer] 物理ユニット番号（0または1，デフォルト: 0）
  # @param frequency [Integer] I2C周波数（Hz単位，デフォルト: 100kHz）
  # @param freq [Integer] frequencyのエイリアス
  # @param scl_pin [Integer] SCLピン番号（デフォルト: RP2040では5）
  # @param sda_pin [Integer] SDAピン番号（デフォルト: RP2040では4）
  #
  # RP2040 I2C対応ピン:
  #   I2C0: SDA=0,4,8,12,16,20  SCL=1,5,9,13,17,21
  #   I2C1: SDA=2,6,10,14,18,26 SCL=3,7,11,15,19,27
  #
  # @example
  #   i2c = I2C.new
  #
  #   # ユニット1のI2Cデバイスを周波数400kHzで使用
  #   i2c0 = I2C.new(1, frequency: 400_000)
  #
  #   # # カスタムピンを指定
  #   # i2c0 = I2C.new(0, scl_pin: 9, sda_pin: 8)
  #   # i2c1 = I2C.new(1, scl_pin: 7, sda_pin: 6)
  def initialize(id = 0, frequency: 100_000, freq: nil, scl_pin: nil, sda_pin: nil)
    @unit = id.to_i

    # 周波数の決定（freqとfrequencyの両方が指定された場合はfrequencyを優先）
    # RP2040/RP2350のI2CはFast Mode Plus（最大1MHz）まで対応
    # @see https://picodocs.pinout.xyz/group__hardware__i2c.html
    @frequency = (freq || frequency).to_i.clamp(1, 1_000_000)

    # デフォルトピンの設定
    # RP2040: I2C0 => GP4(SDA), GP5(SCL), I2C1 => GP6(SDA), GP7(SCL)
    # [TODO] RP2350: I2C0 => GP8(SDA), GP9(SCL), I2C1 => GP10(SDA), GP11(SCL)
    if @unit == 0
      @scl_pin = scl_pin || 5  # RP2040のデフォルト
      @sda_pin = sda_pin || 4
    else
      @scl_pin = scl_pin || 7  # RP2040のデフォルト
      @sda_pin = sda_pin || 6
    end

    # I2C初期化
    mrbc_pico_i2c_init(@unit, @frequency)

    # GPIOピンをI2C機能に設定
    mrbc_pico_gpio_set_function(@sda_pin, GPIO_FUNC_I2C)
    mrbc_pico_gpio_set_function(@scl_pin, GPIO_FUNC_I2C)

    # プルアップ抵抗を有効化
    mrbc_pico_gpio_set_pulls(@sda_pin, 1, 0)  # pullup=true, pulldown=false
    mrbc_pico_gpio_set_pulls(@scl_pin, 1, 0)
  end

  # I2Cデバイスからデータの読み込み
  #
  # @param i2c_adrs_7 [Integer] 7ビットI2Cアドレス
  # @param read_bytes [Integer] 読み込むバイト数
  # @param param [Array<Integer>] オプション: 書き込んでからリピーテッドスタートを挟んで読み込む場合のデータ
  #
  # @return [String] 読み込んだデータ（バイナリ文字列）
  #
  # @example
  #   # デバイスアドレス0x45から3バイト読み込み
  #   data = i2c.read(0x45, 3)
  #
  #   # レジスタ0xf3から3バイト読み込み（リピーテッドスタート）
  #   data = i2c.read(0x45, 3, 0xf3)
  def read(i2c_adrs_7, read_bytes, *param)
    unless param.empty?
      # 書き込み処理（リピーテッドスタート用、nostop=1）
      write_data = ""
      param.each { |p| write_data << p.to_i.chr }
      result = mrbc_pico_i2c_write_blocking(@unit, i2c_adrs_7, write_data, 1)
      return "" if result < 0  # エラー時は空文字列を返す
    end

    # 全バイトを1トランザクションで読み込む
    mrbc_pico_i2c_read_blocking(@unit, i2c_adrs_7, read_bytes, 0)
  end

  # I2Cデバイスにデータの書き込み
  #
  # @param i2c_adrs_7 [Integer] 7ビットI2Cアドレス
  # @param outputs [Array<Integer>, Integer] 書き込むデータ（整数または配列）
  #
  # @return [Integer] 書き込んだバイト数．エラーの場合は負の値
  #
  # @example
  #   # デバイスアドレス0x45に2バイト書き込み
  #   i2c.write(0x45, 0x30, 0xa2)
  #
  #   # 配列で書き込み
  #   i2c.write(0x45, [0x30, 0xa2])
  #
  #   # EEPROMへの書き込み例
  #   i2c.write(0x50, 0x00, 0x80, data_string)
  def write(i2c_adrs_7, *outputs)
    # バイト列を文字列として構築
    data = ""
    outputs.each do |output|
      if output.is_a?(Array)
        output.each { |e| data << e.to_i.chr }
      elsif output.is_a?(String)
        data << output
      else
        data << output.to_i.chr
      end
    end

    return 0 if data.empty?

    # 全バイトを1トランザクションで書き込む
    mrbc_pico_i2c_write_blocking(@unit, i2c_adrs_7, data, 0)
  end

  # I2Cインターフェースの無効化
  #
  # @return [void]
  #
  # @example
  #   i2c.deinit
  def deinit
    mrbc_pico_i2c_deinit(@unit)
  end
end
