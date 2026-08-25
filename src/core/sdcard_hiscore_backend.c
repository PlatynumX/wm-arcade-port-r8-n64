#include "wm/sdcard_hiscore_backend.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static int make_dir(const char *path)
{
    struct stat st;
    if (!path || !*path) return -1;

    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode) ? 0 : -1;
    }

    if (mkdir(path, 0777) == 0) return 0;

    /* Android/Termux storage providers can race between stat and mkdir.
       Re-check before failing so existing mount/path components do not block
       the portable high-score backend proof. */
    if (errno == EEXIST && stat(path, &st) == 0 && S_ISDIR(st.st_mode)) return 0;
    return -1;
}

static int ensure_parent_dirs(const char *file_path)
{
    char tmp[WM_HS_SDCARD_BACKEND_PATH_CAPACITY];
    size_t len;
    size_t i;

    if (!file_path) return -1;
    len = strlen(file_path);
    if (len == 0 || len >= sizeof(tmp)) return -1;
    memcpy(tmp, file_path, len + 1u);

    /* Skip URI/mount prefixes such as sd:/ so libdragon-style paths remain
       legal. If there is no URI prefix (the host-test case), begin at byte 0.
       R36C incorrectly left i parked on the terminating NUL for relative
       paths, so wm_arcade/ was never created before fopen(). */
    {
        size_t start = 0u;
        for (i = 0u; tmp[i] != '\0'; ++i) {
            if (tmp[i] == ':' && tmp[i + 1u] == '/') {
                start = i + 2u;
                break;
            }
        }
        i = start;
    }

    for (; tmp[i] != '\0'; ++i) {
        if (tmp[i] == '/') {
            tmp[i] = '\0';
            if (strlen(tmp) > 0u && make_dir(tmp) != 0) return -1;
            tmp[i] = '/';
        }
    }
    return 0;
}

static int sd_read(void *user, void *dst, size_t size)
{
    const WmHsSdCardBackend *state = (const WmHsSdCardBackend *)user;
    FILE *f;
    size_t got;
    if (!state || !dst || size == 0u) return -1;
    f = fopen(state->path, "rb");
    if (!f) return -1;
    got = fread(dst, 1u, size, f);
    if (fclose(f) != 0) return -1;
    return got == size ? 0 : -1;
}

static int sd_write(void *user, const void *src, size_t size)
{
    const WmHsSdCardBackend *state = (const WmHsSdCardBackend *)user;
    FILE *f;
    size_t put;
    if (!state || !src || size == 0u) return -1;
    if (ensure_parent_dirs(state->path) != 0) return -1;
    f = fopen(state->path, "wb");
    if (!f) return -1;
    put = fwrite(src, 1u, size, f);
    if (fflush(f) != 0) { (void)fclose(f); return -1; }
    if (fclose(f) != 0) return -1;
    return put == size ? 0 : -1;
}

bool wm_hs_sdcard_backend_init(WmHsSdCardBackend *state,
                               WmHsSaveBackend *backend,
                               const char *root_override)
{
    int n;
    if (!state || !backend) return false;
    memset(state, 0, sizeof(*state));
    memset(backend, 0, sizeof(*backend));

    if (root_override && root_override[0] != '\0') {
        size_t len = strlen(root_override);
        const char *slash = (len > 0u && root_override[len - 1u] == '/') ? "" : "/";
        n = snprintf(state->path, sizeof(state->path), "%s%swm_arcade/hiscore.whs",
                     root_override, slash);
    } else {
        n = snprintf(state->path, sizeof(state->path), "%s", WM_HS_SDCARD_SAVE_PATH);
    }
    if (n < 0 || (size_t)n >= sizeof(state->path)) return false;

    backend->read = sd_read;
    backend->write = sd_write;
    backend->user = state;
    return true;
}

const char *wm_hs_sdcard_backend_path(const WmHsSdCardBackend *state)
{
    return state ? state->path : "";
}
