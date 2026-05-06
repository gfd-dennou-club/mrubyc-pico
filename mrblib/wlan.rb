# WiFi（WLAN）クラス
#
# @example
#   wlan = WLAN.new
#   wlan.connect("SSID", "PASSWORD")
#   if wlan.connected?
#     puts wlan.ip
#   end
class WLAN
  # WiFi SSIDのスキャン結果
  #
  # C拡張から各属性が設定される．
  class ScanResult
    # @attribute [rw] ssid
    #   @return [String] SSID文字列
    # @attribute [rw] bssid
    #   @return [String] BSSID（6バイトのバイト列）
    # @attribute [rw] channel
    #   @return [Integer] チャンネル番号
    # @attribute [rw] rssi
    #   @return [Integer] 信号強度（dBm）
    # @attribute [rw] auth_mode
    #   @return [Integer] 認証モード（ビットマスク）
    attr_accessor :ssid, :bssid, :channel, :rssi, :auth_mode
  end

  # WiFi接続ステータス定数
  LINK_DOWN = 0     # 未接続
  LINK_UP = 3       # 接続済み
  LINK_FAIL = -1    # 接続失敗
  LINK_NONET = -2   # ネットワーク未検出
  LINK_BADAUTH = -3 # 認証失敗

  # WiFiドライバの初期化
  #
  # STAモードを有効化する．
  #
  # @example
  #   wlan = WLAN.new
  def initialize
    mrbc_pico_cyw43_enable_sta_mode
  end

  # WiFiネットワークへの接続
  #
  # WPA2認証で接続する．接続結果はconnected?メソッドで確認できる．
  # オプションでタイムアウト付き接続（省略時は無限待機）する．
  #
  # @param ssid [String] SSID
  # @param password [String] パスワード
  # @param params [Hash] オプション
  # @option params [Integer] :timeout_ms タイムアウト時間（ミリ秒）
  #
  # @example 無限待機（ESP32互換）
  #   wlan.connect("ssid", "password")
  #
  # @example タイムアウト付き
  #   wlan.connect("ssid", "password", timeout_ms: 30000)
  #   if wlan.connected?
  #     puts "Connected!"
  #   end
  def connect(ssid, password, params = {})
    timeout_ms = params[:timeout_ms]
    if timeout_ms
      mrbc_pico_cyw43_wifi_connect_timeout_ms(ssid, password, timeout_ms.to_i)
    else
      mrbc_pico_cyw43_wifi_connect_blocking(ssid, password)
    end
  end

  # WiFi接続状態の確認
  #
  # @return [Boolean] 接続済みかどうか
  #
  # @example
  #   if wlan.connected?
  #     puts "Connected!"
  #   end
  def connected?
    mrbc_pico_cyw43_tcpip_link_status == LINK_UP
  end

  # 周辺WiFiネットワークのスキャン
  #
  # スキャン開始後，完了まで待機する．
  #
  # @return [Array<Hash>] スキャン結果の配列
  #   各要素は {
  #     "ssid" => "...",
  #     "bssid" => "XX:XX:...",
  #     "channel" => N,
  #     "rssi" => N,
  #     "authmode" => "...",
  #     "hidden" => false
  #    }
  #
  # @example
  #   results = wlan.scan
  #   results.each do |ap|
  #     puts "#{ap["ssid"]} (#{ap["rssi"]}dBm) #{ap["authmode"]}"
  #   end
  def scan
    # WiFiスキャン完了まで待機
    return [] if mrbc_pico_cyw43_wifi_scan != 0
    while mrbc_pico_cyw43_wifi_scan_active
      sleep_ms(100)
    end

    # スキャン結果の整形
    mrbc_pico_cyw43_wifi_scan_results.map do |result|
      {
        "ssid" => result.ssid,
        "bssid" => format_mac(result.bssid),
        "channel" => result.channel,
        "rssi" => result.rssi,
        "authmode" => auth_mode_name(result.auth_mode),
        "hidden" => false
      }
    end
  end

  # MACアドレスの取得
  #
  # @return [String] MACアドレス文字列 "XX:XX:XX:XX:XX:XX"
  #
  # @example
  #   puts wlan.mac  #=> "28:CD:C1:00:11:22"
  def mac
    format_mac(mrbc_pico_cyw43_wifi_get_mac)
  end

  # IPアドレスの取得
  #
  # @return [String] IPアドレス文字列 "x.x.x.x"
  #
  # @example
  #   puts wlan.ip  #=> "192.168.1.100"
  def ip
    begin
      mrbc_pico_cyw43_lwip_begin
      raw_ip = mrbc_pico_lwip_ip4_addr
    ensure
      mrbc_pico_cyw43_lwip_end
    end
    format_ip(raw_ip)
  end

  # ネットワーク設定の取得
  #
  # @return [Hash] {"ip"=>"...", "netmask"=>"...", "gw"=>"...", "dns"=>"..."}
  #
  # @example
  #   config = wlan.ifconfig
  #   puts config["ip"]
  #   puts config["gw"]
  def ifconfig
    begin
      mrbc_pico_cyw43_lwip_begin
      raw_ip = mrbc_pico_lwip_ip4_addr
      raw_netmask = mrbc_pico_lwip_ip4_netmask
      raw_gw = mrbc_pico_lwip_ip4_gw
      raw_dns = mrbc_pico_lwip_ip4_dns
    ensure
      mrbc_pico_cyw43_lwip_end
    end
    {
      "ip" => format_ip(raw_ip),
      "netmask" => format_ip(raw_netmask),
      "gw" => format_ip(raw_gw),
      "dns" => format_ip(raw_dns)
    }
  end

  private

    # 6バイトのバイト列の"XX:XX:XX:XX:XX:XX"への変換
    def format_mac(raw)
      s = ""
      6.times do |i|
        s += ":" if i > 0
        hex = raw.getbyte(i).to_s(16).upcase
        s += "0" if hex.length < 2
        s += hex
      end
      s
    end

    # 4バイトのバイト列の"x.x.x.x"への変換
    def format_ip(raw)
      s = ""
      4.times do |i|
        s += "." if i > 0
        s += raw.getbyte(i).to_s
      end
      s
    end

    # auth_modeビットマスクの文字列への変換
    # bit0(1): WEP, bit1(2): WPA, bit2(4): WPA2
    def auth_mode_name(auth_mode)
      return "OPEN" if auth_mode == 0
      return "WPA/WPA2 PSK" if auth_mode & 6 == 6
      return "WPA2 PSK" if auth_mode & 4 != 0
      return "WPA PSK" if auth_mode & 2 != 0
      return "WEP" if auth_mode & 1 != 0
      "Unknown"
    end
end
