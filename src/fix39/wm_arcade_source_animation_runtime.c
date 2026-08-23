#include "wm_arcade_source_animation_runtime.h"
#include <string.h>

void wm_source_anim_runtime_init(wm_source_anim_runtime_t *s)
{
    if (s) memset(s, 0, sizeof(*s));
}

static void load_frame(wm_source_anim_runtime_t *s, const wm_arcade_actor_t *a)
{
    const wm_source_anim_frame_t *f;
    uint32_t ticks;
    uint16_t speed;
    if (!s || !s->def || !s->def->frame_count) return;
    if (s->frame_index >= s->def->frame_count) s->frame_index = 0;
    f = &s->def->frames[s->frame_index];
    s->current_frame = f->source_frame;
    speed = (a && a->ani_speed) ? a->ani_speed : 0x0100u;
    /* ANIM.ASM: OANICNT = (script_ticks * ANI_SPEED) >> 8.  Hyper speed is
       a separate global service and remains disabled in the ordinary attract match. */
    ticks = ((uint32_t)f->ticks * (uint32_t)speed) >> 8;
    s->ticks_left = (uint16_t)(ticks ? ticks : 1u);
}

static void apply_source_prefix(const wm_source_anim_init_t *i, wm_arcade_actor_t *a)
{
    const uint32_t sf_clear = WM_STATUS_SCROLL_CTRL | WM_STATUS_DEAD_ANIM |
        WM_STATUS_DID_RAISEARM | WM_STATUS_KOD | WM_STATUS_COMBO_BROKEN |
        WM_STATUS_PUSH;
    uint16_t requested;
    if (!i || !a) return;
    if (i->mask & WM_SRC_ANIM_INIT_ANIM_MODE) {
        a->anim_mode = i->anim_mode;
        a->status_flags &= ~sf_clear; /* ANIM.ASM _ani_setmode */
        if (a->ptime) a->ptime = 1;
    }
    if (i->mask & WM_SRC_ANIM_INIT_PLAYER_MODE) {
        a->climbing_thru = 0;
        requested = i->player_mode;
        if (a->player_mode != WM_PMODE_DEAD) {
            if (requested == WM_PMODE_HEADHOLD && a->delay_meter < 6*60)
                a->delay_meter = 9*60;
            a->player_mode = requested;
        }
    }
    if (i->mask & WM_SRC_ANIM_INIT_FRICTION) {
        a->obj_friction = i->friction;
        a->anim_mode |= WM_ARCADE_MODE_FRICTION;
    }
    if (i->mask & WM_SRC_ANIM_INIT_SPEED) a->ani_speed = i->speed;
    if (i->mask & WM_SRC_ANIM_INIT_XVEL) a->x_vel = i->xvel;
    if (i->mask & WM_SRC_ANIM_INIT_YVEL) a->y_vel = i->yvel;
    if (i->mask & WM_SRC_ANIM_INIT_ZVEL) a->z_vel = i->zvel;
    if (i->mask & WM_SRC_ANIM_INIT_GRAVITY) a->gravity = i->gravity;
}

bool wm_source_anim_runtime_change(wm_source_anim_runtime_t *s, wm_arcade_actor_t *a,
                                   uint8_t roster_id, const char *label)
{
    const wm_source_anim_def_t *d;
    if (!s || !a || !label) return false;
    d = wm_source_anim_find(roster_id, label);
    if (!d || !d->frame_count) return false;
    s->def = d;
    s->frame_index = 0u;
    apply_source_prefix(&d->init, a);
    a->anim_mode &= (uint16_t)~WM_ARCADE_MODE_END;
    load_frame(s, a);
    return true;
}

void wm_source_anim_runtime_tick(wm_source_anim_runtime_t *s, wm_arcade_actor_t *a)
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
}

const char *wm_source_anim_runtime_frame(const wm_source_anim_runtime_t *s)
{ return s ? s->current_frame : 0; }
const char *wm_source_anim_runtime_label(const wm_source_anim_runtime_t *s)
{ return (s && s->def) ? s->def->label : 0; }
