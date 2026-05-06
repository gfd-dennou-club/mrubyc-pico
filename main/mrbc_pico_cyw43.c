/*! @file
  @brief Raspberry Pi Pico W / Pico2 W向けCYW43チップ制御のための関数群

  CYW43ドライバによるGPIO・通信制御のための関数をRubyから利用可能にする．
  Ruby向けクラス・メソッドはRubyコードにて実装される．

  mruby/cペリフェラルガイドにない独自拡張．

  pico-sdkのAPIは下記を参照:
  - https://www.raspberrypi.com/documentation/pico-sdk/networking.html
*/
#include "mrbc_pico_cyw43.h"
#include "mrubyc.h"
#include "pico/cyw43_arch.h"
#include "pico/time.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/ip_addr.h"
#include "lwip/dns.h"
#include "lwip/altcp.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/pbuf.h"
#include "lwip/err.h"
#include "mbedtls/base64.h"
#include <string.h>
#include <stdlib.h>

// スキャン結果の最大保持件数
#define WIFI_SCAN_MAX_RESULTS 20

// スキャン結果の格納用バッファ
static cyw43_ev_scan_result_t cyw43_wifi_scan_results[WIFI_SCAN_MAX_RESULTS];
static volatile int cyw43_wifi_scan_result_count = 0;

// スキャンコールバック
static int cyw43_wifi_scan_result_callback(void *env, const cyw43_ev_scan_result_t *result)
{
  (void)env;
  if (result && cyw43_wifi_scan_result_count < WIFI_SCAN_MAX_RESULTS) {
    cyw43_wifi_scan_results[cyw43_wifi_scan_result_count] = *result;
    cyw43_wifi_scan_result_count++;
  }
  return 0;
}

/*! @brief mrbc_pico_cyw43_gpio_put(pin, level) CYW43 GPIOピンの出力レベルの設定

  @param pin CYW43 GPIOピン番号
  @param level 出力レベル（0: Low，1: High）
  @return void
*/
static void mrbc_pico_cyw43_gpio_put(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  int level = GET_INT_ARG(2);
  cyw43_arch_gpio_put(pin, level);
}

/*! @brief mrbc_pico_cyw43_gpio_get(pin) CYW43 GPIOピンの入力レベルの取得

  @param pin CYW43 GPIOピン番号
  @return GPIOから読み取られた値（0: Low，1: High）
*/
static void mrbc_pico_cyw43_gpio_get(mrb_vm* vm, mrb_value* v, int argc)
{
  int pin = GET_INT_ARG(1);
  SET_INT_RETURN(cyw43_arch_gpio_get(pin));
}

/*! @brief mrbc_pico_cyw43_enable_sta_mode() STAモードの有効化

  @return void
*/
static void mrbc_pico_cyw43_enable_sta_mode(mrb_vm* vm, mrb_value* v, int argc)
{
  cyw43_arch_enable_sta_mode();
}

/*! @brief mrbc_pico_cyw43_wifi_connect_blocking(ssid, password) WiFiネットワークへの接続

  WPA2認証でWiFiネットワークにブロッキング接続する．接続成功または認証失敗まで無限待機する．

  @param ssid SSID文字列
  @param password パスワード文字列
  @return void
*/
static void mrbc_pico_cyw43_wifi_connect_blocking(mrb_vm* vm, mrb_value* v, int argc)
{
  const char *ssid = (const char*)GET_STRING_ARG(1);
  const char *password = (const char*)GET_STRING_ARG(2);

  cyw43_arch_wifi_connect_blocking(ssid, password, CYW43_AUTH_WPA2_AES_PSK);
}

