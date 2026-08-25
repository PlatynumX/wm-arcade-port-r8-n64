#ifndef WM_SDCARD_HISCORE_BACKEND_H
#define WM_SDCARD_HISCORE_BACKEND_H

#include "wm/arcade/wmania_hiscore_persist.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WM_HS_SDCARD_SAVE_DIR "sd:/wm_arcade"
#define WM_HS_SDCARD_SAVE_PATH "sd:/wm_arcade/hiscore.whs"
#define WM_HS_SDCARD_BACKEND_PATH_CAPACITY 512u

typedef struct {
    char path[WM_HS_SDCARD_BACKEND_PATH_CAPACITY];
} WmHsSdCardBackend;

/*
 * Bind the portable HSTD persistence codec to the N64 SD-card filesystem.
 *
 * root_override is for host tests only.  If null/empty, the backend uses the
 * real N64 target path WM_HS_SDCARD_SAVE_PATH (sd:/wm_arcade/hiscore.whs).
 */
bool wm_hs_sdcard_backend_init(WmHsSdCardBackend *state,
                               WmHsSaveBackend *backend,
                               const char *root_override);
const char *wm_hs_sdcard_backend_path(const WmHsSdCardBackend *state);

#ifdef __cplusplus
}
#endif

#endif
