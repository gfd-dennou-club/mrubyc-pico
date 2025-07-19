# GPIO（汎用デジタル入出力）クラス
#
# APIガイドは下記を参照:
# - https://github.com/mruby/microcontroller-peripheral-interface-guide
#
# @example
#   led = GPIO.new(25, GPIO::OUT)
#   led.write(1)
#   value = led.read
class GPIO
  # ピンモード定数
  IN = 0x00        # 入力に設定
  OUT = 0x01       # 出力に設定
  # HIGH_Z = 0     # ハイインピーダンスに設定 https://forums.raspberrypi.com/viewtopic.php?t=330102
  PULL_UP = 0x10   # 内部プルアップを有効化
  PULL_DOWN = 0x20 # 内部プルダウンを有効化
  # OPEN_DRAIN = 0 # オープンドレインモードに設定 https://github.com/raspberrypi/pico-sdk/issues/752

  attr_reader :pin

  # 指定されたピンのGPIOインスタンスの初期化
  #
  # @param pin [Integer, String] ピン番号または識別子（文字列の場合は#to_iが呼び出される）
  # @param params [Integer] ピンモード（ピンモード定数を参照）
  # @raise [ArgumentError] ピンまたはパラメータが無効な場合
  #
  # @example
  #   # ピン番号25を出力に設定
  #   pin = GPIO.new(25, GPIO::OUT)
  def initialize(pin, params)
    if !pin.is_a?(Integer) && !pin.is_a?(String)
      raise ArgumentError, "Invalid pin type: #{pin.class}"
    end
    if !params.is_a?(Integer)
      raise ArgumentError, "Invalid params type: #{params.class}"
    end
    # IN または OUT の指定は必須（HIGH_Zは未サポート）
    if params & (IN | OUT) == 0
      raise ArgumentError, "IN or OUT must be specified"
    end
    @pin = pin.to_i
    setmode(params)
  end

  # ピンの値の読み取り
  #
  # @return [Integer] ピンの値（0または1）
  #
  # @example
  #   value = pin.read
  #   if value == 1
  #     puts "High"
  #   else
  #     puts "Low"
  #   end
  def read
    mrbc_pico_gpio_get(pin)
  end

  # ピンの値がハイレベル（1）かどうかの確認
  #
  # @return [Boolean] ピンがハイレベルの場合はtrue
  #
  # @example
  #   if pin.high?
  #     puts "High"
  #   end
  def high?
    read == 1
  end

  # ピンの値がローレベル（0）かどうかの確認
  #
  # @return [Boolean] ピンがローレベルの場合はtrue
  #
  # @example
  #   if pin.low?
  #     puts "Low"
  #   end
  def low?
    read == 0
  end

  # ピンへ値の書き込み
  #
  # @param integer_data [Integer] 書き込む値（0 または 1）
  #
  # @example
  #   pin.write(1)
  def write(integer_data)
    mrbc_pico_gpio_put(@pin, integer_data)
    nil
  end

  # ピンモードの設定
  #
  # IN/OUTが指定された場合は入出力を変更し，PULL_UP/PULL_DOWNは無効化される．
  # PULL_UP/PULL_DOWNのみ指定された場合は入出力の向きは変更されない．
  #
  # @param params [Integer] ピンモード（ピンモード定数を参照）
  #
  # @example
  #   pin.setmode(GPIO::IN | GPIO::PULL_UP)
  def setmode(params)
    # IN/OUTが指定された場合は初期化と方向設定を行う
    direction = params & (IN | OUT)
    if direction != 0
      mrbc_pico_gpio_init(pin)
      mrbc_pico_gpio_set_dir(pin, direction)
    end

    # PULL_UP/PULL_DOWNは常に適用
    mrbc_pico_gpio_set_pulls(pin, params & PULL_UP, params & PULL_DOWN)
    nil
  end
end
