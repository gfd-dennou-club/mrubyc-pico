/*! @file
  @brief Raspberry Pi Pico向け仮想ファイルシステム（VFS）の宣言
*/
#ifndef VFS_H
#define VFS_H

#include <stdint.h>

// vfs_mount() ファイルシステムのマウント
int vfs_mount(void);

// vfs_write(filename, buffer, size) ファイルへのデータ書き込み
int vfs_write(const char* filename, const uint8_t* buffer, uint32_t size);

// vfs_read(filename, buffer, size) ファイルからのデータ読み込み
int vfs_read(const char* filename, uint8_t* buffer, uint32_t size);

// vfs_stat_size(filename, size) ファイルサイズの取得
int vfs_stat_size(const char* filename, uint32_t* size);

// vfs_remove(filename) ファイルの削除
int vfs_remove(const char* filename);

// vfs_unmount() ファイルシステムのアンマウント
int vfs_unmount(void);

// vfs_crc8(filename, _crc) ファイルのCRC8チェックサム計算
int vfs_crc8(const char* filename, uint8_t* _crc);

#endif // VFS_H