/*! @brief mrbc_pico_cyw43_wifi_connect_timeout_ms(ssid, password, timeout_ms) タイムアウト付きWiFi接続

  WPA2認証でWiFiネットワークに接続する．
  タイムアウトまで接続できない場合はタイムアウトで復帰する．

  @param ssid SSID文字列
  @param password パスワード文字列
  @param timeout_ms タイムアウト時間（ミリ秒）
  @return void
*/
static void mrbc_pico_cyw43_wifi_connect_timeout_ms(mrb_vm* vm, mrb_value* v, int argc)
{
  const char *ssid = (const char*)GET_STRING_ARG(1);
  const char *password = (const char*)GET_STRING_ARG(2);
  int timeout_ms = GET_INT_ARG(3);

  cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, timeout_ms);
}

/*! @brief mrbc_pico_cyw43_lwip_begin() lwIPロックの取得

  @return void
*/
static void mrbc_pico_cyw43_lwip_begin(mrb_vm* vm, mrb_value* v, int argc)
{
  cyw43_arch_lwip_begin();
}

/*! @brief mrbc_pico_cyw43_lwip_end() lwIPロックの解放

  @return void
*/
static void mrbc_pico_cyw43_lwip_end(mrb_vm* vm, mrb_value* v, int argc)
{
  cyw43_arch_lwip_end();
}

/*! @brief mrbc_pico_cyw43_tcpip_link_status() WiFi接続ステータスの取得

  @return ステータス値（0: 未接続，3: 接続済み，-1: 接続失敗，-2: ネットワーク未検出，-3: 認証失敗）
*/
static void mrbc_pico_cyw43_tcpip_link_status(mrb_vm* vm, mrb_value* v, int argc)
{
  int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
  SET_INT_RETURN(status);
}

/*! @brief mrbc_pico_cyw43_wifi_scan() WiFiスキャンの開始

  非同期スキャンを開始する．
  - 完了確認はmrbc_pico_cyw43_wifi_scan_active()で行う．
  - 読みだされたデータはmrbc_pico_cyw43_wifi_scan_results()で確認する．

  @return SDK戻り値（0: 成功，非0: エラー）
*/
static void mrbc_pico_cyw43_wifi_scan(mrb_vm* vm, mrb_value* v, int argc)
{
  cyw43_wifi_scan_result_count = 0;

  cyw43_wifi_scan_options_t scan_options = {0};
  int err = cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, cyw43_wifi_scan_result_callback);
  SET_INT_RETURN(err);
}

/*! @brief mrbc_pico_cyw43_wifi_scan_active() WiFiスキャン進行中の確認

  @return スキャン進行中かどうか
*/
static void mrbc_pico_cyw43_wifi_scan_active(mrb_vm* vm, mrb_value* v, int argc)
{
  SET_BOOL_RETURN(cyw43_wifi_scan_active(&cyw43_state));
}

/*! @brief mrbc_pico_cyw43_wifi_scan_results() スキャン結果の取得

  すべてのWiFiスキャン結果を返す．

  各要素のインスタンス変数:
  - ssid (String)
  - bssid (6バイトString)
  - channel (Integer)
  - rssi (Integer)
  - auth_mode (Integer)

  @return Array<WLAN::ScanResult>．WLAN::ScanResultクラスが未定義の場合はnil
*/
static void mrbc_pico_cyw43_wifi_scan_results(mrb_vm* vm, mrb_value* v, int argc)
{
  // WLAN::ScanResultクラスを探す
  static mrbc_class *scan_result_cls = NULL;
  if (!scan_result_cls) {
    mrbc_class *wlan = mrbc_get_class_by_name("WLAN");
    if (!wlan) {
      SET_NIL_RETURN();
      return;
    }
    mrbc_value *cval = mrbc_get_class_const(wlan, mrbc_str_to_symid("ScanResult"));
    if (!cval || mrbc_type(*cval) != MRBC_TT_CLASS) {
      SET_NIL_RETURN();
      return;
    }
    scan_result_cls = cval->cls;
  }

  mrbc_value array = mrbc_array_new(vm, cyw43_wifi_scan_result_count);
  for (int i = 0; i < cyw43_wifi_scan_result_count; i++) {
    cyw43_ev_scan_result_t *r = &cyw43_wifi_scan_results[i];
    mrbc_value self = mrbc_instance_new(vm, scan_result_cls, 0);
    mrbc_value val;

    int ssid_len = r->ssid_len > 32 ? 32 : r->ssid_len;
    val = mrbc_string_new(vm, r->ssid, ssid_len);
    mrbc_instance_setiv(&self, mrbc_str_to_symid("ssid"), &val);

    val = mrbc_string_new(vm, r->bssid, 6);
    mrbc_instance_setiv(&self, mrbc_str_to_symid("bssid"), &val);

    val = mrbc_integer_value(r->channel);
    mrbc_instance_setiv(&self, mrbc_str_to_symid("channel"), &val);

    val = mrbc_integer_value(r->rssi);
    mrbc_instance_setiv(&self, mrbc_str_to_symid("rssi"), &val);

    val = mrbc_integer_value(r->auth_mode);
    mrbc_instance_setiv(&self, mrbc_str_to_symid("auth_mode"), &val);

    mrbc_array_set(&array, i, &self);
  }
  SET_RETURN(array);
}

