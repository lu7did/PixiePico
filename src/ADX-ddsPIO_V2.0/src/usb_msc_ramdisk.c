// usb_msc_ramdisk.c  (versión depurada: RAM cache + commit a flash solo al EJECT)

#include "tusb.h"
#include <string.h>
#include <stdbool.h>

#include "hardware/flash.h"
#include "hardware/sync.h"

#include "fat12_64k.h"   // unsigned char fat12_64k_img[]; unsigned int fat12_64k_img_len;

#define FLASH_SECTOR_SZ       4096u

#define MSC_BLOCK_SIZE        512u
#define MSC_BLOCK_COUNT       128u
#define MSC_DISK_BYTES        (MSC_BLOCK_SIZE * MSC_BLOCK_COUNT)   // 65536

// MSC en los últimos 64KB de flash, alineado a 4KB
#define MSC_FLASH_OFFSET_RAW  (PICO_FLASH_SIZE_BYTES - MSC_DISK_BYTES)
#define MSC_FLASH_OFFSET      (MSC_FLASH_OFFSET_RAW & ~(FLASH_SECTOR_SZ - 1u))

// --- EEPROM existente (solo para check opcional; no se usa para ubicar MSC)
#define EEPROM_FLASH_OFFSET   (1024u * 1024u)
#define EEPROM_FLASH_BYTES    (4096u)

_Static_assert(MSC_DISK_BYTES == 65536u, "MSC disk must be 64KB");
_Static_assert(sizeof(fat12_64k_img) == MSC_DISK_BYTES, "fat12_64k_img size must match MSC disk size");

// Check opcional de solapamiento (habilitar si querés seguridad extra)
#if 0
#if (MSC_FLASH_OFFSET < (EEPROM_FLASH_OFFSET + EEPROM_FLASH_BYTES)) && ((MSC_FLASH_OFFSET + MSC_DISK_BYTES) > EEPROM_FLASH_OFFSET)
#error "MSC flash region overlaps EEPROM emulation region"
#endif
#endif

/*
#include "fat12_64k.h"
#include "tusb.h"            // for tud_mounted()
#include <string.h>
*/
// If your file already has these, don't duplicate—reuse them.

//extern void msc_commit_dirty_to_flash(void);  // or make a non-static wrapper
//extern void msc_init_once(void);              // or call your init
//extern uint8_t  msc_disk[];                   // if msc_disk is static, do it in the same file
//extern uint16_t dirty_mask;



//*--- Internal Prototypes
static void mark_dirty_range(uint32_t addr, uint32_t len);
static void msc_init_once(void);
static void __not_in_flash_func(msc_commit_dirty_to_flash)(void);

// ---- RAM backing + dirty mask (64KB / 4KB = 16 sectores => 16 bits)
static uint8_t  msc_disk[MSC_DISK_BYTES];
static uint16_t dirty_mask = 0;

//*--- Make RAMdisk image available from main.c (read only)
const uint8_t* msc_disk_ro_ptr(void) { msc_init_once(); return msc_disk; }
uint32_t       msc_disk_size_bytes(void) { return MSC_DISK_BYTES; }
bool usb_msc_factory_reset(bool commit_now);

// --- API de boot (firmware) ---
//*--- Prepare msc_disk[] from flash (MUST call before tud_init())
void msc_boot_prepare(void)
{
  //*--- Reuse current init

  msc_init_once();
}

bool usb_msc_factory_reset(bool commit_now)
{
  // Safety: never modify while host has mounted the device
  if (tud_mounted()) return false;

  // Ensure RAM disk exists
  msc_init_once();

  // Reset RAM disk to default image
  memcpy(msc_disk, fat12_64k_img, (size_t)fat12_64k_img_len);

  // Mark whole disk dirty (simple + safe for small 64KB disk)
  dirty_mask = 0xFFFFu;

  if (commit_now) {
    msc_commit_dirty_to_flash();
  }

  return true;
}


//*--- Explicit commit
void msc_boot_commit_now(void)
{
  msc_init_once();
  msc_commit_dirty_to_flash();
}

// ---- Helpers
static inline const uint8_t* msc_flash_ptr(void)
{
  // XIP_BASE viene del SDK (no redefinir)
  return (const uint8_t*)(XIP_BASE + MSC_FLASH_OFFSET);
}

static inline uint8_t sector_index_from_addr(uint32_t addr)
{
  return (uint8_t)(addr / FLASH_SECTOR_SZ); // 0..15
}

static bool flash_has_valid_fat12(void)
{
  const uint8_t* p = msc_flash_ptr();
  // Firma del boot sector 0x55AA
  return (p[510] == 0x55 && p[511] == 0xAA);
}

static void ram_init_from_default(void)
{
  // Carga FAT12 base (NO persiste hasta EJECT)
  memcpy(msc_disk, fat12_64k_img, fat12_64k_img_len);
  dirty_mask = 0;
}

