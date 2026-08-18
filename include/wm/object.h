#ifndef WM_OBJECT_H
#define WM_OBJECT_H
#include <stdbool.h>
#include <stdint.h>
#include "wm/fixed.h"

typedef struct {
    uint32_t image_id;
    wm_fix16 x, y, z;
    wm_fix16 vx, vy, vz;
    uint16_t flags;
    uint16_t class_id;
    bool visible;
} wm_object;

#endif