/*! @brief mrbc_pico_cyw43_wifi_get_mac() MACアドレスの取得

  @return MACアドレスのバイト列（6バイト）
*/
static void mrbc_pico_cyw43_wifi_get_mac(mrb_vm* vm, mrb_value* v, int argc)
{
  uint8_t mac[6];
  cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, mac);
  SET_RETURN(mrbc_string_new(vm, mac, 6));
}

/*! @brief mrbc_pico_lwip_ip4_addr() IPアドレスの取得

  lwIPロック内で呼ぶこと．

  @return IPアドレスのバイト列（4バイト），未接続時は全て0
*/
static void mrbc_pico_lwip_ip4_addr(mrb_vm* vm, mrb_value* v, int argc)
{
  if (netif_default == NULL) {
    SET_RETURN(mrbc_string_new(vm, "\0\0\0\0", 4));
    return;
  }
  const ip4_addr_t *addr = netif_ip4_addr(netif_default);
  SET_RETURN(mrbc_string_new(vm, &addr->addr, 4));
}

/*! @brief mrbc_pico_lwip_ip4_netmask() サブネットマスクの取得

  lwIPロック内で呼ぶこと．

  @return サブネットマスクのバイト列（4バイト），未接続時は全て0
*/
static void mrbc_pico_lwip_ip4_netmask(mrb_vm* vm, mrb_value* v, int argc)
{
  if (netif_default == NULL) {
    SET_RETURN(mrbc_string_new(vm, "\0\0\0\0", 4));
    return;
  }
  const ip4_addr_t *addr = netif_ip4_netmask(netif_default);
  SET_RETURN(mrbc_string_new(vm, &addr->addr, 4));
}

/*! @brief mrbc_pico_lwip_ip4_gw() デフォルトゲートウェイの取得

  lwIPロック内で呼ぶこと．

  @return ゲートウェイのバイト列（4バイト），未接続時は全て0
*/
static void mrbc_pico_lwip_ip4_gw(mrb_vm* vm, mrb_value* v, int argc)
{
  if (netif_default == NULL) {
    SET_RETURN(mrbc_string_new(vm, "\0\0\0\0", 4));
    return;
  }
  const ip4_addr_t *addr = netif_ip4_gw(netif_default);
  SET_RETURN(mrbc_string_new(vm, &addr->addr, 4));
}

/*! @brief mrbc_pico_lwip_ip4_dns() DNSサーバアドレスの取得

  lwIPロック内で呼ぶこと．

  @return DNSサーバのバイト列（4バイト），未接続時は全て0
*/
static void mrbc_pico_lwip_ip4_dns(mrb_vm* vm, mrb_value* v, int argc)
{
  if (netif_default == NULL) {
    SET_RETURN(mrbc_string_new(vm, "\0\0\0\0", 4));
    return;
  }
  const ip4_addr_t *addr = ip_2_ip4(dns_getserver(0));
  SET_RETURN(mrbc_string_new(vm, &addr->addr, 4));
}

