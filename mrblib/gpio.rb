# GPIO (汎用デジタル入出力) クラス
#
# @example
#   led = GPIO.new(25, GPIO::OUT)
#   led.write(1)
#   value = led.read
class GPIO
  # ピンモード定数
  IN = 0x00        # 入力に設定する
  OUT = 0x01       # 出力に設定する
  # HIGH_Z = 0     # ハイインピーダンスに設定する https://forums.raspberrypi.com/viewtopic.php?t=330102
  PULL_UP = 0x10   # 内部プルアップを有効にする
  PULL_DOWN = 0x20 # 内部プルダウンを有効にする
  # OPEN_DRAIN = 0 # オープンドレインモードに設定する https://github.com/raspberrypi/pico-sdk/issues/752

  attr_reader :pin

  # 指定されたピンのGPIOインスタンス初期化
  #
  # @param pin [Integer, String] ピン番号または識別子（文字列の場合は#to_iが呼び出される）
  # @param params [Integer] ピンモード（ピンモード定数を参照）
  # @raise [ArgumentError] ピンまたはパラメータが無効な場合
  def initialize(pin, params)
    @pin = pin.to_i
    setmode(params)
  end

  # ピンの値の読み取り
  #
  # @return [Integer] ピンの値（0または1）
  def read
    mrbc_gpio_get(pin)
  end

  # ピンがハイレベルかどうかの確認
  #
  # @return [Boolean] ピンがハイレベルの場合はtrue
  def high?
    read == 1
  end

  # ピンがローレベルかどうかの確認
  #
  # @return [Boolean] ピンがローレベルの場合はtrue
  def low?
    read == 0
  end

  # ピンへの値の書き込み
  #
  # @param integer_data [Integer] 書き込む値 (0 または 1)
  def write(integer_data)
    mrbc_gpio_put(@pin, integer_data)
    nil
  end

  # ピンモードの設定
  #
  # @param params [Integer] ピンモード（ピンモード定数を参照）
  def setmode(params)
    mrbc_gpio_init(pin)
    mrbc_gpio_set_dir(pin, params & (IN | OUT))
    mrbc_gpio_set_pulls(pin, params & PULL_UP, params & PULL_DOWN)
    nil
  end
end
