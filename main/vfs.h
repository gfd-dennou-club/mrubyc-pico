#ifndef VFS_H
#define VFS_H

#include <stdint.h>

/*
* ファイルシステムをマウントする。マウントに失敗した場合、フォーマットを試みる。
*
* @return 成功時は0、失敗時は負の値を返却する。
*/
int vfs_mount(void);

/*
* 指定されたファイルにデータを書き込む。
*
* @param filename 書き込み対象のファイル名。
* @param buffer 書き込むデータが格納されたバッファのポインタ。
* @param size 書き込みサイズ（バイト単位）。
* @return 成功時は書き込んだバイト数、失敗時は負の値を返却する。
*/
int vfs_write(const char* filename, const uint8_t* buffer, uint32_t size);

/*
* 指定されたファイルからデータを読み込む。
*
* @param filename 読み込み対象のファイル名。
* @param buffer 読み込みデータを格納するバッファのポインタ。
* @param size 読み込みサイズ（バイト単位）
* @return 成功時は読み込んだバイト数、失敗時は負の値を返却する。
*/
int vfs_read(const char* filename, uint8_t* buffer, uint32_t size);

/*
* 指定されたファイルのサイズを取得する。
*
* @param filename 対象ファイル名。
* @param size ファイルサイズを格納する変数のポインタ（NULLの場合は無視される）。
* @return 成功時は0、失敗時は負の値を返す
*/
int vfs_stat_size(const char* filename, uint32_t* size);

/*
* 指定されたファイルを削除する。
*
* @param filename 削除対象のファイル名。
* @return 成功時は0、失敗時は負の値を返却する。
*/
int vfs_remove(const char* filename);

/*
* ファイルシステムをアンマウントする。
*
* @return 成功時は0、失敗時は負の値を返却する。
*/
int vfs_unmount(void);


/*
* 指定されたファイルのCRC8チェックサムを計算する。
*
* @param filename 対象ファイル名。
* @param _crc CRC8値を格納する変数のポインタ（NULLの場合は無視される）。
* @return 成功時はCRC8値、失敗時は負の値を返却する。
*/
int vfs_crc8(const char* filename, uint8_t* _crc);

#endif // VFS_H
