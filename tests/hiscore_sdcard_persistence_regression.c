#include "wm/sdcard_hiscore_backend.h"
#include "wm/arcade/wmania_hiscore_system.h"
#include "wm/arcade/wmania_hiscore_persist.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *make_test_root(char templ[1024])
{
    int n = snprintf(templ, 1024, ".r37_hiscore_sd_%ld", (long)getpid());
    if (n < 0 || n >= 1024) return NULL;
    (void)rmdir(templ);
    if (mkdir(templ, 0700) != 0) return NULL;
    return templ;
}

int main(void)
{
    char templ[1024];
    char *root = make_test_root(templ);
    WmHsSdCardBackend sd;
    WmHsSaveBackend backend;
    WmHsSystem before;
    WmHsSystem after;
    uint8_t a[WM_HS_SAVE_MAX_BYTES];
    uint8_t b[WM_HS_SAVE_MAX_BYTES];
    size_t aw = 0u;
    size_t bw = 0u;
    const char *path;

    if (root == NULL) {
        perror("mkdir test root");
        fprintf(stderr, "template=%s\n", templ[0] ? templ : "(empty)");
        return 2;
    }

    assert(wm_hs_sdcard_backend_init(&sd, &backend, root));
    path = wm_hs_sdcard_backend_path(&sd);
    assert(strstr(path, "/wm_arcade/hiscore.whs") != NULL);
    assert(backend.read != NULL);
    assert(backend.write != NULL);
    assert(backend.user == &sd);

    wm_hs_system_init(&before, 0x12345678u);
    before.recent_index[WM_HS_TABLE_STREAK] = 1u;
    before.entered_initials[0][0] = 'B';
    before.entered_initials[0][1] = 'A';
    before.entered_initials[0][2] = 'M';
    before.entered_initials[0][3] = 'P';
    before.entered_initials[0][4] = 'X';

    if (!wm_hs_save_write(&before, &backend)) {
        fprintf(stderr, "wm_hs_save_write failed for path=%s\n", path);
        return 3;
    }
    assert(access(path, F_OK) == 0);

    assert(wm_hs_save_read(&after, &backend, 0x12345678u) == WM_HS_LOAD_OK);
    assert(wm_hs_save_encode(&before, a, sizeof(a), &aw));
    assert(wm_hs_save_encode(&after, b, sizeof(b), &bw));
    assert(aw == bw);
    assert(memcmp(a, b, aw) == 0);

    assert(strncmp(WM_HS_SDCARD_SAVE_PATH, "sd:/", 4u) == 0);
    assert(strstr(WM_HS_SDCARD_SAVE_PATH, "sram") == NULL);
    assert(strstr(WM_HS_SDCARD_SAVE_PATH, "eeprom") == NULL);
    assert(strstr(WM_HS_SDCARD_SAVE_PATH, "flash") == NULL);

    printf("High-score SD-card persistence regression: PASS\n");
    printf("default_path=%s\n", WM_HS_SDCARD_SAVE_PATH);
    printf("host_test_path=%s\n", path);

    /* Host regression residue must never become a port asset or Git commit. */
    {
        char dir[1024];
        if (snprintf(dir, sizeof(dir), "%s/wm_arcade", root) < (int)sizeof(dir)) {
            (void)unlink(path);
            (void)rmdir(dir);
        }
        (void)rmdir(root);
    }
    return 0;
}
