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
 
//*--- includes

#include "fs.h"
#include <stdio.h>
#include <string.h>
#include "ff.h"
#include "flash_bd.h"
#include "jsmn.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>

//*--- Global areas

static FATFS g_fs;
static bool  g_mounted = false;
static FRESULT g_fr_mount = FR_OK;
static FRESULT g_fr_mkfs  = FR_OK;
static FRESULT g_fr_open  = FR_OK;
static FRESULT g_fr_read  = FR_OK;

//*--- Prototypes

int fs_last_fr_mount(void){ return (int)g_fr_mount; }
int fs_last_fr_mkfs(void) { return (int)g_fr_mkfs; }
int fs_last_fr_open(void) { return (int)g_fr_open; }
int fs_last_fr_read(void) { return (int)g_fr_read; }
static const char* skip_ws(const char* p);
static bool copy_json_string(const char* p, char* out, size_t out_max, size_t* adv);
static bool copy_json_primitive(const char* p, char* out, size_t out_max);
bool fs_get_kv(const char* json, const char* key, char* out, size_t out_max);

//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//*                         Utility functions
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=

//*--- Trim a string from right

static void rtrim(char* s) {
  size_t n = strlen(s);
  while (n && isspace((unsigned char)s[n-1])) {
    s[--n] = 0;
  }
}

//*--- Trim a string from left

static char* ltrim(char* s) {
  while (*s && isspace((unsigned char)*s)) s++;
  return s;
}


//*--- Helpers to parse and process a JSON file

static const char* skip_ws(const char* p) {
  while (*p && isspace((unsigned char)*p)) p++;
  return p;
}

static bool copy_json_string(const char* p, char* out, size_t out_max, size_t* adv) {
  // p apunta al caracter '"'
  if (*p != '"') return false;
  p++; // salta la primera comilla

  size_t w = 0;
  while (*p) {
    if (*p == '"') { // fin del string
      out[w] = '\0';
      if (adv) *adv = (size_t)((p + 1) - (p - (w ? 0 : 0))); // no usar esta cuenta
      return true;
    }

    if (*p == '\\') { // escape simple
      p++;
      if (!*p) break;

      char c = *p;
      // soportamos escapes típicos mínimos
      if (c == '"' || c == '\\' || c == '/') {
        if (w + 1 < out_max) out[w++] = c;
      } else if (c == 'n') {
        if (w + 1 < out_max) out[w++] = '\n';
      } else if (c == 'r') {
        if (w + 1 < out_max) out[w++] = '\r';
      } else if (c == 't') {
        if (w + 1 < out_max) out[w++] = '\t';
      } else {
        // escape no soportado: lo copiamos tal cual
        if (w + 1 < out_max) out[w++] = c;
      }
      p++;
      continue;
    }

    if (w + 1 < out_max) out[w++] = *p;
    p++;
  }

  return false;
}

static bool copy_json_primitive(const char* p, char* out, size_t out_max) {
  // copia hasta , o } o whitespace
  size_t w = 0;
  while (*p && *p != ',' && *p != '}' && !isspace((unsigned char)*p)) {
    if (w + 1 < out_max) out[w++] = *p;
    p++;
  }
  out[w] = '\0';
  return w > 0;
}

//*--- Search for a key within the JSON file and returns the associated value
//*--- BEWARE: this is not a full implementation of the JSON structure and
//*--- it does not support nesting

bool fs_get_kv(const char* json, const char* key, char* out, size_t out_max) {
  if (!json || !key || !out || out_max < 2) return false;

  //*--- get the key

  char pat[96];
  size_t klen = strlen(key);
  if (klen + 2 >= sizeof(pat)) return false;

  pat[0] = '"';
  memcpy(&pat[1], key, klen);
  pat[1 + klen] = '"';
  pat[2 + klen] = '\0';

  const char* p = json;

  while ((p = strstr(p, pat)) != NULL) {
    p += (2 + klen);        //* After key
    p = skip_ws(p);
    if (*p != ':') continue;
    p++;                    //* Skip : separator
    p = skip_ws(p);

    if (*p == '"') {
      // string
      //*--- Copy string without quotes
      size_t w = 0;
      p++;  //*--- iterate within the string
      while (*p) {
        if (*p == '"') {
          out[w] = '\0';
          return true;
        }
        if (*p == '\\') {
          p++;
          if (!*p) break;
          char c = *p;
          if (c == '"' || c == '\\' || c == '/') {
            if (w + 1 < out_max) out[w++] = c;
          } else if (c == 'n') {
            if (w + 1 < out_max) out[w++] = '\n';
          } else if (c == 'r') {
            if (w + 1 < out_max) out[w++] = '\r';
          } else if (c == 't') {
            if (w + 1 < out_max) out[w++] = '\t';
          } else {
            if (w + 1 < out_max) out[w++] = c;
          }
          p++;
          continue;
        }
        if (w + 1 < out_max) out[w++] = *p;
        p++;
      }
      return false;
    } else {
      return copy_json_primitive(p, out, out_max);
    }
  }
  return false;
}

