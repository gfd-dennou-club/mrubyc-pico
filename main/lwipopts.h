/*! @file
  @brief lwIPスタックの設定

  pico_cyw43_arch_lwip_threadsafe_background使用時に必須の設定ファイル．
  下記pico-examplesの共通設定を参考に最小限の構成で作成．

  - https://github.com/raspberrypi/pico-examples/blob/4c3a3dc0196dd426fddd709616d0da984e027bab/pico_w/wifi/lwipopts_examples_common.h
*/

#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// OS不使用
#define NO_SYS                      1

// ソケットAPIは不使用
#define LWIP_SOCKET                 0

// メモリ設定
#define MEM_LIBC_MALLOC             0
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    4000

// バッファプール設定
#define MEMP_NUM_TCP_PCB            5
#define MEMP_NUM_TCP_SEG            32
#define MEMP_NUM_ARP_QUEUE          10
#define PBUF_POOL_SIZE              24

// プロトコル有効化
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define LWIP_IPV4                   1
#define LWIP_TCP                    1
#define LWIP_UDP                    1
#define LWIP_DHCP                   1
#define LWIP_DNS                    1

// TCP設定
#define TCP_MSS                     1460
#define TCP_WND                     (8 * TCP_MSS)
#define TCP_SND_BUF                 (8 * TCP_MSS)
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define LWIP_TCP_KEEPALIVE          1

// ネットワークインターフェース
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_NETIF_TX_SINGLE_PBUF   1
#define LWIP_NETCONN                0

// DHCP設定（アドレス競合検出の無効化）
#define LWIP_DHCP_DOES_ACD_CHECK    0

// チェックサム
#define LWIP_CHKSUM_ALGORITHM       3

// TLS設定
#define LWIP_ALTCP                  1
#define LWIP_ALTCP_TLS              1
#define LWIP_ALTCP_TLS_MBEDTLS      1
#define MEMP_NUM_ALTCP_PCB          MEMP_NUM_TCP_PCB

// デバッグビルド時のみ統計・ログ有効化
#ifndef NDEBUG
#define LWIP_DEBUG                  1
#define LWIP_STATS                  1
#define LWIP_STATS_DISPLAY          1
#endif

#endif /* _LWIPOPTS_H */