/*! @brief lwIP DNS解決

  ホスト名のIPv4解決をRubyから利用可能にする．

  グローバル状態で状態を管理するため同時実行できない．
  非同期のため次の順番で呼び出す．
  - mrbc_pico_lwip_dns_start  : 名前解決の開始．
  - mrbc_pico_lwip_dns_done?  : 名前解決が終了したかどうか（成功または失敗で終了）．
  - mrbc_pico_lwip_dns_result : IPアドレス、または失敗時はnilの取得．

  pico-sdkのAPIは下記を参照:
  - https://www.raspberrypi.com/documentation/pico-sdk/networking.html
*/

static volatile bool pico_lwip_dns_done;
static ip_addr_t     pico_lwip_dns_addr;

// DNSコールバック（lwIPバックグラウンドタスクから呼ばれる）
static void pico_lwip_dns_callback(const char *name, const ip_addr_t *addr, void *arg)
{
  (void)name;
  (void)arg;
  if (addr != NULL) {
    pico_lwip_dns_addr = *addr;
  }
  pico_lwip_dns_done = true;
}

/*! @brief mrbc_pico_lwip_dns_start(host) DNS解決の開始

  lwIPロック内で呼ぶこと．

  ホスト名のIPv4解決要求を発行する．
  IPアドレス文字列やキャッシュヒットなどで即時にIPアドレスが得られた場合は内部フラグを完了状態にする．

  @param host ホスト名またはIPアドレス文字列
  @return 要求発行成功でtrue，要求発行失敗でfalse
*/
static void mrbc_pico_lwip_dns_start(mrb_vm* vm, mrb_value* v, int argc)
{
  const char *host = (const char*)GET_STRING_ARG(1);
  pico_lwip_dns_done = false;
  ip_addr_set_zero(&pico_lwip_dns_addr);
  err_t err = dns_gethostbyname(host, &pico_lwip_dns_addr, pico_lwip_dns_callback, NULL);
  if (err == ERR_OK) {
    pico_lwip_dns_done = true;
    SET_TRUE_RETURN();
    return;
  }
  if (err == ERR_INPROGRESS) {
    SET_TRUE_RETURN();
    return;
  }
  SET_FALSE_RETURN();
}

/*! @brief mrbc_pico_lwip_dns_done?() DNS解決の完了確認

  @return 完了済みかどうか
*/
static void mrbc_pico_lwip_dns_done(mrb_vm* vm, mrb_value* v, int argc)
{
  SET_BOOL_RETURN(pico_lwip_dns_done);
}

/*! @brief mrbc_pico_lwip_dns_result() DNS解決結果の取得

  @return IPv4アドレスのバイト列（4バイト），失敗時はnil
*/
static void mrbc_pico_lwip_dns_result(mrb_vm* vm, mrb_value* v, int argc)
{
  uint32_t ip4 = ip_2_ip4(&pico_lwip_dns_addr)->addr;
  if (ip4 == 0) {
    SET_NIL_RETURN();
    return;
  }
  SET_RETURN(mrbc_string_new(vm, &ip4, 4));
}

/*! @brief lwIP altcp TCP/TLS（クライアント／同時1接続）

  HTTP等の上位プロトコルはRubyコードで実装される．
  グローバル状態で状態を管理するため同時実行できない．

  pico-sdkのAPIは下記を参照:
  - https://www.raspberrypi.com/documentation/pico-sdk/networking.html
*/

// 受信バッファサイズ
#define MRBC_PICO_LWIP_TCP_RXBUF_SIZE 2048