//*--- When the filesystem is empty this create a file with the default values
//*--- Random data on this exercise

bool fs_ensure_cfg_exists(void) {
  if (!g_mounted) return false;

  FIL fp;
  FRESULT fr = f_open(&fp, ADX_CFG_FILENAME, FA_READ);
  if (fr == FR_OK) { f_close(&fp); return true; }
  if (fr != FR_NO_FILE) return false;

  static const char default_txt[] =
    "region=ar\n"
    "vco_hz=1172000000\n"
    "bfo_hz=465000\n";

  fr = f_open(&fp, ADX_CFG_FILENAME, FA_CREATE_ALWAYS | FA_WRITE);
  if (fr != FR_OK) return false;

  UINT bw = 0;
  fr = f_write(&fp, default_txt, (UINT)strlen(default_txt), &bw);
  if (fr != FR_OK) { f_close(&fp); return false; }

  fr = f_sync(&fp);
  f_close(&fp);

  return (fr == FR_OK) && (bw == (UINT)strlen(default_txt));
}

//*--- Initial mount

bool fs_init_and_mount(void) {
  if (!flash_bd_init()) return false;

  g_fr_mount = f_mount(&g_fs, "0:", 1);

  if (g_fr_mount != FR_OK) {

    //*--- Only format if there is no available filesystem
    if (g_fr_mount != FR_NO_FILESYSTEM) {
      return false;
    }

    flash_bd_erase_all();

    BYTE work[4096];
    MKFS_PARM opt = {
      .fmt    = FM_FAT,   // FAT12/16 (evita FAT32)
      .n_fat  = 1,
      .align  = 0,
      .n_root = 512,
      .au_size= 0
    };

    g_fr_mkfs = f_mkfs("0:", &opt, work, sizeof(work));
    if (g_fr_mkfs != FR_OK) return false;

    g_fr_mount = f_mount(&g_fs, "0:", 1);
    if (g_fr_mount != FR_OK) return false;
  }

  g_mounted = true;

  if (!fs_ensure_cfg_exists()) return false;
  return true;
}

//*--- Unmount when finished, from this point on the FS won't be accessible

void fs_unmount(void) {
  f_mount(NULL, "", 1);
  g_mounted = false;
}

//*--- Read and write
bool fs_read_text(char* out, size_t out_max, size_t* out_len) {
  if (!g_mounted || !out || out_max < 2) return false;

  FIL fp;
  FRESULT fr = f_open(&fp, ADX_CFG_FILENAME, FA_READ);
  g_fr_open = fr;
  if (fr != FR_OK) return false;

  UINT br = 0;
  fr = f_read(&fp, out, (UINT)(out_max - 1), &br);
  g_fr_read = fr;
  f_close(&fp);

  if (fr != FR_OK) return false;
  out[br] = '\0';
  if (out_len) *out_len = (size_t)br;
  return true;
}

bool fs_write_text(const char* text, size_t len) {
  if (!g_mounted || !text) return false;

  FIL fp;
  FRESULT fr = f_open(&fp, ADX_CFG_FILENAME, FA_CREATE_ALWAYS | FA_WRITE);
  if (fr != FR_OK) return false;

  UINT bw = 0;
  fr = f_write(&fp, text, (UINT)len, &bw);
  if (fr != FR_OK) { f_close(&fp); return false; }

  fr = f_sync(&fp);   // <- clave
  f_close(&fp);

  return (fr == FR_OK) && (bw == (UINT)len);
}

//*--- JSON advanced parsing

