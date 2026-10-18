#include "wm/arcade/wm_arcade_final_battle.h"

#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wmania_ring_geometry.h"

#include <string.h>

/*
 * PROGRESS.ASM:1593 get_final_lineup's tail. The shuffle itself is
 * INIT_TEMP_TABLE + RANDOMIZE_ORDER, which pregame.c already carries for
 * the ladder builder; what is left here is the copy and the marker.
 */
void wm_final_set_lineup(wm_final_battle_state_t *fb,
                         const uint8_t shuffled[WM_FINAL_LINEUP_WRESTLERS])
{
    unsigned i;
    if (!fb) return;
    memset(fb->lineup, 0, sizeof fb->lineup);
    if (shuffled) {
        for (i = 0; i < WM_FINAL_LINEUP_WRESTLERS; ++i)
            fb->lineup[i] = (int8_t)shuffled[i];
    }
    /* `movi -1,a14 / move a14,*a1,W` -- a WORD, so both bytes. */
    for (i = WM_FINAL_LINEUP_WRESTLERS; i < WM_FINAL_LINEUP_SLOTS; ++i)
        fb->lineup[i] = -1;
    wm_final_reset_ptr(fb);
}

/* Entries 0..3 as the source's first long, low byte first. */
static uint32_t pack_half(const int8_t *p)
{
    return ((uint32_t)(uint8_t)p[0]) |
           ((uint32_t)(uint8_t)p[1] << 8) |
           ((uint32_t)(uint8_t)p[2] << 16) |
           ((uint32_t)(uint8_t)p[3] << 24);
}

static void unpack_half(int8_t *p, uint32_t v)
{
    p[0] = (int8_t)(v & 0xffu);
    p[1] = (int8_t)((v >> 8) & 0xffu);
    p[2] = (int8_t)((v >> 16) & 0xffu);
    p[3] = (int8_t)((v >> 24) & 0xffu);
}

/*
 * The test the source runs twice, on the low two BYTES of the leading
 * long: `andi >00FF` is entry 0 and `andi >FF00 / srl 8` is entry 1.
 */
static bool leads_with(uint32_t a3, uint8_t index1, uint8_t index2)
{
    uint8_t first = (uint8_t)(a3 & 0xffu);
    uint8_t second = (uint8_t)((a3 >> 8) & 0xffu);
    return first == index1 || first == index2 ||
           second == index1 || second == index2;
}

void wm_final_royal_fixup(wm_final_battle_state_t *fb,
                          uint8_t index1, uint8_t index2)
{
    uint32_t a3, a4, t;
    if (!fb) return;

    a3 = pack_half(&fb->lineup[0]);
    a4 = pack_half(&fb->lineup[4]);

    if (leads_with(a3, index1, index2)) {
        /* `#swap SWAP a3,a4`. */
        t = a3; a3 = a4; a4 = t;
        if (leads_with(a3, index1, index2)) {
            /* `#rot rl 16,a3` -- entries 2 and 3 move in front of 0 and 1. */
            a3 = (a3 << 16) | (a3 >> 16);
        }
    }

    unpack_half(&fb->lineup[0], a3);
    unpack_half(&fb->lineup[4], a4);
}

void wm_final_reset_ptr(wm_final_battle_state_t *fb)
{
    if (!fb) return;
    fb->ptr = (uint8_t)WM_FINAL_PTR_START;
}

bool wm_final_queue_empty(const wm_final_battle_state_t *fb)
{
    if (!fb) return true;
    if (fb->ptr >= WM_FINAL_LINEUP_SLOTS) return true;
    return fb->lineup[fb->ptr] < 0;
}

int wm_final_next_wrestler(wm_final_battle_state_t *fb)
{
    int value;
    if (wm_final_queue_empty(fb)) return -1;
    value = (int)fb->lineup[fb->ptr];
    /* `addk 8,a0 / move a0,@FINAL_PTR,L`, only on the live path. */
    ++fb->ptr;
    return value;
}

/*
 * ANIM.ASM:3113-3128. RING_TOP is 1023 and RING_BOT 1345 (RING.EQU:48),
 * so Z grows from the back of the ring toward the front.
 */
int32_t wm_final_zombie_z_nudge(int32_t z)
{
    /* `cmpi RING_TOP+7,a14 / jrle #mvdn` -- at or behind the back edge. */
    if (z <= (int32_t)(WM_RING_TOP + 7))
        return z + 7;            /* `#mvdn addk 7,a14` */
    /* `cmpi RING_BOT-7,a14 / jrle #zombie` -- comfortably inside. */
    if (z <= (int32_t)(WM_RING_BOT - 7))
        return z;
    return z - 7;                /* `subk 7,a14`, away from the front edge */
}

void wm_final_make_zombie(struct wm_arcade_actor *actor, int wrestler)
{
    if (!actor) return;

    /*
     * `;7->8 hack / cmpi 7,a1 / jrne #vok / movk 8,a1`. Packed slot 7 is
     * the spare the roster never fills; live wrestler 8 is Lex. It is
     * the same mapping SORT_OUT_WRESTLER_NUM applies to every ladder
     * entry, written out by hand here because the queue is read raw.
     */
    if (wrestler == 7) wrestler = 8;

    actor->new_wrestlernum = wrestler;
    actor->status_flags |= WM_STATUS_ZOMBIE;
    actor->zombie_time = 0;
    actor->z_int = wm_final_zombie_z_nudge(actor->z_int);
}