// TCPコネクション状態
typedef struct {
  struct altcp_pcb *pcb;        // lwIP altcp PCB（未初期化／エラー時はNULL）
  volatile bool     connected;  // TCP接続、TLSの場合はハンドシェイクの完了
  volatile bool     closed;     // 切断検知（FIN受信またはエラー）
  volatile int      err_code;   // 直近のlwIPエラーコード
  uint8_t           rx_buf[MRBC_PICO_LWIP_TCP_RXBUF_SIZE];
  volatile uint16_t rx_len;     // 受信バッファに溜まっているバイト数
} pico_lwip_tcp_state_t;

static pico_lwip_tcp_state_t pico_lwip_tcp_state;

static struct altcp_tls_config *pico_lwip_tcp_tls_config = NULL;

// 受信コールバック（lwIPバックグラウンドタスクから呼ばれる）
static err_t pico_lwip_tcp_recv_callback(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
  (void)arg;
  if (p == NULL) {
    // 相手側がFINを送ってきた
    pico_lwip_tcp_state.closed = true;
    return ERR_OK;
  }
  if (err == ERR_OK && p->tot_len > 0) {
    uint16_t free_space = MRBC_PICO_LWIP_TCP_RXBUF_SIZE - pico_lwip_tcp_state.rx_len;
    uint16_t copy = (p->tot_len <= free_space) ? p->tot_len : free_space;
    if (copy > 0) {
      pbuf_copy_partial(p, pico_lwip_tcp_state.rx_buf + pico_lwip_tcp_state.rx_len, copy, 0);
      pico_lwip_tcp_state.rx_len += copy;
    }
    // バッファあふれ分も含めACK（あふれ分はドロップ）
    altcp_recved(pcb, p->tot_len);
  }
  pbuf_free(p);
  return ERR_OK;
}

// エラーコールバック（lwIPバックグラウンドタスクから呼ばれる）
// 呼出後はpcbがlwIPに解放されるためpcbをNULLにする
static void pico_lwip_tcp_err_callback(void *arg, err_t err)
{
  (void)arg;
  pico_lwip_tcp_state.pcb = NULL;
  pico_lwip_tcp_state.err_code = err;
  pico_lwip_tcp_state.closed = true;
}

// 接続完了コールバック（lwIPバックグラウンドタスクから呼ばれる）
static err_t pico_lwip_tcp_connected_callback(void *arg, struct altcp_pcb *pcb, err_t err)
{
  (void)arg;
  (void)pcb;
  if (err != ERR_OK) {
    pico_lwip_tcp_state.err_code = err;
    pico_lwip_tcp_state.closed = true;
    return ERR_OK;
  }
  pico_lwip_tcp_state.connected = true;
  return ERR_OK;
}

/*! @brief mrbc_pico_lwip_tcp_open() plain TCP用の接続設定の生成

  TCP PCBを生成して、受信／エラーのコールバックを登録する．
  lwIPロック内で呼ぶこと．

  @return 成功したかどうか
*/
static void mrbc_pico_lwip_tcp_open(mrb_vm* vm, mrb_value* v, int argc)
{
  memset(&pico_lwip_tcp_state, 0, sizeof(pico_lwip_tcp_state));
  pico_lwip_tcp_state.pcb = altcp_tcp_new_ip_type(IPADDR_TYPE_V4);
  if (pico_lwip_tcp_state.pcb == NULL) {
    SET_FALSE_RETURN();
    return;
  }
  altcp_arg(pico_lwip_tcp_state.pcb, NULL);
  altcp_recv(pico_lwip_tcp_state.pcb, pico_lwip_tcp_recv_callback);
  altcp_err(pico_lwip_tcp_state.pcb, pico_lwip_tcp_err_callback);
  SET_TRUE_RETURN();
}

