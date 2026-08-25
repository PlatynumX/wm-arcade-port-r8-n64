#ifndef WM_ARCADE_COMBAT_DEFS_H
#define WM_ARCADE_COMBAT_DEFS_H

#include <stdint.h>

/*
 * Direct semantic translation constants from the original arcade source.
 * Source paths:
 *   PLYR.EQU    player modes, attack modes, status bits
 *   ANIM.EQU    animation mode bits
 *   GAME.EQU    move directions, STAY_TIME / FLUNG_TIME
 *   DISPLAY.EQU object horizontal flip bit
 *
 * Do not renumber these to make them prettier: animation scripts and combat
 * tables use the original numeric values.
 */

enum wm_arcade_player_type {
    WM_PTYPE_PLAYER  = 0,
    WM_PTYPE_DRONE   = 1,
    WM_PTYPE_REFEREE = -1
};

enum wm_arcade_player_mode {
    WM_PMODE_NORMAL       = 0,
    WM_PMODE_RUNNING      = 1,
    WM_PMODE_INAIR        = 2,
    WM_PMODE_ATTACHED     = 3,
    WM_PMODE_ONGROUND     = 4,
    WM_PMODE_BOUNCING     = 5,
    WM_PMODE_ONTURNBKL    = 6,
    WM_PMODE_BLOCK        = 7,
    WM_PMODE_DIZZY        = 8,
    WM_PMODE_DEAD         = 9,
    WM_PMODE_OPPOVERHEAD  = 10,
    WM_PMODE_CLIMBTURNBKL = 11,
    WM_PMODE_WAITANIM     = 12,
    WM_PMODE_GRAPPLE      = 13,
    WM_PMODE_MASTER       = 14,
    WM_PMODE_SLAVE        = 15,
    WM_PMODE_HEADHOLD     = 16,
    WM_PMODE_PUPPET2      = 17,
    WM_PMODE_HEADHELD     = 19,
    WM_PMODE_PUPPET       = 20,
    WM_PMODE_INAIR2       = 21,
    WM_PMODE_CHOKEHOLD    = 24,
    WM_PMODE_CHOKING      = 25
};

enum wm_arcade_attack_mode {
    WM_AMODE_PUNCH             = 0,
    WM_AMODE_HDBUTT            = 1,
    WM_AMODE_KICK              = 2,
    WM_AMODE_FLYKICK           = 3,
    WM_AMODE_GRABTHROW         = 4,
    WM_AMODE_UPRCUT            = 5,
    WM_AMODE_LBOWDROP          = 6,
    WM_AMODE_GRABHOLD          = 7,
    WM_AMODE_GRABFLING         = 8,
    WM_AMODE_PUSH              = 9,
    WM_AMODE_URN               = 10,
    WM_AMODE_BIGBOOT           = 11,
    WM_AMODE_KNEE              = 12,
    WM_AMODE_HDBUTT2           = 13,
    WM_AMODE_BOXPUNCH          = 14,
    WM_AMODE_STOMP             = 15,
    WM_AMODE_SPINKICK          = 16,
    WM_AMODE_CLINE             = 17,
    WM_AMODE_HEADHOLD          = 18,
    WM_AMODE_JUMPKICK          = 19,
    WM_AMODE_RUN               = 20,
    WM_AMODE_PUPPET            = 21,
    WM_AMODE_BACKHAND          = 22,
    WM_AMODE_BUZZ              = 23,
    WM_AMODE_HAYMAKER          = 24,
    WM_AMODE_BLBOWDROP         = 25,
    WM_AMODE_BSTOMP            = 26,
    WM_AMODE_HEADKNEES         = 27,
    WM_AMODE_EARSLAP           = 28,
    WM_AMODE_HAMMER            = 29,
    WM_AMODE_BUTTSTOMP         = 30,
    WM_AMODE_PUPPET2           = 31,
    WM_AMODE_PUPPET_HDGRAB     = 32,
    WM_AMODE_TOMB              = 33,
    WM_AMODE_BIGKNEE           = 34,
    WM_AMODE_SHNBFKIK          = 35,
    WM_AMODE_SHNSPDKIK         = 36,
    WM_AMODE_SHNSPDKIK2        = 37,
    WM_AMODE_HITCHECK          = 38,
    WM_AMODE_UPRCUT2           = 39,
    WM_AMODE_RSLASH            = 40,
    WM_AMODE_HEADDSLASH        = 41,
    WM_AMODE_HEADUSLASH        = 42,
    WM_AMODE_RSLASH2           = 43,
    WM_AMODE_HDBUTT_STAY       = 44,
    WM_AMODE_FIRE_PUNCH        = 45,
    WM_AMODE_BSTOMP2           = 46,
    WM_AMODE_GUTPUSH           = 47,
    WM_AMODE_SUPER_KICK        = 48,
    WM_AMODE_PUNCH2            = 49,
    WM_AMODE_HDBUTT3           = 50,
    WM_AMODE_LBOWDROP2         = 51,
    WM_AMODE_STOMP2            = 52,
    WM_AMODE_PUPPET_NOFLAIL    = 53,
    WM_AMODE_PUPPET_TOSS       = 54,
    WM_AMODE_NAPALM            = 55
};

