# HTTPクライアントクラス
#
# @example
#   body = HTTP.get("http://example.com/")
#   body = HTTP.get("https://example.com/")
#   body = HTTP.post("https://api.example.com/data", '{"k":1}')
class HTTP
  # 接続タイムアウト（ms）
  CONNECT_TIMEOUT_MS = 15000
  # 受信タイムアウト（ms, 1チャンク毎）
  READ_TIMEOUT_MS    = 10000

  # GETリクエスト
  #
  # @param url [String] URL（例：https://example.com/path/to/resources）
  # @param params [Hash] :user, :passwd でBasic認証
  # @return [String] レスポンスボディ（接続失敗時は空文字列）
  def self.get(url, params = {})
    return "" unless url.start_with?("http://") || url.start_with?("https://")
    host, port, path, tls = URI.parse(url)
    return "" unless host

    default_port = tls ? (port == 443) : (port == 80)
    host_hdr = default_port ? host : "#{host}:#{port}"
    raw = "GET #{path} HTTP/1.1\r\n"
    raw << "Host: #{host_hdr}\r\n"
    raw << "User-Agent: mrubyc-pico\r\n"
    raw << "Accept: */*\r\n"
    raw << "Connection: close\r\n"
    if params[:user] && params[:passwd]
      token = mrbc_pico_mbedtls_base64_encode("#{params[:user]}:#{params[:passwd]}")
      raw << "Authorization: Basic #{token}\r\n"
    end
    raw << "\r\n"

    scheme = tls ? "https" : "http"
    request("#{scheme}://#{host}:#{port}", raw)
  end

  # POSTリクエスト（Content-Type: application/json固定）
  #
  # @param url [String] URL（例：https://example.com/path/to/resources）
  # @param body [String] POSTボディ
  # @param params [Hash] :user, :passwd でBasic認証
  # @return [String] レスポンスボディ（接続失敗時は空文字列）
  def self.post(url, body, params = {})
    return "" unless url.start_with?("http://") || url.start_with?("https://")
    host, port, path, tls = URI.parse(url)
    return "" unless host

    default_port = tls ? (port == 443) : (port == 80)
    host_hdr = default_port ? host : "#{host}:#{port}"
    raw = "POST #{path} HTTP/1.1\r\n"
    raw << "Host: #{host_hdr}\r\n"
    raw << "User-Agent: mrubyc-pico\r\n"
    raw << "Accept: */*\r\n"
    raw << "Connection: close\r\n"
    if params[:user] && params[:passwd]
      token = mrbc_pico_mbedtls_base64_encode("#{params[:user]}:#{params[:passwd]}")
      raw << "Authorization: Basic #{token}\r\n"
    end
    raw << "Content-Type: application/json\r\n"
    raw << "Content-Length: #{body.length}\r\n"
    raw << "\r\n"
    raw << body

    scheme = tls ? "https" : "http"
    request("#{scheme}://#{host}:#{port}", raw)
  end

  # 任意のHTTPリクエスト
  #
  # @param target [String] 接続先（例：https://example.com/path/to/resources）
  # @param raw [String] 送信するHTTP文字列（リクエストライン＋ヘッダ＋空行＋body）
  # @return [String] レスポンスボディ（接続失敗時は空文字列）
  def self.request(target, raw)
    host, port, _path, tls = URI.parse(target)
    return "" unless host

    return "" unless TCP.open(host, port, CONNECT_TIMEOUT_MS, tls)

    begin
      TCP.write(raw)

      received = ""
      until TCP.closed?
        chunk = TCP.read(READ_TIMEOUT_MS)
        break if chunk.empty?
        received << chunk
      end
      last = TCP.read(0)
      received << last unless last.empty?

      idx = received.index("\r\n\r\n")
      return received unless idx
      received[idx + 4, received.length - idx - 4]
    ensure
      TCP.close
    end
  end

  # URI解析クラス（内部用）
  class URI
    # URIの解析
    #
    # @param url [String] 解析対象のURL（例：https://example.com/path/to/resources）
    # @return [Array(String, Integer, String, Boolean)] [host, port, path, tls]
    def self.parse(url)
      tls = false
      rest = url
      if rest.start_with?("https://")
        rest = rest[8, rest.length - 8]
        tls = true
      elsif rest.start_with?("http://")
        rest = rest[7, rest.length - 7]
      end
      slash_idx = rest.index("/")
      if slash_idx
        host_port = rest[0, slash_idx]
        path = rest[slash_idx, rest.length - slash_idx]
      else
        host_port = rest
        path = "/"
      end
      colon_idx = host_port.index(":")
      if colon_idx
        host = host_port[0, colon_idx]
        port = host_port[colon_idx + 1, host_port.length - colon_idx - 1].to_i
      else
        host = host_port
        port = tls ? 443 : 80
      end
      [host, port, path, tls]
    end
  end

  # TCP操作クラス（内部用）
  #
  # C実装がシングルトンなため同時1接続前提．
  class TCP
    # lwIPロックを取得してからブロックの実行
    def self.with_lock
      mrbc_pico_cyw43_lwip_begin
      yield
    ensure
      mrbc_pico_cyw43_lwip_end
    end

    # TCP接続の確立
    #
    # @param host [String] ホスト名またはIPアドレス文字列
    # @param port [Integer] ポート番号
    # @param timeout_ms [Integer] 各段階のタイムアウト
    # @param tls [Boolean] TLSを有効にする場合true
    # @return [Boolean] 接続成功時true，失敗時false
    def self.open(host, port, timeout_ms, tls = false)
      ok = with_lock do
        tls ? mrbc_pico_lwip_tcp_open_tls : mrbc_pico_lwip_tcp_open
      end
      return false unless ok

      ok = with_lock do
        mrbc_pico_lwip_dns_start(host)
      end
      unless ok
        close
        return false
      end
      remaining_ms = timeout_ms.to_i
      while !mrbc_pico_lwip_dns_done? && remaining_ms > 0
        sleep_ms(10)
        remaining_ms -= 10
      end
      ip4 = mrbc_pico_lwip_dns_result
      unless ip4
        close
        return false
      end

      err = with_lock do
        mrbc_pico_lwip_tcp_connect(ip4, port.to_i)
      end
      if err != 0
        close
        return false
      end

      remaining_ms = timeout_ms.to_i
      while !mrbc_pico_lwip_tcp_connected? && !mrbc_pico_lwip_tcp_closed? && remaining_ms > 0
        sleep_ms(10)
        remaining_ms -= 10
      end
      unless mrbc_pico_lwip_tcp_connected?
        close
        return false
      end
      true
    end

    # TCPでのデータ送信
    #
    # @param data [String] 送信データ
    # @return [Integer] 送信バイト数（失敗時 -1）
    def self.write(data)
      err = with_lock do
        e = mrbc_pico_lwip_tcp_write(data)
        mrbc_pico_lwip_tcp_output if e == 0
        e
      end
      return -1 if err != 0
      data.length
    end

    # 受信バッファの読み出し
    #
    # バッファが空ならタイムアウトまでデータ到着または接続終了を待機する．
    #
    # @param timeout_ms [Integer] 待機タイムアウト
    # @return [String] 受信バイト列（無し時は空文字列）
    def self.read(timeout_ms)
      remaining_ms = timeout_ms.to_i
      while mrbc_pico_lwip_tcp_rx_size == 0 && !mrbc_pico_lwip_tcp_closed? && remaining_ms > 0
        sleep_ms(10)
        remaining_ms -= 10
      end
      with_lock do
        mrbc_pico_lwip_tcp_recv_pop
      end
    end

    # TCP接続の切断
    def self.close
      with_lock do
        mrbc_pico_lwip_tcp_close
      end
    end

    # 切断検知の確認
    def self.closed?
      mrbc_pico_lwip_tcp_closed?
    end
  end
end