/*! @brief mrbc_pico_lwip_tcp_open_tls() TLS用TCP用の接続設定の生成

  TCP PCBを生成して、受信／エラーのコールバックを登録する．
  lwIPロック内で呼ぶこと．

  @return 成功でtrue，config作成失敗またはPCB作成失敗でfalse
*/
static void mrbc_pico_lwip_tcp_open_tls(mrb_vm* vm, mrb_value* v, int argc)
{
  memset(&pico_lwip_tcp_state, 0, sizeof(pico_lwip_tcp_state));
  if (pico_lwip_tcp_tls_config == NULL) {
    pico_lwip_tcp_tls_config = altcp_tls_create_config_client(NULL, 0);
    if (pico_lwip_tcp_tls_config == NULL) {
      SET_FALSE_RETURN();
      return;
    }
  }
  pico_lwip_tcp_state.pcb = altcp_tls_alloc(pico_lwip_tcp_tls_config, IPADDR_TYPE_V4);
  if (pico_lwip_tcp_state.pcb == NULL) {
    SET_FALSE_RETURN();
    return;
  }
  altcp_arg(pico_lwip_tcp_state.pcb, NULL);
  altcp_recv(pico_lwip_tcp_state.pcb, pico_lwip_tcp_recv_callback);
  altcp_err(pico_lwip_tcp_state.pcb, pico_lwip_tcp_err_callback);
  SET_TRUE_RETURN();
}

/*! @brief mrbc_pico_lwip_tcp_connect(ip4, port) TCP接続要求の発行

  lwIPロック内で呼ぶこと．

  @param ip4 接続先IPv4アドレスのバイト列（4バイト）
  @param port ポート番号
  @return エラー（0:成功，それ以外:エラー）
*/
static void mrbc_pico_lwip_tcp_connect(mrb_vm* vm, mrb_value* v, int argc)
{
  mrb_value ip4_val = GET_ARG(1);
  int port = GET_INT_ARG(2);
  const uint8_t *ip4 = (const uint8_t*)mrbc_string_cstr(&ip4_val);
  int ip4_len = mrbc_string_size(&ip4_val);

  if (pico_lwip_tcp_state.pcb == NULL) {
    SET_INT_RETURN(ERR_ARG);
    return;
  }
  if (ip4_len != 4) {
    SET_INT_RETURN(ERR_ARG);
    return;
  }

  ip_addr_t addr;
  IP_ADDR4(&addr, ip4[0], ip4[1], ip4[2], ip4[3]);
  err_t err = altcp_connect(pico_lwip_tcp_state.pcb, &addr, port, pico_lwip_tcp_connected_callback);
  SET_INT_RETURN(err);
}

/*! @brief mrbc_pico_lwip_tcp_write(data) 送信キューへの追加

  送信キューにデータを追加する．実際の送信はされない．
  lwIPロック内で呼ぶこと．

  @param data 送信データ（String，バイナリ可）
  @return エラー（0:成功，それ以外:エラー）
*/
static void mrbc_pico_lwip_tcp_write(mrb_vm* vm, mrb_value* v, int argc)
{
  mrb_value data_val = GET_ARG(1);
  const uint8_t *data = (const uint8_t*)mrbc_string_cstr(&data_val);
  int len = mrbc_string_size(&data_val);

  if (pico_lwip_tcp_state.pcb == NULL) {
    SET_INT_RETURN(ERR_ARG);
    return;
  }
  err_t err = altcp_write(pico_lwip_tcp_state.pcb, data, len, TCP_WRITE_FLAG_COPY);
  SET_INT_RETURN(err);
}

/*! @brief mrbc_pico_lwip_tcp_output() 送信キューのフラッシュ

  送信キューに溜めたデータを実際に送信する．
  lwIPロック内で呼ぶこと．

  @return エラー（0:成功，それ以外:エラー）
*/
static void mrbc_pico_lwip_tcp_output(mrb_vm* vm, mrb_value* v, int argc)
{
  if (pico_lwip_tcp_state.pcb == NULL) {
    SET_INT_RETURN(ERR_ARG);
    return;
  }
  err_t err = altcp_output(pico_lwip_tcp_state.pcb);
  SET_INT_RETURN(err);
}

