/* LIFEBAR.ASM:750 ADD_TO_COMBO_COUNT -- see wm/arcade/wm_arcade_combo.h. */
#include "wm/arcade/wm_arcade_combo.h"
#include "wm/award.h"

#include <string.h>

void wm_arcade_add_to_combo_count(wm_arcade_actor_t *a, int32_t move_bits) {
    if (!a) return;

    /* `AND a1,a2 / JRNZ ALREADY_ADDED_ONCE` -- the branch is real, but it
       changes only whether COMBO_START is re-OR'd; see the header. */
    if ((a->combo_start & move_bits) == 0)
        a->combo_start |= move_bits;

    ++a->combo_size;
    if (a->combo_size >= WM_COMBO_SUPER_SIZE) a->combo_flash = 1;
}

/*
 * LIFEBAR.ASM:3687 DO_COMBO_MESS -- see wm/arcade/wm_arcade_combo.h.
 *
 * Read in the source's own order; every branch below is one of its own.
 */
wm_combo_mess_result wm_arcade_do_combo_mess(wm_arcade_actor_t *a,
                                             const wm_combo_mess_ctx *ctx) {
    wm_combo_mess_result r;

    memset(&r, 0, sizeof(r));
    if (!a) return r;

    /*
     * `btst B_COMBO_BROKEN,a14 / jrnz #rets`, and the source explains
     * itself: "if our combo_broken bit is set, blow out of here
     * altogether. We audit it, but we don't adjust the bar or display any
     * kind of message." Note it returns without clearing COMBO_START.
     */
    if (a->status_flags & WM_STATUS_COMBO_BROKEN) {
        r.combo_broken = true;
        return r;
    }

    /* `MOVE *A13(COMBO_COUNT),A10 / DEC A10` -- everything below is
       decided on the count MINUS ONE, not the count. */
    r.count = a->combo_count - 1;

    /* `CMPI 2,A10 / JRLT NO_MESSAGE` skips the award and both sounds as
       well as the message: a two-hit combo is silent. */
    if (r.count >= WM_COMBO_MESS_MIN) {
        r.awarded = true;
        r.award_index = (r.count < WM_COMBO_MESS_ULTRA)
                            ? WM_AWARD_COMBOS : WM_AWARD_ULTRA_COMBOS;
        if (ctx && ctx->round_award)
            ctx->round_award(ctx->award_user, (int)a->player_num,
                             r.award_index);

        if (ctx && ctx->sound) {
            ctx->sound(ctx->sound_user, WM_COMBO_MESS_SOUND);
            r.sound = true;
        }
        /* ADD_VOICE, not ADD_IF_SILENT: this one talks over the
           announcer rather than waiting for him. */
        if (ctx && ctx->announcer &&
            wm_announcer_add(ctx->announcer,
                             WM_VOICE_INCREDIBLE_COMBINATION))
            r.voice = true;

        r.message_count = r.count;
        if (r.message_count > WM_COMBO_MESS_FUDGE_OVER)
            r.message_count += WM_COMBO_MESS_FUDGE;
        /* The MESSAGE_FLAGS gate and SPECIAL_MESSAGE itself are not
           translated -- see the header -- so this reports that a message
           is due rather than pretending to have shown one. */
        r.message = true;
    }

    /* `CLR A0 / MOVE A0,*A13(COMBO_START)`, reached from both paths.
       COMBO_COUNT is deliberately left alone: ANI_CLEAR_COMBO clears it. */
    a->combo_start = 0;
    return r;
}
