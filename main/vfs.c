/*! @file
  @brief Raspberry Pi Pico向け仮想ファイルシステム（VFS）の実装

  LittleFSライブラリを利用してフラッシュメモリ上にファイルシステムを構築し，
  ファイルの読み書き，削除，CRC8計算などの基本的なファイル操作機能を提供する．
*/
#include "vfs.h"
#include "lfs.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <string.h>
#include <stdlib.h>

// フラッシュストレージオフセット（プログラムコード後から開始）
#define FLASH_TARGET_OFFSET (768 * 1024) // 768KBオフセット

// ファイルシステムで使用する変数
static lfs_t lfs;
static lfs_file_t file;

/** @brief フラッシュメモリから指定ブロックのデータの読み込み

  @param cfg ファイルシステム設定構造体のポインタ
  @param block 読み込み対象のブロック番号
  @param off ブロック内のオフセット位置
  @param buffer 読み込みデータを格納するバッファのポインタ
  @param size 読み込みサイズ（バイト単位）
  @return 成功時は0を返す
*/
static int pico_lfs_read(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    uint32_t flash_addr = FLASH_TARGET_OFFSET + (block * cfg->block_size) + off;
    const uint8_t *flash_ptr = (const uint8_t *)(XIP_BASE + flash_addr);
    memcpy(buffer, flash_ptr, size);
    return 0;
}

/** @brief フラッシュメモリの指定ブロックへのデータの書き込み

  @param cfg ファイルシステム設定構造体のポインタ
  @param block 書き込み対象のブロック番号
  @param off ブロック内のオフセット位置
  @param buffer 書き込むデータが格納されたバッファのポインタ
  @param size 書き込みサイズ（バイト単位）
  @return 成功時は0を返す
*/
static int pico_lfs_prog(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    uint32_t flash_addr = FLASH_TARGET_OFFSET + (block * cfg->block_size) + off;
    uint32_t ints = save_and_disable_interrupts();
    flash_range_program(flash_addr, (const uint8_t *)buffer, size);
    restore_interrupts(ints);
    return 0;
}

/** @brief フラッシュメモリの指定ブロックの消去

  @param cfg ファイルシステム設定構造体のポインタ
  @param block 消去対象のブロック番号
  @return 成功時は0を返す
*/
static int pico_lfs_erase(const struct lfs_config *cfg, lfs_block_t block) {
    uint32_t flash_addr = FLASH_TARGET_OFFSET + (block * cfg->block_size);
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(flash_addr, cfg->block_size);
    restore_interrupts(ints);
    return 0;
}

/** @brief ファイルシステムの同期処理．書き込み完了を待つ

  @param cfg ファイルシステム設定構造体のポインタ
  @return 成功時は0を返す
*/
static int pico_lfs_sync(const struct lfs_config *cfg) {
    return 0;
}

// ファイルシステムの設定構造体
static const struct lfs_config cfg = {
  // ブロックデバイス操作関数
  .read  = pico_lfs_read,
  .prog  = pico_lfs_prog,
  .erase = pico_lfs_erase,
  .sync  = pico_lfs_sync,

  // ブロックデバイス設定
  .read_size = 1, // 読み取り操作の最小単位（cache_sizeとblock_sizeの約数）
  .prog_size = FLASH_PAGE_SIZE, // 書き込み操作の最小単位（cache_sizeとblock_sizeの約数）
  .block_size = FLASH_SECTOR_SIZE, // 消去可能なブロックのサイズ（=cache_size、read_sizeとprog_sizeの倍数）
  .block_count = 512 * 1024 / FLASH_SECTOR_SIZE, // デバイス上の総ブロック数（ファイルシステムサイズ[512KiB]/block_size）
  .cache_size = FLASH_SECTOR_SIZE, // キャッシュバッファのサイズ（=block_size）
  .lookahead_size = 16, // 空きブロック探索用ルックアヘッドバッファのサイズ
  .block_cycles = 500 // ウェアレベリングの均一性（100-1000推奨）
};

/** @brief ファイルシステムのマウント

  LittleFSを使用してファイルシステムをマウントする．
  未フォーマットの場合はフォーマットを行う．

  @return 成功時は0，失敗時は負の値を返す
*/
int vfs_mount() {
  int ret = lfs_mount(&lfs, &cfg);
  if (ret < 0) {
    lfs_format(&lfs, &cfg);
    return lfs_mount(&lfs, &cfg);
  }
  return ret;
}

