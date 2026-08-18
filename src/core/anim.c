#include "wm/anim.h"
#include <string.h>

static bool take_word_legacy(wm_anim_state *a, uint16_t *out) {
    if (a->pc >= a->word_count) return false;
    *out = a->words[a->pc++];
    return true;
}

void wm_anim_start(wm_anim_state *a, const uint16_t *words, size_t word_count) {
    memset(a, 0, sizeof(*a));
    a->words = words;
    a->word_count = word_count;
    a->speed = 0x100;
}

bool wm_anim_step(wm_anim_state *a, wm_object *obj) {
    if (!a || !obj || a->ended) return false;
    if (a->pause_ticks) {
        --a->pause_ticks;
        return true;
    }

    while (a->pc < a->word_count) {
        uint16_t op;
        if (!take_word_legacy(a, &op)) break;
        switch (op) {
            case WM_ANI_SETMODE: {
                uint16_t v;
                if (!take_word_legacy(a, &v)) goto malformed;
                a->mode = v;
                obj->visible = (v & WM_MODE_INVISIBLE) == 0;
                break;
            }
            case WM_ANI_ZEROVELS:
                obj->vx = obj->vy = obj->vz = 0;
                break;
            case WM_ANI_ZERO_XZVELS:
                obj->vx = obj->vz = 0;
                break;
            case WM_ANI_SET_YVEL:
            case WM_ANI_SET_XVEL:
            case WM_ANI_SET_ZVEL:
                /* These source commands consume LONG (and X/Z also WORD mode).
                   The old generated uint16_t stream cannot represent them. */
                a->unsupported = true;
                a->ended = true;
                return false;
            case WM_ANI_SETSPEED: {
                uint16_t v;
                if (!take_word_legacy(a, &v)) goto malformed;
                a->speed = v;
                break;
            }
            case WM_ANI_SETFACING:
            case WM_ANI_SET_WRESTLER_XFLIP:
                break;
            case WM_ANI_PAUSE: {
                uint16_t ticks;
                if (!take_word_legacy(a, &ticks)) goto malformed;
                a->pause_ticks = ticks;
                return true;
            }
            case WM_ANI_END:
                a->mode |= WM_MODE_END;
                a->ended = true;
                return false;
            default:
                a->unsupported = true;
                a->ended = true;
                return false;
        }
    }

malformed:
    a->malformed = true;
    a->ended = true;
    return false;
}

static bool take_u16(wm_source_anim_state *a, uint16_t *out) {
    if (!a || !out || a->pc + 2 > a->byte_count) return false;
    *out = (uint16_t)(((uint16_t)a->bytes[a->pc] << 8) |
                      (uint16_t)a->bytes[a->pc + 1]);
    a->pc += 2;
    return true;
}

static bool take_u32(wm_source_anim_state *a, uint32_t *out) {
    if (!a || !out || a->pc + 4 > a->byte_count) return false;
    *out = ((uint32_t)a->bytes[a->pc] << 24) |
           ((uint32_t)a->bytes[a->pc + 1] << 16) |
           ((uint32_t)a->bytes[a->pc + 2] << 8) |
           (uint32_t)a->bytes[a->pc + 3];
    a->pc += 4;
    return true;
}

static int32_t source_signed_long(uint32_t raw) {
    return (int32_t)raw;
}

void wm_source_anim_start(wm_source_anim_state *a,
                          const uint8_t *bytes, size_t byte_count) {
    memset(a, 0, sizeof(*a));
    a->bytes = bytes;
    a->byte_count = byte_count;
    a->base_pc = 0;
    a->speed = 0x100;
}

bool wm_source_anim_step(wm_source_anim_state *a, wm_object *obj) {
    if (!a || !obj || a->ended) return false;

    /* OANICNT is decremented before command fetch. We store only the number of
       future ticks that still need to return before the next command fetch. */
    if (a->hold_ticks) {
        --a->hold_ticks;
        if (a->hold_ticks) return true;
    }

    for (unsigned guard = 0; guard < 1024 && a->pc < a->byte_count; ++guard) {
        uint16_t token;
        if (!take_u16(a, &token)) goto malformed;

        if (token < 0x8000u) {
            if (token == 0) continue; /* source _ani_zip/no-op */
            uint32_t frame_ref;
            if (!take_u32(a, &frame_ref)) goto malformed;
            a->current_frame_ref = frame_ref;
            uint32_t ticks = ((uint32_t)token * a->speed) >> 8;
            a->hold_ticks = (uint16_t)(ticks > 0xFFFFu ? 0xFFFFu : ticks);
            return true;
        }

        switch (token) {
            case WM_ANI_ZIP:
                break;
            case WM_ANI_REPEAT:
                a->pc = a->base_pc;
                break;
            case WM_ANI_SETMODE: {
                uint16_t mode;
                if (!take_u16(a, &mode)) goto malformed;
                a->mode = mode;
                obj->visible = (mode & WM_MODE_INVISIBLE) == 0;
                break;
            }
            case WM_ANI_ZEROVELS:
                obj->vx = obj->vy = obj->vz = 0;
                break;
            case WM_ANI_SET_YVEL: {
                uint32_t raw;
                if (!take_u32(a, &raw)) goto malformed;
                obj->vy = source_signed_long(raw);
                break;
            }
            case WM_ANI_SETFACING:
                a->facing_right = a->new_facing_right;
                break;
            case WM_ANI_PAUSE: {
                uint16_t ticks;
                if (!take_u16(a, &ticks)) goto malformed;
                a->hold_ticks = ticks;
                return true;
            }
            case WM_ANI_SETSPEED: {
                uint16_t speed;
                if (!take_u16(a, &speed)) goto malformed;
                a->speed = speed;
                break;
            }
            case WM_ANI_ZERO_XZVELS:
                obj->vx = obj->vz = 0;
                break;
            case WM_ANI_SET_XVEL: {
                uint32_t raw;
                uint16_t relative;
                if (!take_u32(a, &raw) || !take_u16(a, &relative)) goto malformed;
                int32_t vel = source_signed_long(raw);
                bool positive = true;
                if (relative == 1) positive = a->facing_right;
                else if (relative == 2) positive = a->hit_from_right;
                else if (relative != 0) positive = a->new_facing_right;
                if (relative != 0 && !positive) vel = -vel;
                obj->vx = vel;
                break;
            }
            case WM_ANI_SET_ZVEL: {
                uint32_t raw;
                uint16_t relative;
                if (!take_u32(a, &raw) || !take_u16(a, &relative)) goto malformed;
                int32_t vel = source_signed_long(raw);
                bool positive = true;
                if (relative == 1) positive = a->facing_down;
                else if (relative != 0) positive = a->hit_from_above;
                if (relative != 0 && !positive) vel = -vel;
                obj->vz = vel;
                break;
            }
            case WM_ANI_SET_WRESTLER_XFLIP:
                a->xflip = !a->facing_right;
                break;
            case WM_ANI_END:
                a->mode |= WM_MODE_END;
                a->ended = true;
                return false;
            default:
                a->unsupported = true;
                a->ended = true;
                return false;
        }
    }

    if (a->pc >= a->byte_count) goto malformed;
    a->unsupported = true; /* runaway REPEAT/ZIP command loop */
    a->ended = true;
    return false;

malformed:
    a->malformed = true;
    a->ended = true;
    return false;
}