static void ram_init_from_flash(void)
{
  memcpy(msc_disk, msc_flash_ptr(), MSC_DISK_BYTES);
  dirty_mask = 0;
}

static void msc_init_once(void)
{
  static bool inited = false;
  if (inited) return;

  if (flash_has_valid_fat12()) {
    ram_init_from_flash();
  } else {
    ram_init_from_default();
  }

  inited = true;
}

static void __not_in_flash_func(msc_commit_dirty_to_flash)(void)
{
  if (!dirty_mask) return;

  uint32_t ints = save_and_disable_interrupts();

  for (uint8_t s = 0; s < 16; s++) {
    if (dirty_mask & (uint16_t)(1u << s)) {
      uint32_t off = (uint32_t)s * FLASH_SECTOR_SZ;
      flash_range_erase(MSC_FLASH_OFFSET + off, FLASH_SECTOR_SZ);
      flash_range_program(MSC_FLASH_OFFSET + off, &msc_disk[off], FLASH_SECTOR_SZ);
    }
  }

  restore_interrupts(ints);
  dirty_mask = 0;
}

static void mark_dirty_range(uint32_t addr, uint32_t len)
{
  if (!len) return;

  uint32_t start = addr;
  uint32_t end   = addr + len - 1u;

  uint8_t s0 = sector_index_from_addr(start);
  uint8_t s1 = sector_index_from_addr(end);

  for (uint8_t s = s0; s <= s1; s++) {
    dirty_mask |= (uint16_t)(1u << s);
  }
}

// --------------------------------------------------------------------
// TinyUSB MSC callbacks (NO duplicados)
// --------------------------------------------------------------------

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
  (void) lun;

  const char vid[] = "LU7DZ   ";
  const char pid[] = "ADX-ddsPIO MSC   ";
  const char rev[] = "1.0 ";

  memcpy(vendor_id,  vid, 8);
  memcpy(product_id, pid, 16);
  memcpy(product_rev, rev, 4);
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
{
  (void) lun;
  *block_count = MSC_BLOCK_COUNT;
  *block_size  = MSC_BLOCK_SIZE;
}

bool tud_msc_is_writable_cb(uint8_t lun)
{
  (void) lun;
  return true;
}

bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
  (void) lun;
  msc_init_once();
  return true;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
  (void) lun;
  msc_init_once();

  uint32_t addr = lba * MSC_BLOCK_SIZE + offset;
  if (addr + bufsize > MSC_DISK_BYTES) return -1;

  memcpy(buffer, &msc_disk[addr], bufsize);
  return (int32_t) bufsize;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
  (void) lun;
  msc_init_once();

  uint32_t addr = lba * MSC_BLOCK_SIZE + offset;
  if (addr + bufsize > MSC_DISK_BYTES) return -1;

  memcpy(&msc_disk[addr], buffer, bufsize);
  mark_dirty_range(addr, bufsize);
  return (int32_t) bufsize;
}

// SCSI callback requerido por tu TinyUSB (si no está, falla el link)
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{
  (void) buffer;
  (void) bufsize;

  // Si no implementamos comandos SCSI específicos, informamos "illegal request"
  // para que el host no se quede colgado.
  uint8_t const cmd = scsi_cmd[0];
  (void) cmd;

  tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
  return -1;
}

// Commit SOLO al expulsar (load_eject==true). Cambios se leen en el próximo boot.
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
  (void) lun;
  (void) power_condition;
  (void) start;

  if (load_eject) {
    msc_init_once();
    msc_commit_dirty_to_flash();
  }
  return true;
}

bool tud_msc_prevent_allow_medium_removal_cb(uint8_t lun, uint8_t prohibit_removal, uint8_t control)
{
  (void) lun;
  (void) prohibit_removal;
  (void) control;
  return true;
}

// ---------------- FAT12 minimal writer for root file (8.3) ----------------

static inline uint16_t rd16(const uint8_t* p, uint32_t off) {
  return (uint16_t)(p[off] | (p[off + 1] << 8));
}
static inline uint32_t rd32(const uint8_t* p, uint32_t off) {
  return (uint32_t)(p[off] | (p[off + 1] << 8) | (p[off + 2] << 16) | (p[off + 3] << 24));
}
static inline void wr16(uint8_t* p, uint32_t off, uint16_t v) {
  p[off] = (uint8_t)(v & 0xFF);
  p[off + 1] = (uint8_t)((v >> 8) & 0xFF);
}
static inline void wr32(uint8_t* p, uint32_t off, uint32_t v) {
  p[off] = (uint8_t)(v & 0xFF);
  p[off + 1] = (uint8_t)((v >> 8) & 0xFF);
  p[off + 2] = (uint8_t)((v >> 16) & 0xFF);
  p[off + 3] = (uint8_t)((v >> 24) & 0xFF);
}

