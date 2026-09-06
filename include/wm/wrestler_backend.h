#ifndef WM_WRESTLER_BACKEND_H
#define WM_WRESTLER_BACKEND_H

#include <stdbool.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_roster.h"
#include "wm/arcade/wm_arcade_razor.h"
#include "wm/anim_program.h"
#include "wm/movement.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The shared, wrestler-agnostic half of a wrestler backend, for the seven
 * wrestlers that do not (yet) have one of their own.
 *
 * wm/bret_backend.h is Bret's full backend: it resolves his animation ids
 * to real wm_visual_sequence frame data, drives two visual tracks from it,
 * fires each attack's hand-traced ATTACK_ON/OFF window, and derives a real
 * per-frame hurt_box. None of that exists for the other seven yet -- their
 * frame data has not been extracted and their attack windows have not been
 * traced -- so this deliberately supplies only the parts that are genuinely
 * shared source services and need no per-wrestler artwork or hand-tracing:
 *
 *   - execute_walk: WRESTLE.ASM's own execute_walk/set_velocities
 *     (wm/movement.h), driven by that wrestler's OWN real velocity table
 *     (see wm_wrestler_velocity_table).
 *   - adjust_health: the one real LIFEBAR.ASM::adjust_health translation
 *     (wm/arcade/wm_arcade_lifebar.h), same function Bret's own self-death
 *     path and the REACT1.ASM hit path already share.
 *   - mode_dead: DOINK.ASM's shared mode_dead (wm/arcade/wm_arcade_mode_dead.h),
 *     which every wrestler's own mode_table[9] points at.
 *   - check_combo_go: LIFEBAR.ASM's combo-meter gate, which this port's own
 *     established finding (wm_arcade_mode_dead.h) proves always reports
 *     "not lit" here.
 *
 * Every animation/sound seam is left NULL on purpose. The per-wrestler
 * dispatchers (wm_arcade_move_taker/yoko/shawn/bam/doink/lex, and Razor's
 * own typed-id variant) all null-check these, so a wrestler wired through
 * here runs its real, complete decision logic -- mode dispatch, attack
 * selection, blocks, movement, secret/charge handling -- and genuinely
 * moves, but is not drawn and has no attack/hurt box, exactly the boundary
 * described above. It invents nothing to fill that in.
 */
typedef struct wm_wrestler_backend_actor {
    wm_arcade_actor_t *opponent;
    uint32_t pcnt;
    bool attract_mode;
    int32_t wrestler_num;
    /*
     * The animation this wrestler is playing, as the ANIM.ASM program it
     * really is (wm/anim_program.h).
     *
     * This is what closed the gap the comment above describes. These six
     * dispatchers were always calling change_anim_label with the source's
     * own label -- "und_2_punch_anim", "dnk_4_kick_anim" -- and the program
     * registry is keyed on exactly that, so nothing had to be invented to
     * join them: the seam was already the right shape and had nothing
     * behind it. Now a label resolves to that routine's real op stream, and
     * the wrestler plays its real frames, fires its real attack boxes and
     * takes its real branches, the same way Bret does.
     */
    wm_anim_exec prog;
    wm_anim_env anim_env;
    /* The label most recently selected, so a repeated call with the same
       one continues rather than restarting -- the source's own
       "already playing this" behaviour. */
    const char *current_label;
} wm_wrestler_backend_actor;

/*
 * Each wrestler's own xxx_velocity_table (BRET.ASM:2848 hrt_, RAZOR.ASM:2586
 * rzr_, TAKER.ASM:3145 und_, YOKO.ASM:2662 yok_, SHAWN.ASM:3203 shn_,
 * BAM.ASM:2810 bam_, DNK.ASM:2794 dnk_, LEX.ASM:2619 lex_), all built from
 * that file's own #VEL/#DVEL. Seven of the eight share #VEL=3a000h /
 * #DVEL=31000h; Doink is the one genuine outlier at 30000h / 21f0eh, i.e.
 * really is a little slower than everyone else.
 */
const wm_move_velocity_entry *wm_wrestler_velocity_table(int32_t wrestler_num);

/*
 * The one piece of the block animation these wrestlers genuinely need but
 * cannot get from an animation stream they don't have extracted.
 *
 * MODE_BLOCK is not set by any wrestler's own .ASM -- it is set by the
 * block *animation*, e.g. HRTSEQ4.ASM:104 hrt_4_block_anim and
 * UNDSEQ2.ASM:3643 und_4_block_anim, which both read:
 *
 *     ANI_SETPLYRMODE,MODE_BLOCK      <- enter BLOCK
 *     ... frames ...
 *     ANI_WAITRELEASE,PLAYER_BLOCK_BIT <- hold here while BLOCK is held
 *     ... frames ...
 *     ANI_SETPLYRMODE,MODE_NORMAL     <- leave BLOCK once it is released
 *     ANI_END
 *
 * The per-wrestler modules translate the *entry* half directly (their own
 * do_block does setmode(BLOCK), standing in for ANI_SETPLYRMODE), but the
 * *exit* half lives only in the animation. mode_block's own release check
 * is gated behind BUT_VAL_DOWN carrying a newly-pressed button, exactly
 * like the source, so without the animation's ungated WAITRELEASE a
 * wrestler that simply stops holding block never leaves MODE_BLOCK at all.
 *
 * This applies that one real ungated transition, and nothing else about
 * the animation (no frames, no timing, no attack/hurt box). Call it once
 * per tick for a wrestler running on this backend.
 */
void wm_wrestler_backend_tick(wm_wrestler_backend_actor *state,
                              wm_arcade_actor_t *actor);

/* Label-based callback set (TAKER/YOKO/SHAWN/BAM/DOINK/LEX). */
wm_arcade_roster_callbacks_t wm_wrestler_roster_callbacks(wm_wrestler_backend_actor *state);

/* Razor's own typed-anim-id callback set, same shared bodies. */
wm_arcade_razor_callbacks_t wm_wrestler_razor_callbacks(wm_wrestler_backend_actor *state);

#ifdef __cplusplus
}
#endif

#endif