/*! @brief mrbc_pico_lwip_tcp_close() 接続のクローズ

  コールバックの解除および接続の切断．
  lwIPロック内で呼ぶこと．

  @return エラー（0:成功，それ以外:エラー）
*/
static void mrbc_pico_lwip_tcp_close(mrb_vm* vm, mrb_value* v, int argc)
{
  if (pico_lwip_tcp_state.pcb != NULL) {
    altcp_arg(pico_lwip_tcp_state.pcb, NULL);
    altcp_recv(pico_lwip_tcp_state.pcb, NULL);
    altcp_sent(pico_lwip_tcp_state.pcb, NULL);
    altcp_err(pico_lwip_tcp_state.pcb, NULL);
    altcp_poll(pico_lwip_tcp_state.pcb, NULL, 0);
    if (altcp_close(pico_lwip_tcp_state.pcb) != ERR_OK) {
      altcp_abort(pico_lwip_tcp_state.pcb);
    }
    pico_lwip_tcp_state.pcb = NULL;
  }
  SET_INT_RETURN(ERR_OK);
}

/*! @brief mrbc_pico_lwip_tcp_connected?() 接続完了の確認

  @return 接続完了済みかどうか
*/
static void mrbc_pico_lwip_tcp_connected(mrb_vm* vm, mrb_value* v, int argc)
{
  SET_BOOL_RETURN(pico_lwip_tcp_state.connected);
}

/*! @brief mrbc_pico_lwip_tcp_closed?() 切断検知の確認

  FIN受信またはエラーによる切断が検知されたかどうかを返す．

  @return 切断検知済みかどうか
*/
static void mrbc_pico_lwip_tcp_closed(mrb_vm* vm, mrb_value* v, int argc)
{
  SET_BOOL_RETURN(pico_lwip_tcp_state.closed);
}

/*! @brief mrbc_pico_lwip_tcp_err_code() 直近エラーコードの取得

  @return エラー（0:成功，それ以外:エラー）
*/
static void mrbc_pico_lwip_tcp_err_code(mrb_vm* vm, mrb_value* v, int argc)
{
  SET_INT_RETURN(pico_lwip_tcp_state.err_code);
}

/*! @brief mrbc_pico_lwip_tcp_rx_size() 受信バッファ滞留量の取得

  @return 受信バッファに溜まっているバイト数
*/
static void mrbc_pico_lwip_tcp_rx_size(mrb_vm* vm, mrb_value* v, int argc)
{
  SET_INT_RETURN(pico_lwip_tcp_state.rx_len);
}

/*! @brief mrbc_pico_lwip_tcp_recv_pop() 受信バッファの取り出し

  受信バッファに溜まっているデータを返し，バッファをクリアする．
  lwIPロック内で呼ぶこと．

  @return 受信バイト列（文字列）．空の場合は空文字列
*/
static void mrbc_pico_lwip_tcp_recv_pop(mrb_vm* vm, mrb_value* v, int argc)
{
  uint16_t len = pico_lwip_tcp_state.rx_len;
  mrbc_value ret;
  if (len == 0) {
    ret = mrbc_string_new(vm, "", 0);
  } else {
    ret = mrbc_string_new(vm, pico_lwip_tcp_state.rx_buf, len);
  }
  pico_lwip_tcp_state.rx_len = 0;
  SET_RETURN(ret);
}

/*! @brief mrbc_pico_mbedtls_base64_encode(src) Base64エンコード

  HTTP Basic認証ヘッダの組み立て等で使用される想定．

  @param src 入力バイト列（文字列）
  @return Base64エンコード後の文字列．失敗時は空文字列
*/
static void mrbc_pico_mbedtls_base64_encode(mrb_vm* vm, mrb_value* v, int argc)
{
  mrb_value src_val = GET_ARG(1);
  const unsigned char *src = (const unsigned char*)mrbc_string_cstr(&src_val);
  size_t slen = mrbc_string_size(&src_val);

  // 必要バッファサイズの算出（dst=NULL, dlen=0 で olen に必要長が入る．終端のヌル文字を含む）
  size_t needed = 0;
  mbedtls_base64_encode(NULL, 0, &needed, src, slen);
  if (needed <= 1) {
    SET_RETURN(mrbc_string_new(vm, "", 0));
    return;
  }

  unsigned char *buf = (unsigned char*)malloc(needed);
  if (buf == NULL) {
    SET_RETURN(mrbc_string_new(vm, "", 0));
    return;
  }
  size_t olen = 0;
  int err = mbedtls_base64_encode(buf, needed, &olen, src, slen);
  if (err != 0) {
    free(buf);
    SET_RETURN(mrbc_string_new(vm, "", 0));
    return;
  }
  mrbc_value ret = mrbc_string_new(vm, buf, olen);
  free(buf);
  SET_RETURN(ret);
}