// FAT12 entry get/set (cluster >= 0)
static uint16_t fat12_get(const uint8_t* fat, uint16_t cluster)
{
  uint32_t i = (uint32_t)cluster + ((uint32_t)cluster / 2u);
  uint16_t v = (uint16_t)(fat[i] | (fat[i + 1] << 8));
  if (cluster & 1u) v >>= 4; else v &= 0x0FFFu;
  return (uint16_t)(v & 0x0FFFu);
}

static void fat12_set(uint8_t* fat, uint16_t cluster, uint16_t value12)
{
  value12 &= 0x0FFFu;
  uint32_t i = (uint32_t)cluster + ((uint32_t)cluster / 2u);

  uint16_t v = (uint16_t)(fat[i] | (fat[i + 1] << 8));

  if (cluster & 1u) {
    // odd cluster -> high 12 bits
    v &= 0x000Fu;
    v |= (uint16_t)(value12 << 4);
  } else {
    // even cluster -> low 12 bits
    v &= 0xF000u;
    v |= value12;
  }

  fat[i] = (uint8_t)(v & 0xFF);
  fat[i + 1] = (uint8_t)((v >> 8) & 0xFF);
}

// Finds root dir entry for name11 (11 bytes) or returns first free slot
static bool fat_root_find_or_free(uint8_t* img, uint32_t root_off, uint32_t root_bytes,
                                  const char name11[11], uint32_t* entry_off_out, bool* existed_out)
{
  uint32_t first_free = 0xFFFFFFFFu;

  for (uint32_t off = 0; off < root_bytes; off += 32u) {
    uint8_t* e = &img[root_off + off];

    if (e[0] == 0x00) { // end of directory (also free)
      if (first_free == 0xFFFFFFFFu) first_free = root_off + off;
      break;
    }
    if (e[0] == 0xE5) { // deleted
      if (first_free == 0xFFFFFFFFu) first_free = root_off + off;
      continue;
    }
    if (e[11] == 0x0F) continue; // LFN

    if (memcmp(e, name11, 11) == 0) {
      *entry_off_out = root_off + off;
      *existed_out = true;
      return true;
    }
  }

  if (first_free != 0xFFFFFFFFu) {
    *entry_off_out = first_free;
    *existed_out = false;
    return true;
  }
  return false;
}

// Free an existing FAT chain starting at first_cluster (FAT12)
static void fat12_free_chain(uint8_t* fat, uint16_t first_cluster)
{
  uint16_t c = first_cluster;
  while (c >= 2u && c < 0x0FF8u) {
    uint16_t next = fat12_get(fat, c);
    fat12_set(fat, c, 0x000u);
    c = next;
  }
}

// Allocate N contiguous clusters, returns first cluster or 0 on failure
static uint16_t fat12_alloc_contiguous(uint8_t* fat, uint16_t start_cluster, uint16_t max_cluster, uint16_t n)
{
  if (n == 0) return 0;

  for (uint16_t c = start_cluster; (uint32_t)c + (uint32_t)n - 1u <= max_cluster; c++) {
    bool ok = true;
    for (uint16_t k = 0; k < n; k++) {
      if (fat12_get(fat, (uint16_t)(c + k)) != 0x000u) { ok = false; break; }
    }
    if (!ok) continue;

    // write chain
    for (uint16_t k = 0; k < n; k++) {
      uint16_t cur = (uint16_t)(c + k);
      uint16_t val = (k == (uint16_t)(n - 1u)) ? 0x0FFFu : (uint16_t)(cur + 1u);
      fat12_set(fat, cur, val);
    }
    return c;
  }
  return 0;
}

