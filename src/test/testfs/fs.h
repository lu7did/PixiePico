/*
 * =======================================================================================
 * fs
 * (c) Dr. Pedro E. Colla (LU7DZ) <pedro.colla@gmail.com>
 * 
 * Implementation of a rp2040 based USB MSC controller 
 * =======================================================================================
 * This is mainly an integration effort, the code in this library has been developed 
 * from scratch for this project.
 *----------------------------------------------------------------------------
 * Version 1.0
 * - Initial release
 *----------------------------------------------------------------------------*/
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ADX_CFG_FILENAME  "CONFIG.TXT"

//*--- API

bool fs_init_and_mount(void);
void fs_unmount(void);
bool fs_ensure_cfg_exists(void);
bool fs_read_text(char* out, size_t out_max, size_t* out_len);
bool fs_write_text(const char* text, size_t len);
bool fs_json_get(const char* key, char* out, size_t out_max);
bool fs_json_set(const char* key, const char* value);
bool fs_json_save(void);
bool fs_get_kv(const char* json, const char* key, char* out, size_t out_max);

//*--- Memory areas (DEBUG)
int  fs_last_fr_mount(void);
int  fs_last_fr_mkfs(void);
int  fs_last_fr_open(void);
int  fs_last_fr_read(void);

#ifdef __cplusplus
}
#endif
