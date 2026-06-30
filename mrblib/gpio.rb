# GPIO（汎用デジタル入出力）クラス
#
# APIガイドは下記を参照:
# - https://github.com/mruby/microcontroller-peripheral-interface-guide
#
# @example
#   led = GPIO.new(25, GPIO::OUT)    # ピン番号で指定
#   led = GPIO.new("LED", GPIO::OUT) # オンボードLED（Pico/Pico 2: GPIO 25，Pico W/Pico 2 W: CYW43 GPIO 0）
#   led.write(1)
#   value = led.read
class GPIO
  # Ruby側のピンモード定数（C拡張のPico SDKが期待する値とは異なる）
  #
  #   |    Mode    | Ruby | Pico SDK |
  #   |------------|------|----------|
  #   | IN         | 0x01 |   0x00   | 入力
  #   | OUT        | 0x02 |   0x01   | 出力
  #   | HIGH_Z     | 0x04 |   ----   | ハイインピーダンス：SDKにAPIなし（INで代用可） https://forums.raspberrypi.com/viewtopic.php?t=330102
  #   | PULL_UP    | 0x08 |   0x10   | 内部プルアップ
  #   | PULL_DOWN  | 0x10 |   0x20   | 内部プルダウン
  #   | OPEN_DRAIN | 0x20 |   ----   | オープンドレインモード：SDKにAPIなし https://github.com/raspberrypi/pico-sdk/issues/752
  IN = 0x01
  OUT = 0x02
  # HIGH_Z = 0x04
  PULL_UP = 0x08
  PULL_DOWN = 0x10
  # OPEN_DRAIN = 0x20

  # 指定されたピンのGPIOインスタンスの初期化
  #
  # @param pin [Integer, String] ピン番号または識別子（"LED"でオンボードLED）
  # @param params [Integer] ピンモード（ピンモード定数を参照）
  # @raise [ArgumentError] ピンまたはパラメータが無効な場合
  #
  # @example
  #   # ピン番号25を出力に設定
  #   pin = GPIO.new(25, GPIO::OUT)
  def initialize(pin, params)
    # 型チェック
    if !pin.is_a?(Integer) && !pin.is_a?(String)
      raise ArgumentError, "Invalid pin type: #{pin.class}"
    end
    if pin.is_a?(String) && pin != "LED"
      raise ArgumentError, "Invalid pin identifier: #{pin}"
    end
    if !params.is_a?(Integer)
      raise ArgumentError, "Invalid params type: #{params.class}"
    end

    # IN または OUT の指定は必須（HIGH_Zは未サポート）
    if params & (IN | OUT) == 0
      raise ArgumentError, "IN or OUT must be specified"
    end

    @pin = pin
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
    if cyw43_pin?
      mrbc_pico_cyw43_gpio_get(pin_number)
    else
      mrbc_pico_gpio_get(pin_number)
    end
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

  # ピンへの値の書き込み
  #
  # @param integer_data [Integer] 書き込む値（0 または 1）
  #
  # @example
  #   pin.write(1)
  def write(integer_data)
    if cyw43_pin?
      mrbc_pico_cyw43_gpio_put(pin_number, integer_data)
    else
      mrbc_pico_gpio_put(pin_number, integer_data)
    end
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
    # CYW43経由のオンボードLEDはモード設定不要（cyw43_arch_initで初期化済み）
    return nil if cyw43_pin?

    real_mode = 0
    real_mode |= 0x01 if params & OUT != 0
    real_mode |= 0x10 if params & PULL_UP != 0
    real_mode |= 0x20 if params & PULL_DOWN != 0

    # IN/OUTが指定された場合は初期化と方向設定を行う
    if params & (IN | OUT) != 0
      mrbc_pico_gpio_init(pin_number)
      mrbc_pico_gpio_set_dir(pin_number, real_mode & 0x01)
    end

    # PULL_UP/PULL_DOWNは常に適用
    mrbc_pico_gpio_set_pulls(pin_number, real_mode & 0x10, real_mode & 0x20)
    nil
  end

  private

    # 実際のピン番号への解決
    #
    # 初期化時に識別子（"LED"文字列）が指定されていた場合，ボード固有のデフォルトLEDのピン番号に解決する．
    #
    # @return [Integer] ピン番号
    def pin_number
      @pin == "LED" ? mrbc_pico_default_led_pin : @pin
    end

    # ピンがCYW43ドライバ経由で制御されるピンかどうか
    #
    # @return [Boolean] CYW43ドライバ経由の場合はtrue
    def cyw43_pin?
      @pin == "LED" && mrbc_pico_default_led_controlled_by_cyw43?
    end
end