/*
 * DOINK.ASM:3247 #run_speeds -> PLYR.EQU:452-475. Slot 7 is the roster's
 * unfilled spare and the table says `.long 0` for it, so a zombie who
 * somehow became him would stand still rather than run off.
 */
const int32_t wm_final_run_speeds[WM_FINAL_RUN_SPEEDS] = {
    0x64000,   /* 0 HRT_XRUN */
    0x60000,   /* 1 RZR_XRUN */
    0x64000,   /* 2 UND_XRUN */
    0x58000,   /* 3 YOK_XRUN */
    0x64000,   /* 4 SHN_XRUN */
    0x64000,   /* 5 BAM_XRUN */
    0x64000,   /* 6 DNK_XRUN */
    0,         /* 7 the spare */
    0x60000    /* 8 LEX_XRUN */
};

/* WRESTLE2.ASM:3852 #init_positions, verbatim. */
const wm_final_start_pos_t
wm_final_start_positions[WM_FINAL_START_POSITIONS] = {
    { WM_RING_X_CENTER,  WM_RING_Z_CENTER, WM_MAT_Y, 0 }, /* centre */
    { WM_RING_TOP_LEFT,  WM_RING_Z_CENTER, WM_MAT_Y, 0 }, /* centre left */
    { WM_RING_TOP_RIGHT, WM_RING_Z_CENTER, WM_MAT_Y, 0 }, /* centre right */
    { WM_RING_BOT_LEFT,  WM_RING_BOT,      WM_MAT_Y, 0 }, /* bottom left */
    { WM_RING_BOT_RIGHT, WM_RING_BOT,      WM_MAT_Y, 0 }, /* bottom right */
    { 0x024f,            0x060e,           0,        1 }, /* outside left */
    { 0x060e,            0x04ab,           0,        1 }  /* outside right */
};

unsigned wm_final_choose_start_position(int32_t world_tlx)
{
    unsigned i;
    /*
     * `#lp1 move *a0(#NXT),a14,W / jrn #usea0` peeks at the NEXT entry's
     * first word and stops on the -1 terminator, so the last entry is
     * taken without being tested at all.
     */
    for (i = 0; i + 1u < (unsigned)WM_FINAL_START_POSITIONS; ++i) {
        int32_t x = wm_final_start_positions[i].x;
        int32_t left = world_tlx - 30;
        if (x <= left) return i;
        /* `addi 460,a14` onto the already-decremented edge. */
        if (x >= left + 460) return i;
    }
    return (unsigned)(WM_FINAL_START_POSITIONS - 1);
}

void wm_final_change_wrestler(struct wm_arcade_actor *actor,
                              int32_t world_tlx)
{
    const wm_final_start_pos_t *pos;
    if (!actor) return;

    actor->wrestler_num = actor->new_wrestlernum;
    actor->player_mode = WM_PMODE_NORMAL;
    /* `clr a14 / move a14,*a13(STATUS_FLAGS),L` -- the whole long, which
       is how M_ZOMBIE and M_CAN_XFORM come back off. */
    actor->status_flags = 0u;
    actor->i_will_die = 0;

    pos = &wm_final_start_positions[wm_final_choose_start_position(world_tlx)];
    actor->x_int = pos->x;
    actor->z_int = pos->z;
    actor->y_int = pos->y;
    actor->ground_y = pos->y;
    actor->in_ring = pos->in_ring;

    actor->x_vel = 0;
    actor->y_vel = 0;
    actor->z_vel = 0;
}

bool wm_final_zombie_tick(struct wm_arcade_actor *actor,
                          int32_t world_tlx,
                          bool *started_run,
                          bool *run_left,
                          int32_t *x_vel)
{
    int32_t speed;
    bool left;

    if (started_run) *started_run = false;
    if (run_left) *run_left = false;
    if (x_vel) *x_vel = 0;
    if (!actor) return false;

    /* `move *a13(ZOMBIE_TIME),a14 / inc a14 / move a14,... /
       cmpi TSEC*10,a14 / jrlt #zmb_ok`. */
    ++actor->zombie_time;
    if (actor->zombie_time >= WM_FINAL_ZOMBIE_TIMEOUT)
        return true;                    /* `#change` -- transform now */

    /* `btst MODE_END_BIT,a14 / jrz #done`: still getting up. */
    if (!(actor->anim_mode & WM_MODE_END)) return false;

    /* `ori M_CAN_XFORM,a14` -- from here, hitting the arena edge is his
       cue, which confine_wrestler's own #dead branch acts on. */
    actor->status_flags |= WM_STATUS_CAN_XFORM;

    /* `cmpi RING_X_CENTER-200,a14 / jrge #run_left`. */
    left = world_tlx >= (int32_t)(WM_RING_X_CENTER - 200);
    speed = 0;
    if (actor->wrestler_num >= 0 &&
        actor->wrestler_num < WM_FINAL_RUN_SPEEDS)
        speed = wm_final_run_speeds[actor->wrestler_num];

    actor->stick_val_cur = (uint16_t)(left ? WM_MOVE_LEFT : WM_MOVE_RIGHT);
    actor->x_vel = left ? -speed : speed;

    if (started_run) *started_run = true;
    if (run_left) *run_left = left;
    if (x_vel) *x_vel = actor->x_vel;
    return false;
}
