# BOOTSELクラス
#
# Raspberry Pi PicoのBOOTSELボタンの状態を読み取るクラス．
# BOOTSELボタンは起動時のBOOTSELモードだけでなく，実行時にも読み取り可能．
#
# @example
#   bootsel = BOOTSEL.new
#   if bootsel.high?
#     puts "BOOTSEL pressed"
#   end
class BOOTSEL
  # BOOTSELボタンの状態の読み取り
  #
  # @return [Integer] ボタンの状態（0: 押されていない，1: 押されている）
  #
  # @example
  #   state = bootsel.read
  #   puts "BOOTSEL: #{state}"
  def read
    mrbc_pico_bootsel_get
  end

  # BOOTSELボタンが押されているかどうかの確認
  #
  # @return [Boolean] ボタンが押されている場合はtrue
  #
  # @example
  #   if bootsel.high?
  #     puts "BOOTSEL pressed"
  #   end
  def high?
    read == 1
  end

  # BOOTSELボタンが押されていないかどうかの確認
  #
  # @return [Boolean] ボタンが押されていない場合はtrue
  #
  # @example
  #   if bootsel.low?
  #     puts "BOOTSEL not pressed"
  #   end
  def low?
    read == 0
  end
end