static bool token_streq(const char* js, const jsmntok_t* t, const char* s) {
  int slen = (int)strlen(s);
  int tlen = t->end - t->start;
  return (t->type == JSMN_STRING) && (tlen == slen) && (memcmp(js + t->start, s, (size_t)slen) == 0);
}

bool fs_json_get(const char* key, char* out, size_t out_max) {
  if (!key || !out || out_max == 0) return false;

  char json[2048];
  size_t n = 0;
  if (!fs_read_text(json, sizeof(json), &n)) return false;

  jsmn_parser p;
  jsmn_init(&p);
  jsmntok_t tok[128];

  int rc = jsmn_parse(&p, json, (unsigned int)n, tok, 128);
  if (rc < 1 || tok[0].type != JSMN_OBJECT) return false;

  for (int i = 1; i < rc - 1; i++) {
    if (token_streq(json, &tok[i], key)) {
      jsmntok_t* v = &tok[i + 1];
      int vlen = v->end - v->start;
      if ((size_t)vlen >= out_max) vlen = (int)out_max - 1;
      memcpy(out, json + v->start, (size_t)vlen);
      out[vlen] = '\0';
      return true;
    }
  }
  return false;
}

static bool json_set_impl(const char* key, const char* value_raw, bool quoted) {
  char json[2048];
  size_t n = 0;

  if (!fs_read_text(json, sizeof(json), &n)) {
    n = 0;
    json[0] = '\0';
  }

  jsmn_parser p;
  jsmn_init(&p);
  jsmntok_t tok[128];
  int rc = jsmn_parse(&p, json, (unsigned int)n, tok, 128);

  if (rc < 1 || tok[0].type != JSMN_OBJECT) {
    char tmp[2048];
    if (quoted) snprintf(tmp, sizeof(tmp), "{ \"%s\": \"%s\" }\n", key, value_raw);
    else        snprintf(tmp, sizeof(tmp), "{ \"%s\": %s }\n", key, value_raw);
    return fs_write_text(tmp, strlen(tmp));
  }

  int key_tok = -1, val_tok = -1;
  for (int i = 1; i < rc - 1; i++) {
    if (token_streq(json, &tok[i], key)) { key_tok = i; val_tok = i + 1; break; }
  }

  char out[2048];
  memset(out, 0, sizeof(out));

  if (key_tok >= 0) {
    int pre = tok[val_tok].start;
    int post = tok[val_tok].end;

    int w = 0;
    memcpy(out, json, (size_t)pre); w += pre;

    if (quoted) w += snprintf(out + w, sizeof(out) - (size_t)w, "\"%s\"", value_raw);
    else        w += snprintf(out + w, sizeof(out) - (size_t)w, "%s", value_raw);

    memcpy(out + w, json + post, n - (size_t)post);
    w += (int)(n - (size_t)post);

    return fs_write_text(out, (size_t)w);
  }

  int end = (int)n - 1;
  while (end > 0 && json[end] != '}') end--;
  if (end <= 0) return false;

  int w = 0;
  memcpy(out, json, (size_t)end); w += end;

  if (tok[0].size > 0) {
    int j = end - 1;
    while (j > 0 && (json[j] == ' ' || json[j] == '\n' || json[j] == '\r' || json[j] == '\t')) j--;
    if (json[j] != '{' && json[j] != ',') out[w++] = ',';
  }

  w += snprintf(out + w, sizeof(out) - (size_t)w,
                "\n  \"%s\": %s%s%s\n",
                key,
                quoted ? "\"" : "",
                value_raw,
                quoted ? "\"" : "");

  out[w++] = '}';
  out[w++] = '\n';

  return fs_write_text(out, (size_t)w);
}

bool fs_json_set_string(const char* key, const char* value) {
  return (key && value) ? json_set_impl(key, value, true) : false;
}

bool fs_json_set_raw(const char* key, const char* value_raw, bool quoted) {
  return (key && value_raw) ? json_set_impl(key, value_raw, quoted) : false;
}

bool fs_init(void) {
  return fs_init_and_mount();
}

bool fs_json_set(const char* key, const char* value) {
  return fs_json_set_string(key, value);
}

bool fs_json_save(void) {
  // ya persistimos con fs_write_text() en fs_json_set_*
  return true;
}
