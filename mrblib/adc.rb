# ADC（アナログ・デジタル変換）クラス
#
# APIガイドは下記を参照:
# - https://github.com/mruby/microcontroller-peripheral-interface-guide
#
# @example
#   # GPIO26をADCピンとして初期化し，電圧と生のADC値を読み取る
#   adc = ADC.new(26)
#   voltage = adc.read_voltage
#   raw_value = adc.read_raw
#   puts "Voltage: #{voltage}/3.3, Raw value: #{raw_value}/4096"
class ADC
  # ADC変換定数
  ADC_MAX_VALUE = 4095        # 12bit ADCの最大値
  REFERENCE_VOLTAGE = 3.3     # 基準電圧（V）
  CONVERSION_FACTOR = REFERENCE_VOLTAGE / ADC_MAX_VALUE

  # ADCインスタンスの初期化
  #
  # @param pin [Integer] ADCピン番号
  # @param params [Hash] ADC設定パラメータ
  # @raise [ArgumentError] ピンが無効な場合
  #
  # @example
  #   # GPIO26をADCピンとして初期化
  #   adc = ADC.new(26)
  def initialize(pin, params = {})
    if !pin.is_a?(Integer)
      raise ArgumentError, "Invalid pin type: #{pin.class}"
    end

    @pin = pin

    @channel = case @pin
    when 26 then 0
    when 27 then 1
    when 28 then 2
    when 29 then 3
    end

    if @channel.nil?
      raise ArgumentError, "Invalid ADC pin: #{@pin}"
    end

    mrbc_pico_adc_gpio_init(@pin)
    mrbc_pico_adc_select_input(@channel)
  end

  # ADC値をボルト単位で読み取り
  #
  # @return [Float] 電圧値（V）
  #
  # @example
  #   voltage = adc.read_voltage()
  #   puts "Voltage: #{voltage}V"
  def read_voltage
    read_raw * CONVERSION_FACTOR
  end

  # ADC値をボルト単位で読み取り（read_voltageのエイリアス）
  #
  # @return [Float] 電圧値（V）
  #
  # @example
  #   voltage = adc.read()
  def read
    read_voltage
  end

  # ADCの生の値の読み取り
  #
  # @return [Integer] 12bitのADC値（0-4095）
  #
  # @example
  #   raw_value = adc.read_raw()
  #   puts "Raw ADC: #{raw_value}"
  def read_raw
    mrbc_pico_adc_read()
  end
end
