/*
 * The high-score tables' cartridge storage.
 *
 * wm/arcade/wmania_hiscore_persist.h deliberately does not choose one:
 * it takes two callbacks and says "the port supplies backend
 * callbacks". This is that choice, made once and written down.
 *
 * WHY EEPROM. The encoded tables are 1,130 bytes. A 4 kbit EEPROM is
 * 512 and cannot hold them; a 16 kbit EEPROM is 2,048 and can, with
 * room to spare. Controller Pak would work too but is removable and
 * shared with other games, and the arcade's tables are the machine's
 * rather than the player's -- an EEPROM soldered to the cartridge is
 * the closer analogue of the CMOS the source checks with
 * TABLE_CMOS_CHECK, right down to failing over to the factory tables
 * when the checksum does not hold.
 *
 * libdragon's eepromfs gives exactly the two operations the backend
 * wants, plus a signature it can verify and a wipe for a fresh chip.
 * A cartridge with no EEPROM, or one too small, simply reports the
 * failure: wm_hs_save_read then builds the factory tables, which is
 * what a new machine does anyway, and wm_hs_save_write refuses rather
 * than half-writing.
 */
#include "wm/arcade/wmania_hiscore_persist.h"

#include <stddef.h>
#include <string.h>

#include <eeprom.h>
#include <eepromfs.h>

#define WM_HS_EEPROM_PATH "/hiscore.dat"
/* Rounded up to whole 8-byte EEPROM blocks. */
#define WM_HS_EEPROM_BYTES ((WM_HS_SAVE_MAX_BYTES + 7u) & ~7u)

static const eepfs_entry_t wm_hs_eepfs_files[] = {
    { WM_HS_EEPROM_PATH, WM_HS_EEPROM_BYTES }
};

/* One staging copy, because eepromfs reads and writes whole files and
   the backend hands its bytes over in pieces. */
static uint8_t wm_hs_staging[WM_HS_EEPROM_BYTES];
static size_t wm_hs_cursor;
static bool wm_hs_ready;

/*
 * Mounted once. `eeprom_total_blocks` is how big the chip actually is;
 * a 4 kbit part reports 64 blocks (512 bytes) and is refused here
 * rather than corrupting a partial save.
 */
bool wm_n64_hiscore_storage_init(void) {
    if (wm_hs_ready) return true;
    if (eeprom_present() == EEPROM_NONE) return false;
    if (eeprom_total_blocks() * 8u < WM_HS_EEPROM_BYTES) return false;
    if (eepfs_init(wm_hs_eepfs_files, 1) != EEPFS_ESUCCESS) return false;
    /* A chip that has never held this layout is wiped to zeros, which
       decodes as "no save" and gets the factory tables. */
    if (!eepfs_verify_signature()) eepfs_wipe();
    wm_hs_ready = true;
    return true;
}

static int wm_n64_hiscore_read(void *user, void *dst, size_t size) {
    (void)user;
    if (!wm_hs_ready || !dst) return -1;
    if (wm_hs_cursor == 0u &&
        eepfs_read(WM_HS_EEPROM_PATH, wm_hs_staging, WM_HS_EEPROM_BYTES) !=
            EEPFS_ESUCCESS)
        return -1;
    if (wm_hs_cursor + size > WM_HS_EEPROM_BYTES) return -1;
    memcpy(dst, wm_hs_staging + wm_hs_cursor, size);
    wm_hs_cursor += size;
    return 0;
}

static int wm_n64_hiscore_write(void *user, const void *src, size_t size) {
    (void)user;
    if (!wm_hs_ready || !src) return -1;
    if (wm_hs_cursor + size > WM_HS_EEPROM_BYTES) return -1;
    memcpy(wm_hs_staging + wm_hs_cursor, src, size);
    wm_hs_cursor += size;
    return 0;
}

/* The backend, with the cursor reset around each whole operation --
   the encoder and decoder each make one pass. */
void wm_n64_hiscore_backend(WmHsSaveBackend *out) {
    if (!out) return;
    out->read = wm_n64_hiscore_read;
    out->write = wm_n64_hiscore_write;
    out->user = NULL;
}

void wm_n64_hiscore_begin(void) { wm_hs_cursor = 0u; }

/* Called after wm_hs_save_write has filled the staging copy. */
bool wm_n64_hiscore_flush(void) {
    if (!wm_hs_ready) return false;
    return eepfs_write(WM_HS_EEPROM_PATH, wm_hs_staging,
                       WM_HS_EEPROM_BYTES) == EEPFS_ESUCCESS;
}
