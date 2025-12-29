# PWM（パルス幅変調）クラス
#
# APIガイドは下記を参照:
# - https://github.com/mruby/microcontroller-peripheral-interface-guide
#
# @example
#   # 周波数1kHz，デューティ比75%でPWMを初期化
#   # 5秒後にデューティ比を0%にして停止
#   pwm = PWM.new(25, frequency: 1000, duty: 75)
#   sleep(5)
#   pwm.duty(0)
class PWM
  GPIO_FUNC_PWM = 4 # C: enum gpio_function_rp2040 { GPIO_FUNC_PWM = 4 }
  CLOCK_INDEX_CLK_SYS = 5 # C: enum clock_index { clk_sys = 5 }

  # PWMインスタンスの初期化
  #
  # ピン番号のみ指定した場合は初期化（停止）．
  # 周波数を指定した場合はデューティ比50%で初期化．
  # 周波数とデューティ比を両方指定した場合はその値で初期化．
  #
  # @param pin [Integer] PWMピン番号
  # @param params [Hash] PWM設定パラメータ
  # @option params [Integer, Float] :frequency PWM周波数（Hz）．frequencyとfreqの両方が指定された場合，frequencyが優先される
  # @option params [Integer, Float] :freq frequencyのエイリアス
  # @option params [Integer, Float] :duty デューティ比（0-100%）
  #
  # @example
  #   # 基本的な使用例（初期化のみ）
  #   pwm = PWM.new(25)
  #
  #   # 周波数とデューティ比を指定して初期化（1kHz/75%）
  #   pwm = PWM.new(25, freq: 1000, duty: 75)
  def initialize(pin, params = {})
    @pin = pin
    @wrap = (params[:wrap] || 10000).to_i.clamp(0, 65535)

    mrbc_pico_gpio_set_function(@pin, GPIO_FUNC_PWM)
    @slice_num = mrbc_pico_pwm_gpio_to_slice_num(@pin)
    mrbc_pico_pwm_set_wrap(@slice_num, @wrap)

    # 周波数の初期値を設定
    freq = params[:frequency] || params[:freq] || 0
    self.frequency(freq)

    # デューティ比の初期値を設定
    duty = params[:duty] || (freq == 0 ? 0 : 50)
    self.duty(duty)
  end

  # PWM周波数の設定
  #
  # 0以下の場合は0として扱い，出力を停止する．
  #
  # @param freq [Integer, Float] 設定する周波数（Hz）
  #
  # @example
  #   pwm.frequency(1000)  # 1kHzに設定
  #   pwm.frequency(0)     # PWM出力を停止
  def frequency(freq)
    # 周波数が0以下の場合は出力を停止
    @freq = freq < 0 ? 0 : freq
    if @freq == 0
      mrbc_pico_pwm_set_enabled(@slice_num, 0)
      return
    end

    # クロック周波数を取得（デフォルトは125MHz）
    clk_sys = mrbc_pico_clock_get_hz(CLOCK_INDEX_CLK_SYS)
    clk_sys = clk_sys > 0 ? clk_sys : 125000000

    # PWM動作周波数・クロック周波数からクロック分周比を計算
    #   (1) clock_divider = system_clock / ((wrap_value + 1) * frequency)
    #   (2) 1.0 <= clock_divider <= 255.0
    clkdiv = clk_sys.to_f / (@freq * (@wrap + 1))
    clkdiv = clkdiv.clamp(1.0, 255.0)

    mrbc_pico_pwm_set_clkdiv(@slice_num, clkdiv)
    mrbc_pico_pwm_set_enabled(@slice_num, 1)
  end

  # PWMデューティ比の設定
  #
  # @param percent [Integer, Float] デューティ比（0-100%）．範囲外の値は0または100になる
  #
  # @example
  #   pwm.duty(50)   # 50%のデューティ比
  #   pwm.duty(75.5) # 75.5%のデューティ比
  def duty(percent)
    # 0 <= level <= wrap
    @duty = percent.clamp(0, 100)
    level = (@duty / 100.0 * @wrap).to_i

    mrbc_pico_pwm_set_gpio_level(@pin, level)
  end

  # PWM周期をマイクロ秒単位で設定
  #
  # 周期は周波数の逆数で計算され，0以下の場合は出力を停止する．
  #
  # @param micro_sec [Integer, Float] 周期（マイクロ秒）
  #
  # @example
  #   pwm.period_us(1000)  # 1000μs（1kHz）の周期
  def period_us(micro_sec)
    @freq = micro_sec <= 0 ? 0 : 1000000.0 / micro_sec
    self.frequency(@freq)
  end

  # PWMパルス幅をマイクロ秒単位で設定
  #
  # @param micro_sec [Integer, Float] パルス幅（マイクロ秒）
  #
  # @example
  #   pwm.pulse_width_us(500)  # 500μsのパルス幅
  def pulse_width_us(micro_sec)
    duty = (micro_sec * @freq / 1000000.0) * 100.0
    duty = duty.clamp(0.0, 100.0)
    self.duty(duty)
  end
end
