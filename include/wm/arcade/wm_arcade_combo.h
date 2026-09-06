#ifndef WM_ARCADE_COMBO_H
#define WM_ARCADE_COMBO_H

#include "wm/arcade/wm_arcade_announcer.h"
#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The combo meter: ANIM.ASM's three combo opcodes and the LIFEBAR.ASM
 * routine behind them. There are TWO separate counters and they are easy
 * to confuse:
 *
 *   COMBO_COUNT     per wrestler, bumped by ANI_INC_COMBO. This is what
 *                   adjust_health reads, and a hit landed while it is
 *                   non-zero does FIXED damage of -max(10-COMBO_COUNT, 4)
 *                   instead of the normal scaled amount. Nothing in this
 *                   port incremented it before -- wm_arcade_lifebar.h said
 *                   so in its own notes -- so the whole combo damage rule
 *                   was dead code with a correct implementation.
 *
 *   PLT_COMBO_SIZE  per player, grown by ANI_ADD_MOVE through
 *                   LIFEBAR.ASM:750 ADD_TO_COMBO_COUNT. This is the meter
 *                   BAR, and at 16 it trips the super-combo flash.
 */

/*
 * LIFEBAR.ASM:750 SUBR ADD_TO_COMBO_COUNT, called by ANIM.ASM:4247
 * _ani_add_move with the move's identity bits.
 *
 * The source's own comment on it is "Adds first value each time! [why?]"
 * and the reason is worth recording, because it looks like a bug and is
 * not one to work around: the routine sets a3 to 1, and its "already
 * added" branch only skips re-setting a3 from a5 -- which `movk 1,a5` has
 * just made 1 as well. So COMBO_SIZE grows by exactly 1 every call
 * whether or not the move was already counted, and the COMBO_START
 * bookkeeping affects nothing but COMBO_START. Translated as written.
 *
 * Not translated: the royal-rumble branch that halves the amount and gives
 * it to player 0 (a mode this port has no equivalent of), and everything
 * from `#do_check` on -- picking a bar image out of WHICH_SIZE_BAR and
 * spawning the FLASH_COMBO process -- which is pure rendering. The
 * threshold itself IS translated, as combo_flash.
 */
void wm_arcade_add_to_combo_count(wm_arcade_actor_t *a, int32_t move_bits);

/* LIFEBAR.ASM's `movk 16,a6` super-combo threshold. */
#define WM_COMBO_SUPER_SIZE 16

/*
 * LIFEBAR.ASM:3687 SUBR DO_COMBO_MESS -- "Combo messages come from the
 * scripts", and it is the single most-called ANI_CODE routine in the
 * game: 193 uses across sixteen of the sequence files. It is what ends a
 * combo, and until now it was a named gap because the line it says needs
 * the announcer queue.
 *
 * The routine reads COMBO_COUNT, decrements it, and does nothing at all
 * below 2 -- no award, no sound, no line -- which is what the source's
 * own comment "We need to announce even the lowly combos / How can you
 * have a combo of 1 ?????" is arguing with. Above that it gives an
 * award, plays the humbug sound, queues the announcer's line, and
 * displays a message whose number is the count plus a display fudge.
 * COMBO_START is cleared either way, and COMBO_COUNT is NOT: that is
 * ANI_CLEAR_COMBO's job.
 */

/* `MOVI 0BAH,A0 / CALLA triple_sound`, with the source's own comment on
   the constant: "0BAH humbug!". */
#define WM_COMBO_MESS_SOUND 0x0BAu
/* `MOVI 01A4H,A0 / CALLA ADD_VOICE` -- and the source names it in a
   comment: INCREDIBLE COMBINATION MOVE! Note ADD_VOICE, not
   ADD_IF_SILENT: a finished combo interrupts whatever is being said. */
#define WM_VOICE_INCREDIBLE_COMBINATION 0x1A4u
/* `MOVI 1803H,A11` -- the message object's Z for BEGINOBJ, not an id;
   LIFEBAR.ASM:4130 uses the same value the same way. */
#define WM_COMBO_MESS_Z 0x1803u
/* `CMPI 2,A10 / JRLT NO_MESSAGE` and `cmpi 10,a10 / jrlt #award_reg`. */
#define WM_COMBO_MESS_MIN 2
#define WM_COMBO_MESS_ULTRA 10
/* `CMPI 5,A10 / JRLE ... / addk 2,a10`, with the source's own comment:
   "Add a couple for a better appearance!" */
#define WM_COMBO_MESS_FUDGE_OVER 5
#define WM_COMBO_MESS_FUDGE 2

/* What DO_COMBO_MESS reaches for. Any of it may be absent. */
typedef struct {
    void *sound_user;
    void (*sound)(void *user, uint16_t call);
    wm_announcer_state *announcer;
    /* AWARD.ASM round_award, through the JJXM.H RND_AWARD macro. */
    void *award_user;
    void (*round_award)(void *user, int player_num, unsigned award_index);
} wm_combo_mess_ctx;

typedef struct {
    /* The `btst B_COMBO_BROKEN` early-out: audited, then nothing. */
    bool combo_broken;
    /* COMBO_COUNT-1, the number everything below is decided on. */
    int32_t count;
    bool awarded;
    unsigned award_index;      /* GAME.EQU COMBOS_AWD / UCOMBOS_AWD */
    bool sound;                /* the humbug went out */
    bool voice;                /* the announcer's line was queued */
    /* A SPECIAL_MESSAGE would be created, and the number it would show
       after the display fudge. */
    bool message;
    int32_t message_count;
} wm_combo_mess_result;

/*
 * Runs it for `a`, which is the source's a13 -- the wrestler whose
 * animation called it. `ctx` may be NULL.
 *
 * NOT translated, and each for a reason: the AUD_COMBO operator audit
 * (there is no audit subsystem here), `is_there_one_already`'s
 * MESSAGE_FLAGS gate and the SPECIAL_MESSAGE process itself (nothing
 * here sets or clears those flags, so modelling half the gate would be
 * worse than saying so -- and the gate has a quirk worth recording: its
 * `NOT A1` makes it read the OTHER side's slot), the royal-rumble branch
 * that hands player 1's meter to player 0, and zero_combo_meter, which
 * is lifebar object drawing.
 */
wm_combo_mess_result wm_arcade_do_combo_mess(wm_arcade_actor_t *a,
                                             const wm_combo_mess_ctx *ctx);

#ifdef __cplusplus
}
#endif
#endif
