#ifndef WM_ANIM_H
#define WM_ANIM_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "wm/object.h"

/* Values copied from the original ANIM.EQU command IDs. */
enum wm_anim_opcode {
    WM_ANI_ZIP                 = 0x8000,
    WM_ANI_REPEAT              = 0x8001,
    WM_ANI_SETMODE             = 0x8002,
    WM_ANI_ZEROVELS            = 0x8003,
    WM_ANI_SETPLYRMODE         = 0x8004,
    WM_ANI_SET_YVEL            = 0x8005,
    WM_ANI_SOUND               = 0x801A,
    WM_ANI_SETFACING           = 0x801B,
    WM_ANI_PAUSE               = 0x801C,
    WM_ANI_SETSPEED            = 0x8026,
    WM_ANI_ZERO_XZVELS         = 0x8028,
    WM_ANI_SET_XVEL            = 0x802C,
    WM_ANI_SET_ZVEL            = 0x8030,
    WM_ANI_END                 = 0x8049,
    WM_ANI_SET_WRESTLER_XFLIP  = 0x805F
};

enum wm_anim_mode {
    WM_MODE_NORMAL       = 0x0000,
    WM_MODE_END          = 0x0001,
    WM_MODE_INTURN       = 0x0002,
    WM_MODE_UNINT        = 0x0004,
    WM_MODE_NOAUTOFLIP   = 0x0008,
    WM_MODE_CHECKHIT     = 0x0010,
    WM_MODE_NOGRAVITY    = 0x0020,
    WM_MODE_FRICTION     = 0x0040,
    WM_MODE_NOCONFINE    = 0x0080,
    WM_MODE_NOCOLLIS     = 0x0100,
    WM_MODE_STATUS       = 0x0200,
    WM_MODE_OVERLAP      = 0x0400,
    WM_MODE_GHOST        = 0x0800,
    WM_MODE_NOSHADOW     = 0x1000,
    WM_MODE_KEEPATTACHED = 0x2000,
    WM_MODE_WAITHITOPP   = 0x4000,
    WM_MODE_INVISIBLE    = 0x8000
};

/* Legacy WORD-only stream retained for already generated bring-up sequences.
   Commands requiring LONG operands fail closed instead of truncating source
   data to 16 bits. */
typedef struct {
    const uint16_t *words;
    size_t word_count;
    size_t pc;
    uint16_t mode;
    uint16_t speed;
    bool xflip;
    bool ended;
    bool unsupported;
    bool malformed;
    uint16_t pause_ticks;
} wm_anim_state;

void wm_anim_start(wm_anim_state *a, const uint16_t *words, size_t word_count);
bool wm_anim_step(wm_anim_state *a, wm_object *obj);

/* Typed source stream. Original animation data mixes WORD and LONG operands;
   this view consumes the byte stream exactly as the TMS34010 source does. */
typedef struct {
    const uint8_t *bytes;
    size_t byte_count;
    size_t base_pc;
    size_t pc;
    uint16_t mode;
    uint16_t speed;
    uint32_t current_frame_ref;
    uint16_t hold_ticks;
    bool xflip;
    bool ended;
    bool malformed;
    bool unsupported;

    /* Minimal source player-direction state used by translated velocity/flip
       commands. true means the matching source direction bit is set. */
    bool facing_right;
    bool facing_down;
    bool new_facing_right;
    bool hit_from_right;
    bool hit_from_above;
} wm_source_anim_state;

void wm_source_anim_start(wm_source_anim_state *a,
                          const uint8_t *bytes, size_t byte_count);
bool wm_source_anim_step(wm_source_anim_state *a, wm_object *obj);

#endif