/** @brief C関数のRubyへの公開

  @param vm mruby/c VM
*/
void mrbc_pico_cyw43_gem_init(struct VM* vm)
{
  // CYW43ドライバの初期化
  if (cyw43_arch_init()) {
    panic("cyw43_arch_init() failed");
  }

  // CYW43 GPIO
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_cyw43_gpio_put", mrbc_pico_cyw43_gpio_put);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_cyw43_gpio_get", mrbc_pico_cyw43_gpio_get);

  // CYW43 WiFi
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_cyw43_enable_sta_mode", mrbc_pico_cyw43_enable_sta_mode);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_cyw43_wifi_connect_blocking", mrbc_pico_cyw43_wifi_connect_blocking);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_cyw43_wifi_connect_timeout_ms", mrbc_pico_cyw43_wifi_connect_timeout_ms);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_cyw43_lwip_begin", mrbc_pico_cyw43_lwip_begin);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_cyw43_lwip_end", mrbc_pico_cyw43_lwip_end);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_cyw43_tcpip_link_status", mrbc_pico_cyw43_tcpip_link_status);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_cyw43_wifi_scan", mrbc_pico_cyw43_wifi_scan);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_cyw43_wifi_scan_active", mrbc_pico_cyw43_wifi_scan_active);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_cyw43_wifi_scan_results", mrbc_pico_cyw43_wifi_scan_results);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_cyw43_wifi_get_mac", mrbc_pico_cyw43_wifi_get_mac);

  // lwIP
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_lwip_ip4_addr", mrbc_pico_lwip_ip4_addr);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_lwip_ip4_netmask", mrbc_pico_lwip_ip4_netmask);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_lwip_ip4_gw", mrbc_pico_lwip_ip4_gw);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_lwip_ip4_dns", mrbc_pico_lwip_ip4_dns);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_lwip_dns_start", mrbc_pico_lwip_dns_start);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_lwip_dns_done?", mrbc_pico_lwip_dns_done);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_lwip_dns_result", mrbc_pico_lwip_dns_result);

  // lwIP TCP
  pico_lwip_tcp_state.pcb = NULL;
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_lwip_tcp_open", mrbc_pico_lwip_tcp_open);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_lwip_tcp_open_tls", mrbc_pico_lwip_tcp_open_tls);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_lwip_tcp_connect", mrbc_pico_lwip_tcp_connect);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_lwip_tcp_write", mrbc_pico_lwip_tcp_write);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_lwip_tcp_output", mrbc_pico_lwip_tcp_output);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_lwip_tcp_close", mrbc_pico_lwip_tcp_close);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_lwip_tcp_connected?", mrbc_pico_lwip_tcp_connected);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_lwip_tcp_closed?", mrbc_pico_lwip_tcp_closed);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_lwip_tcp_err_code", mrbc_pico_lwip_tcp_err_code);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_lwip_tcp_rx_size", mrbc_pico_lwip_tcp_rx_size);
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_lwip_tcp_recv_pop", mrbc_pico_lwip_tcp_recv_pop);

  // mbedtls
  mrbc_define_method(0, mrbc_class_object, "mrbc_pico_mbedtls_base64_encode", mrbc_pico_mbedtls_base64_encode);
}