enum wm_arcade_move_dir {
    WM_MOVE_ZIP        = 0,
    WM_MOVE_UP         = 1,
    WM_MOVE_DOWN       = 2,
    WM_MOVE_LEFT       = 4,
    WM_MOVE_UP_LEFT    = 5,
    WM_MOVE_DOWN_LEFT  = 6,
    WM_MOVE_RIGHT      = 8,
    WM_MOVE_UP_RIGHT   = 9,
    WM_MOVE_DOWN_RIGHT = 10
};

enum wm_arcade_button_bits {
    WM_BTN_PUNCH  = 1u << 0,
    WM_BTN_BLOCK  = 1u << 1,
    WM_BTN_SPUNCH = 1u << 2,
    WM_BTN_KICK   = 1u << 3,
    WM_BTN_SKICK  = 1u << 4
};
#define WM_BTN_ATTACK_MASK 0x001fu

#define WM_J_UP          WM_MOVE_UP
#define WM_J_TOWARD      WM_MOVE_RIGHT
#define WM_J_DOWN        WM_MOVE_DOWN
#define WM_J_AWAY        WM_MOVE_LEFT
#define WM_J_DOWN_TOWARD WM_MOVE_DOWN_RIGHT
#define WM_J_UP_TOWARD   WM_MOVE_UP_RIGHT
#define WM_J_DOWN_AWAY   WM_MOVE_DOWN_LEFT
#define WM_J_UP_AWAY     WM_MOVE_UP_LEFT
#define WM_J_LEFT        (WM_MOVE_LEFT << 8)
#define WM_J_RIGHT       (WM_MOVE_RIGHT << 8)
#define WM_J_REAL_LR     (WM_J_LEFT | WM_J_RIGHT)
#define WM_J_ALL         (0x0fu | WM_J_REAL_LR)
#define WM_B_PUNCH       (WM_BTN_PUNCH << 4)
#define WM_B_BLOCK       (WM_BTN_BLOCK << 4)
#define WM_B_SPUNCH      (WM_BTN_SPUNCH << 4)
#define WM_B_KICK        (WM_BTN_KICK << 4)
#define WM_B_SKICK       (WM_BTN_SKICK << 4)

enum wm_arcade_anim_mode_bits {
    WM_ARCADE_MODE_END          = 0x0001,
    WM_ARCADE_MODE_INTURN       = 0x0002,
    WM_ARCADE_MODE_UNINT        = 0x0004,
    WM_ARCADE_MODE_NOAUTOFLIP   = 0x0008,
    WM_ARCADE_MODE_CHECKHIT     = 0x0010,
    WM_ARCADE_MODE_NOGRAVITY    = 0x0020,
    WM_ARCADE_MODE_FRICTION     = 0x0040,
    WM_ARCADE_MODE_NOCONFINE    = 0x0080,
    WM_ARCADE_MODE_NOCOLLIS     = 0x0100,
    WM_ARCADE_MODE_STATUS       = 0x0200,
    WM_ARCADE_MODE_OVERLAP      = 0x0400,
    WM_ARCADE_MODE_GHOST        = 0x0800,
    WM_ARCADE_MODE_NOSHADOW     = 0x1000,
    WM_ARCADE_MODE_KEEPATTACHED = 0x2000,
    WM_ARCADE_MODE_WAITHITOPP   = 0x4000,
    WM_ARCADE_MODE_INVISIBLE    = 0x8000
};

enum wm_arcade_status_bits {
    WM_STATUS_PRESS_LAST    = 1u << 0,
    WM_STATUS_DID_PIN       = 1u << 1,
    WM_STATUS_TEMP_PAL      = 1u << 2,
    WM_STATUS_ZOMBIE        = 1u << 3,
    WM_STATUS_SMART_ATTACK  = 1u << 4,
    WM_STATUS_PINNED        = 1u << 5,
    WM_STATUS_CAN_XFORM     = 1u << 6,
    WM_STATUS_KOD           = 1u << 7,
    WM_STATUS_NO_KO         = 1u << 8,
    WM_STATUS_PINABLE       = 1u << 9,
    WM_STATUS_SCROLL_CTRL   = 1u << 10,
    WM_STATUS_WEAK_HIT      = 1u << 11,
    WM_STATUS_DO_BUCKOFF    = 1u << 12,
    WM_STATUS_NO_BUCKOFF    = 1u << 13,
    WM_STATUS_DID_BUCKOFF   = 1u << 14,
    WM_STATUS_DEAD_ANIM     = 1u << 15,
    WM_STATUS_DID_RAISEARM  = 1u << 16,
    WM_STATUS_NEW_BUCKOFF   = 1u << 17,
    WM_STATUS_COUNTED_DEAD  = 1u << 18,
    WM_STATUS_COMBO_BROKEN  = 1u << 19,
    WM_STATUS_PUSH          = 1u << 20
};

/* PLYR.EQU exact masks. */
#define WM_STATUS_RESET_MASK \
    (WM_STATUS_TEMP_PAL | WM_STATUS_DID_BUCKOFF)
#define WM_STATUS_CLEAR_ON_SETMODE \
    (WM_STATUS_SCROLL_CTRL | WM_STATUS_DEAD_ANIM | \
     WM_STATUS_DID_RAISEARM | WM_STATUS_KOD | \
     WM_STATUS_COMBO_BROKEN | WM_STATUS_PUSH)

#define WM_OBJ_FLIPH 0x0010u
#define WM_STAY_TIME 270
#define WM_FLUNG_TIME 120

#endif
