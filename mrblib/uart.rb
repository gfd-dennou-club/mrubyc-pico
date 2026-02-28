# UART（Universal Asynchronous Receiver-Transmitter）クラス
#
# 非同期シリアル通信をサポートするクラス．
# デフォルト設定: ボーレート9600，データ8ビット，ストップビット1，パリティなし．
#
# APIガイドは下記を参照:
# - https://github.com/mruby/microcontroller-peripheral-interface-guide
#
# @example 基本的な使い方
#   # UART1を全てデフォルトパラメータで使う
#   uart1 = UART.new(1)
#
#   # データを送信する
#   uart1.write("Hello, UART!\n")
#
#   # データを受信する（10バイト）
#   data = uart1.read(10)
class UART
  # パリティビット定数
  NONE = 0  # パリティなし
  EVEN = 1  # 偶数パリティ
  ODD = 2   # 奇数パリティ

  # GPIOの機能定数（UART用）
  GPIO_FUNC_UART = 2 # C: enum gpio_function_rp2040 { GPIO_FUNC_UART = 2 }

  attr_reader :unit, :baudrate, :data_bits, :stop_bits, :parity

  # 指定された物理ユニットでUARTオブジェクトの生成
  #
  # @param id [Integer] 物理ユニット番号（0または1，省略時は1を推奨）
  # @param baudrate [Integer] ボーレート（bps単位，デフォルト: 9600）
  # @param baud [Integer] baudrateのエイリアス
  # @param data_bits [Integer] データビット数（5〜8，デフォルト: 8）
  # @param stop_bits [Integer] ストップビット数（1または2，デフォルト: 1）
  # @param parity [Integer] パリティビット（NONE, EVEN, ODD，デフォルト: NONE）
  # @param unit [Integer] ユニット番号（idのエイリアス）
  # @param txd_pin [Integer] TxDピン番号（オプション）
  # @param rxd_pin [Integer] RxDピン番号（オプション）
  #
  # RP2040 UART対応ピン:
  #   UART0: TX=0,12,16,28  RX=1,13,17,29
  #   UART1: TX=4,8,20,24   RX=5,9,21,25
  #
  # @example
  #   uart1 = UART.new(1)
  #
  #   # ユニット1のUARTデバイスをボーレート19200bps，偶数パリティで使用
  #   uart2 = UART.new(1, baudrate: 19200, parity: UART::EVEN)
  def initialize(id = nil, baudrate: 9600, baud: nil, data_bits: 8, stop_bits: 1, parity: NONE, unit: nil, txd_pin: nil, rxd_pin: nil)
    @unit = (unit || id || 1).to_i  # デフォルトはUART1（UART0はmrbwriteで使用中）
    @baudrate = (baud || baudrate).to_i
    @data_bits = data_bits.to_i.clamp(5, 8)
    @stop_bits = stop_bits.to_i.clamp(1, 2)
    @parity = parity.to_i.clamp(0, 2)

    # デフォルトピンの設定
    # RP2040: UART0 => GP0(TX), GP1(RX), UART1 => GP4(TX), GP5(RX)
    if @unit == 0
      @txd_pin = txd_pin || 0
      @rxd_pin = rxd_pin || 1
    else
      @txd_pin = txd_pin || 4
      @rxd_pin = rxd_pin || 5
    end

    # UART初期化
    actual_baud = mrbc_pico_uart_init(@unit, @baudrate)
    @baudrate = actual_baud

    # フロー制御を無効化（uart_init後の状態が不定のため）
    mrbc_pico_uart_set_hw_flow(@unit, 0, 0)

    # UARTフォーマット設定
    mrbc_pico_uart_set_format(@unit, @data_bits, @stop_bits, @parity)

    # GPIOピンをUART機能に設定
    mrbc_pico_gpio_set_function(@txd_pin, GPIO_FUNC_UART)
    mrbc_pico_gpio_set_function(@rxd_pin, GPIO_FUNC_UART)
  end

  # UARTのモード（パラメータ）の変更
  #
  # @param baudrate [Integer] ボーレート（bps単位）
  # @param data_bits [Integer] データビット数（5〜8）
  # @param stop_bits [Integer] ストップビット数（1または2）
  # @param parity [Integer] パリティビット（NONE, EVEN, ODD）
  #
  # @return [void]
  #
  # @example
  #   uart1.setmode(baudrate: 38400)
  def setmode(baudrate: nil, data_bits: nil, stop_bits: nil, parity: nil)
    if baudrate
      @baudrate = baudrate.to_i
      actual_baud = mrbc_pico_uart_set_baudrate(@unit, @baudrate)
      @baudrate = actual_baud
    end

    if data_bits || stop_bits || parity
      @data_bits = data_bits.to_i.clamp(5, 8) if data_bits
      @stop_bits = stop_bits.to_i.clamp(1, 2) if stop_bits
      @parity = parity.to_i.clamp(0, 2) if parity

      mrbc_pico_uart_set_format(@unit, @data_bits, @stop_bits, @parity)
    end
  end

  # UARTからデータの読み込み（ブロッキング）
  #
  # @param read_bytes [Integer] 読み込むバイト数
  #
  # @return [String] 読み込んだデータ（バイナリ文字列）
  #
  # @example
  #   # 10バイト読み込み
  #   data = uart1.read(10)
  def read(read_bytes)
    result = ""
    read_bytes.times do
      result << mrbc_pico_uart_getc(@unit).chr
    end
    result
  end

  # UARTへデータの書き込み（ブロッキング）
  #
  # @param string [String, Array<Integer>] 書き込むデータ（文字列または配列）
  #
  # @return [Integer] 書き込んだバイト数
  #
  # @example
  #   # 文字列を送信
  #   uart1.write("Output string\r\n")
  def write(string)
    count = 0
    if string.is_a?(String)
      string.each_byte do |byte|
        mrbc_pico_uart_putc_raw(@unit, byte)
        count += 1
      end
    elsif string.is_a?(Array)
      string.each do |byte|
        mrbc_pico_uart_putc_raw(@unit, byte)
        count += 1
      end
    end
    count
  end

  # UARTから1行読み込み（ブロッキング）
  #
  # @return [String] 読み込んだ行（改行を含む）
  #
  # @example
  #   line = uart1.gets()
  def gets()
    result = ""
    loop do
      if mrbc_pico_uart_is_readable(@unit) == 1
        ch = mrbc_pico_uart_getc(@unit)
        result << ch.chr
        break if ch == 10  # LF
      end
    end
    result
  end

  # UARTに1行送信（ブロッキング）
  #
  # 引数stringが改行で終わっていない場合は改行コード（LF）を送信する．
  #
  # @param string [String] 送信する文字列
  #
  # @return [nil]
  #
  # @example
  #   uart1.puts("Output string")
  def puts(string)
    write(string)
    if string.is_a?(String) && !string.end_with?("\n")
      write("\n")
    end
    nil
  end

  # リードバッファの読み込み可能バイト数の取得
  #
  # @return [Integer] 読み込み可能バイト数（簡易実装: readable?の結果を返す）
  #
  # @example
  #   len = uart1.bytes_available()
  def bytes_available()
    mrbc_pico_uart_is_readable(@unit)
  end

  # 1行読み込みが可能かの確認（簡易実装）
  #
  # @return [Boolean] 読み込み可能な場合はtrue，それ以外はfalse
  #
  # @example
  #   if uart1.can_read_line()
  #     line = uart1.gets()
  #   end
  def can_read_line()
    mrbc_pico_uart_is_readable(@unit) == 1
  end

  # 送信バッファのフラッシュ（簡易実装: 何もしない）
  #
  # @return [void]
  #
  # @example
  #   uart1.flush()
  def flush()
    # Pico SDK には flush 相当の機能がないため，何もしない
  end

  # UARTインターフェースの無効化
  #
  # @return [void]
  #
  # @example
  #   uart1.deinit
  def deinit
    mrbc_pico_uart_deinit(@unit)
  end
end