bool usb_msc_fw_write_config_sys(const uint8_t* data, uint32_t len, bool commit_now)
{
  if (!data) return false;

  // Opción 1: esta función se usa en BOOT ANTES de tusb_init(),
  // por lo tanto NO debe depender de tud_mounted() ni de estado USB.

  // Asegura que msc_disk[] esté inicializado desde flash/default
  msc_init_once();

  // Parse BPB (FAT12/FAT16)
  uint16_t bps      = rd16(msc_disk, 11);
  uint8_t  spc      = msc_disk[13];
  uint16_t rsvd     = rd16(msc_disk, 14);
  uint8_t  nfats    = msc_disk[16];
  uint16_t root_ent = rd16(msc_disk, 17);
  uint16_t tot16    = rd16(msc_disk, 19);
  uint16_t spf16    = rd16(msc_disk, 22);
  uint32_t tot32    = rd32(msc_disk, 32);

  uint32_t tot_sec = tot16 ? (uint32_t)tot16 : tot32;

  // Sanity mínimo
  if (bps == 0 || spc == 0 || rsvd == 0 || nfats == 0 || root_ent == 0 || spf16 == 0 || tot_sec == 0) {
    return false;
  }

  uint32_t bytes_per_cluster = (uint32_t)bps * (uint32_t)spc;

  uint32_t fat_off   = (uint32_t)rsvd * (uint32_t)bps;
  uint32_t fat_bytes = (uint32_t)spf16 * (uint32_t)bps;

  uint32_t root_off   = fat_off + (uint32_t)nfats * fat_bytes;
  uint32_t root_bytes = (uint32_t)root_ent * 32u;

  uint32_t data_off = root_off + root_bytes;

  // Sanity dentro de nuestra imagen
  if (fat_off + fat_bytes > MSC_DISK_BYTES) return false;
  if (root_off + root_bytes > MSC_DISK_BYTES) return false;
  if (data_off >= MSC_DISK_BYTES) return false;

  // Área de datos / clusters máximos
  uint32_t data_bytes = MSC_DISK_BYTES - data_off;

  // Evita división por cero y discos degenerados
  if (bytes_per_cluster == 0) return false;
  uint16_t max_clusters = (uint16_t)(data_bytes / bytes_per_cluster);
  if (max_clusters < 2) return false;

  // FAT12 clusters numerados desde 2
  uint16_t max_cluster_num = (uint16_t)(max_clusters + 1u);

  // Cuántos clusters necesitamos para len bytes
  uint16_t need_clusters = (uint16_t)((len + bytes_per_cluster - 1u) / bytes_per_cluster);
  if (need_clusters == 0) need_clusters = 1;

  // No aceptar tamaños imposibles
  if ((uint32_t)need_clusters * bytes_per_cluster > data_bytes) return false;

  // Trabajamos sobre FAT#1 y luego espejamos
  uint8_t* fat1 = &msc_disk[fat_off];

  // Buscar o crear entry en root para "CONFIG   SYS"
  const char name11[11] = { 'C','O','N','F','I','G',' ',' ','S','Y','S' };

  uint32_t ent_off = 0;
  bool existed = false;
  if (!fat_root_find_or_free(msc_disk, root_off, root_bytes, name11, &ent_off, &existed)) {
    return false;
  }

  // Si existía, liberamos la cadena anterior
  if (existed) {
    uint16_t old_first = rd16(msc_disk, ent_off + 26);
    if (old_first >= 2u) {
      fat12_free_chain(fat1, old_first);
    }
  }

  // Reservar clusters contiguos para el nuevo archivo
  uint16_t first_cluster = fat12_alloc_contiguous(fat1, 2u, max_cluster_num, need_clusters);
  if (first_cluster == 0) return false;

  // Espejar FAT1 a las demás FATs
  for (uint8_t fi = 1; fi < nfats; fi++) {
    memcpy(&msc_disk[fat_off + (uint32_t)fi * fat_bytes], fat1, fat_bytes);
  }

  // Escribir contenido en clusters
  uint32_t remaining = len;
  const uint8_t* src = data;

  for (uint16_t k = 0; k < need_clusters; k++) {
    uint16_t c = (uint16_t)(first_cluster + k);
    uint32_t cl_index = (uint32_t)(c - 2u);
    uint32_t dst_off = data_off + cl_index * bytes_per_cluster;

    if (dst_off + bytes_per_cluster > MSC_DISK_BYTES) return false;

    uint32_t chunk = (remaining > bytes_per_cluster) ? bytes_per_cluster : remaining;

    if (chunk) memcpy(&msc_disk[dst_off], src, chunk);
    if (chunk < bytes_per_cluster) memset(&msc_disk[dst_off + chunk], 0, bytes_per_cluster - chunk);

    src += chunk;
    remaining -= chunk;
  }

  // Actualizar entry root dir (8.3)
  uint8_t* e = &msc_disk[ent_off];
  memset(e, 0, 32u);
  memcpy(e, name11, 11);
  e[11] = 0x20; // ATTR_ARCHIVE
  wr16(msc_disk, ent_off + 26, first_cluster);
  wr32(msc_disk, ent_off + 28, len);

  // Para nuestro disco chico: marcamos todo sucio
  dirty_mask = 0xFFFFu;

  if (commit_now) {
    msc_commit_dirty_to_flash();
  }

  return true;
}


static bool fw_wrote_config_once = false;

void maybe_generate_config_sys(void)
{
  if (fw_wrote_config_once) return;
  fw_wrote_config_once = true;

  // tu decisión programática:
  bool need_replace = true; // <-- tu lógica
  if (!need_replace) return;

  const char* txt =
    "MODE=FT8\r\n"
    "CALL=LU7DZ\r\n";

  usb_msc_fw_write_config_sys((const uint8_t*)txt, (uint32_t)strlen(txt), true);
}
