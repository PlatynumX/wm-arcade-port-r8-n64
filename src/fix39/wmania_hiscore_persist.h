#ifndef WMANIA_HISCORE_PERSIST_H
#define WMANIA_HISCORE_PERSIST_H

#include "wmania_hiscore_system.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WM_HS_SAVE_VERSION 1u
#define WM_HS_SAVE_MAX_BYTES 1200u

typedef int (*WmHsSaveReadFn)(void *user, void *dst, size_t size);
typedef int (*WmHsSaveWriteFn)(void *user, const void *src, size_t size);

typedef struct {
    WmHsSaveReadFn read;
    WmHsSaveWriteFn write;
    void *user;
} WmHsSaveBackend;

typedef enum {
    WM_HS_LOAD_OK = 0,
    WM_HS_LOAD_REPAIRED = 1,
    WM_HS_LOAD_RESET = 2,
    WM_HS_LOAD_IO_ERROR = 3
} WmHsLoadResult;

/*
 * Portable serialization wrapper. It does not choose SRAM, FlashRAM,
 * Controller Pak, SD, etc.; the port supplies backend callbacks.
 */
size_t wm_hs_save_encoded_size(void);
bool wm_hs_save_encode(
    const WmHsSystem *system,
    uint8_t *buffer,
    size_t capacity,
    size_t *written);
WmHsLoadResult wm_hs_save_decode(
    WmHsSystem *system,
    const uint8_t *buffer,
    size_t size,
    uint32_t adjusted_reset_value);

bool wm_hs_save_write(
    const WmHsSystem *system,
    const WmHsSaveBackend *backend);
WmHsLoadResult wm_hs_save_read(
    WmHsSystem *system,
    const WmHsSaveBackend *backend,
    uint32_t adjusted_reset_value);

#ifdef __cplusplus
}
#endif

#endif
