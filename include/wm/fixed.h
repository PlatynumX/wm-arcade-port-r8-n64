#ifndef WM_FIXED_H
#define WM_FIXED_H
#include <stdint.h>

typedef int32_t wm_fix16;
#define WM_FIX_ONE ((wm_fix16)0x00010000)
static inline wm_fix16 wm_fix_from_int(int32_t v) { return (wm_fix16)(v << 16); }
static inline int32_t wm_fix_to_int(wm_fix16 v) { return v >> 16; }
static inline wm_fix16 wm_fix_mul(wm_fix16 a, wm_fix16 b) {
    return (wm_fix16)(((int64_t)a * (int64_t)b) >> 16);
}
#endif
