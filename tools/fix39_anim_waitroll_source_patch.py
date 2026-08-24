#!/usr/bin/env python3
from pathlib import Path
import re,sys
repo=Path(sys.argv[1])
p=repo/'src/fix39/wm_arcade_source_animation_runtime.c'
s=p.read_text()
pat=re.compile(r'void wm_source_anim_runtime_tick\(wm_source_anim_runtime_t \*s, wm_arcade_actor_t \*a\)\n\{.*?\n\}',re.S)
new=r'''void wm_source_anim_runtime_tick(wm_source_anim_runtime_t *s, wm_arcade_actor_t *a)
{
    const wm_source_anim_def_t *d;
    const char *next;
    if (!s || !a || !s->def || !s->def->frame_count) return;
    if (a->anim_mode & WM_ARCADE_MODE_END) return;
    if (s->ticks_left > 1u) { --s->ticks_left; return; }
    if ((uint16_t)(s->frame_index + 1u) < s->def->frame_count) {
        ++s->frame_index;
        load_frame(s, a);
        return;
    }
    if (s->def->repeat) {
        s->frame_index = 0u;
        load_frame(s, a);
        return;
    }

    d = s->def;
    next = d->next_label;

    /* ANIM.ASM::_ani_waitroll (84): knockdown scripts hold their final lying
       frame while GETUP_TIME / IMMOBILIZE_TIME are nonzero, then continue to
       the script's ANI_CHANGEANIM get-up sequence.  The old visual-only
       catalog discarded both commands and therefore left MODE_ONGROUND actors
       permanently on the mat. */
    if (d->control_flags & WM_SRC_ANIM_CTRL_WAITROLL) {
        if (a->player_mode != WM_PMODE_DEAD)
            a->player_mode = WM_PMODE_ONGROUND;
        if (a->immobilize_time != 0 || a->getup_time != 0) {
            s->ticks_left = 1u;
            return;
        }
        a->stars_flag = 0;
        /* do_roll is a separate WRESTLE2.ASM service.  Do not invent a roll
           here: with no live roll request, _ani_waitroll falls straight
           through to the following ANI_CHANGEANIM. */
    }

    /* ANIM.ASM::_ani_getup_wait (53) uses the same GETUP_TIME hold contract. */
    if ((d->control_flags & WM_SRC_ANIM_CTRL_GETUP_WAIT) && a->getup_time != 0) {
        s->ticks_left = 1u;
        return;
    }

    if (next && *next) {
        (void)wm_source_anim_runtime_change(s, a, (uint8_t)a->wrestler_num, next);
        return;
    }

    s->ticks_left = 1u;
    a->anim_mode |= WM_ARCADE_MODE_END;
}'''
s2,n=pat.subn(new,s,1)
if n!=1: raise SystemExit('source animation tick function anchor missing')
p.write_text(s2)
print('Combat2CD ANIM.ASM WAITROLL/GETUP_WAIT runtime patch applied')