/** @brief ファイルへのデータ書き込み

  指定されたファイルにバッファのデータを書き込む．
  ファイルが存在しない場合は新規作成される．

  @param filename 書き込み対象のファイル名
  @param buffer 書き込むデータが格納されたバッファのポインタ
  @param size 書き込みサイズ（バイト単位）
  @return 成功時は書き込んだバイト数，失敗時は負の値を返す
*/
int vfs_write(const char* filename, const uint8_t* buffer, uint32_t size) {
  int ret = lfs_file_open(&lfs, &file, filename, LFS_O_WRONLY | LFS_O_CREAT);
  if (ret < 0) {
    return ret;
  }
  ret = lfs_file_write(&lfs, &file, buffer, size);
  if (ret < 0) {
    lfs_file_close(&lfs, &file);
    return ret;
  }

  int ret_close = lfs_file_close(&lfs, &file);
  return ret_close < 0 ? ret_close : ret;
}

/** @brief ファイルからのデータ読み込み

  指定されたファイルからデータを読み込み，バッファに格納する．
  ファイルが存在しない場合はエラーを返す．

  @param filename 読み込み対象のファイル名
  @param buffer 読み込みデータを格納するバッファのポインタ
  @param size 読み込みサイズ（バイト単位）
  @return 成功時は読み込んだバイト数，失敗時は負の値を返す
*/
int vfs_read(const char* filename, uint8_t* buffer, uint32_t size) {
  // ファイルを読み込みモードでオープン
  int ret = lfs_file_open(&lfs, &file, filename, LFS_O_RDONLY);
  if (ret < 0) {
    return ret;
  }

  // ファイルからデータを読み込み
  ret = lfs_file_read(&lfs, &file, buffer, size);
  if (ret < 0) {
    lfs_file_close(&lfs, &file);
    return ret;
  }

  // ファイルをクローズして結果を返す
  int ret_close = lfs_file_close(&lfs, &file);
  return ret_close < 0 ? ret_close : ret;
}

/** @brief ファイルサイズの取得

  指定されたファイルのサイズ情報を取得する．
  ファイルが存在しない場合はエラーを返す．

  @param filename 対象ファイル名
  @param size ファイルサイズを格納する変数のポインタ（NULLの場合は無視される）
  @return 成功時は0，失敗時は負の値を返す
*/
int vfs_stat_size(const char* filename, uint32_t* size) {
  struct lfs_info info;

  // ファイルの情報を取得
  int ret = lfs_stat(&lfs, filename, &info);
  if (ret == 0) {
    // 成功時はファイルサイズを設定
    *size = info.size;
  }
  return ret;
}

/** @brief ファイルの削除

  指定されたファイルをファイルシステムから削除する．
  ファイルが存在しない場合もエラーを返す．

  @param filename 削除対象のファイル名
  @return 成功時は0，失敗時は負の値を返す
*/
int vfs_remove(const char* filename) {
  return lfs_remove(&lfs, filename);
}

/** @brief ファイルシステムのマウント解除

  マウントされているファイルシステムを安全にアンマウントする．
  未保存のデータがある場合は自動的に保存される．

  @return 成功時は0，失敗時は負の値を返す
*/
int vfs_unmount() {
  return lfs_unmount(&lfs);
}

/** @brief ファイルのCRC8チェックサム計算

  指定されたファイルの内容からCRC8チェックサムを計算する．
  生成多項式は0x31を使用し，初期値は0xFFとする．

  @param filename 対象ファイル名
  @param _crc CRC8値を格納する変数のポインタ（NULLの場合は無視される）
  @return 成功時はCRC8値，失敗時は負の値を返す
*/
int vfs_crc8(const char* filename, uint8_t* _crc) {
  // CRC8初期値と生成多項式の設定
  uint8_t crc = 0xFF;
  const uint8_t poly = 0x31;

  // ファイルの内容を動的メモリで読み込み
  uint8_t *buffer = NULL;
  uint32_t buffer_size = 0;
  if (vfs_stat_size(filename, &buffer_size) < 0 || buffer_size == 0) {
    return -1;
  }
  buffer = calloc(buffer_size, sizeof(uint8_t));
  if (buffer == NULL || vfs_read(filename, buffer, buffer_size) < 0) {
    if (buffer != NULL) free(buffer);
    return -1;
  }

  // CRC8をバイトごとに計算
  for (size_t i = 0; i < buffer_size; i++) {
    crc ^= buffer[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ poly;
      } else {
        crc <<= 1;
      }
    }
  }

  // 使用したメモリを解放
  free(buffer);

  // CRC8値を返却用変数に設定
  if (_crc != NULL) {
    *_crc = crc;
  }
  return (int)crc;
}
