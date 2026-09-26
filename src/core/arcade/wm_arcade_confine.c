#include "wm/arcade/wm_arcade_confine.h"
#include "wm/arcade/wmania_ring_geometry.h"
#include "wm/arcade/wmania_ring_climb.h"

#include <string.h>

#define WM_CONFINE_MAX_ACTORS 8

/*
 * The climb checks live in wm/arcade/wmania_ring_climb.h against their
 * own view of a wrestler, so confine_wrestler has to hand them one.
 *
 * ONE FIELD INVERTS. PLYR.EQU's INRING is 1 for OUTSIDE -- the source's
 * own `move *a13(INRING),a0 / jrnz #outring` says so -- and the climb
 * module kept that polarity, while wm_arcade_actor_t::in_ring is the
 * ordinary boolean (1 = in the ring). Getting this backwards silently
 * turns every climb check inside out, so it is written once, here.
 */
typedef struct {
    WmRingClimbPlayer players[WM_CONFINE_MAX_ACTORS];
    size_t count;
    size_t self_index;
    bool have_self;
    uint32_t pcnt;
    wm_confine_result_t *out;
} climb_ctx;

static void climb_from_actor(WmRingClimbPlayer *p,
                             const wm_arcade_actor_t *a) {
    memset(p, 0, sizeof *p);
    p->active = a->active != 0;
    p->wrestler_num = (uint8_t)a->wrestler_num;
    p->player_num = (uint8_t)a->player_num;
    p->player_side = (int16_t)a->player_side;
    p->player_mode = (int16_t)a->player_mode;
    p->anim_mode = a->anim_mode;
    p->status_flags = (uint16_t)a->status_flags;
    p->inring = (int16_t)(a->in_ring ? 0 : 1);   /* inverted -- see above */
    p->climbing_thru = (int16_t)a->climbing_thru;
    p->getup_time = (int16_t)a->getup_time;
    p->x_int = (int16_t)a->x_int;
    p->z_int = (int16_t)a->z_int;
    p->z_fp16 = a->z_fixed;
    p->coll_x1 = (int16_t)a->hurt_box.x1;
    p->coll_x2 = (int16_t)a->hurt_box.x2;
    p->move_dir = (uint16_t)a->move_dir;
    p->stick_val_cur = a->stick_val_cur;
    p->but_val_cur = a->but_val_cur;
    p->facing_dir = (uint16_t)a->facing_dir;
    p->new_facing_dir = (uint16_t)a->new_facing_dir;
    p->climb_start = a->climb_start;
    p->climb_last = a->climb_last;
    p->animbase_label = a->anipc_program;
}

/*
 * The fields a climb check writes back onto the wrestler it ran on.
 *
 * PLYRMODE is one of them. The checks really do SETMODE: the two side
 * ones drop to MODE_NORMAL when they start the climb (WRESTLE2.ASM:590,
 * :715), climb_turnbuckle goes to MODE_CLIMBTURNBKL (:213), and all
 * three of the rotate-first paths SETMODE WAITANIM (:201, :579, :703). The climb module computes all of
 * that faithfully; this used to leave it in the module's own row and
 * copy only the other four fields back, so a wrestler who climbed in
 * while RUNNING stayed RUNNING and -- worse -- one parked in WAITANIM
 * was never parked at all.
 *
 * The two enums are the same numbers: WM_RING_MODE_* and WM_PMODE_* are
 * both transcribed from PLYR.EQU, so this is a copy and not a mapping.
 */
static void climb_to_actor(wm_arcade_actor_t *a, const WmRingClimbPlayer *p) {
    a->climbing_thru = p->climbing_thru;
    a->new_facing_dir = (int32_t)p->new_facing_dir;
    a->climb_start = p->climb_start;
    a->climb_last = p->climb_last;
    a->player_mode = (int32_t)p->player_mode;
}

static void climb_ctx_init(climb_ctx *c,
                           const wm_arcade_actor_t *self,
                           wm_arcade_actor_t *const *actors,
                           size_t actor_count,
                           uint32_t pcnt,
                           wm_confine_result_t *out) {
    size_t i;
    memset(c, 0, sizeof *c);
    c->pcnt = pcnt;
    c->out = out;
    if (!actors) return;
    for (i = 0; i < actor_count && c->count < WM_CONFINE_MAX_ACTORS; ++i) {
        if (!actors[i]) continue;
        if (actors[i] == self) {
            c->self_index = c->count;
            c->have_self = true;
        }
        climb_from_actor(&c->players[c->count], actors[i]);
        ++c->count;
    }
}

/*
 * Run one ck_climb_* and record what it started. The source calls these
 * with `calla` and ignores the return; what it really produces is a
 * change_anim1a on the wrestler, which this port reports upward rather
 * than performing -- confine_wrestler owns no animation system.
 *
 * The six checks do not share a signature, and the differences are real
 * rather than stylistic: the two side ones need the boundary X that the
 * clamp just computed, and ck_climb_out_side additionally needs a read
 * this port cannot honestly supply (see below).
 */
/*
 * Refresh the caller's own row from the actor before a check runs.
 *
 * This is not bookkeeping -- it is the difference between the check
 * firing and never firing. The source reads *a13(OBJ_ZPOS) live, and
 * every climb-out test is an EQUALITY against the boundary
 * (`z_fp16 != RING_BOT<<16` and friends), which only holds because the
 * clamp immediately above has just written exactly that value. A
 * snapshot taken before the clamp still holds the position the wrestler
 * walked in with, and misses by however far he overshot.
 */
static void climb_sync_self(climb_ctx *c, const wm_arcade_actor_t *a) {
    WmRingClimbPlayer *p = &c->players[c->self_index];
    p->x_int = (int16_t)a->x_int;
    p->z_int = (int16_t)a->z_int;
    p->z_fp16 = a->z_fixed;
    p->coll_x1 = (int16_t)a->hurt_box.x1;
    p->coll_x2 = (int16_t)a->hurt_box.x2;
    p->inring = (int16_t)(a->in_ring ? 0 : 1);
    /* Read live, like the position above: an earlier check in this same
       pass may already have moved him (climb_to_actor writes PLYRMODE
       back now), and every one of these routines gates on it. */
    p->player_mode = (int16_t)a->player_mode;
}

static void climb_record(climb_ctx *c, wm_arcade_actor_t *actor,
                         WmRingClimbResult r) {
    climb_to_actor(actor, &c->players[c->self_index]);
    if (!c->out) return;
    if (r.action == WM_RING_CLIMB_ACTION_SOURCE_QUIRK_INPUT_REQUIRED) {
        c->out->climb_needs_source_quirk = true;
        return;
    }
    if (r.action == WM_RING_CLIMB_ACTION_ROTATE_THEN_CONTINUE) {
        /* He is facing the wrong way. The source turns him first and
           defers the climb through CODE_ADDR + MODE WAITANIM; see
           wm_confine_result_t::climb_rotate_then. climb_to_actor above
           has already parked him in WAITANIM and, for the two side
           checks, set CLIMBING_THRU -- so the caller MUST carry this
           continuation, or he waits on an animation nobody started. */
        c->out->climb_rotate_then = r.continuation;
        c->out->climb_facing = r.target_facing;
        return;
    }
    if (r.action != WM_RING_CLIMB_ACTION_START_ANIMATION) return;
    c->out->climb_anim = r.source_animation_label;
    c->out->climb_facing = r.target_facing;
}

static void climb_out_top(climb_ctx *c, wm_arcade_actor_t *actor) {
    if (!c->have_self) return;
    climb_sync_self(c, actor);
    climb_record(c, actor,
                 wm_ring_ck_climb_out_top(&c->players[c->self_index],
                                          c->players, c->count, c->pcnt));
}

static void climb_out_bot(climb_ctx *c, wm_arcade_actor_t *actor) {
    if (!c->have_self) return;
    climb_sync_self(c, actor);
    climb_record(c, actor,
                 wm_ring_ck_climb_out_bot(&c->players[c->self_index],
                                          c->players, c->count, c->pcnt));
}

/*
 * ck_climb_out_side carries a shipped source quirk that
 * wm/arcade/wmania_ring_climb.h sets out in full: after
 * any_opp_outside, `move *a0(CLIMBING_THRU),a0` reads through a0, and
 * a0 is the post-incremented process_ptrs CURSOR rather than a wrestler
 * process. The value it lands on is whatever the next POINTER SLOT's
 * bits happen to be, which is not a field of anything and is not
 * reproducible without the arcade's own memory layout.
 *
 * So no read is supplied. The module then reports
 * SOURCE_QUIRK_INPUT_REQUIRED, and that is passed up as a fact instead
 * of being papered over with an invented "intended" behaviour -- which
 * means climbing out through the SIDE ropes does not happen in this
 * port, while out of the top and bottom does.
 */
static void climb_out_side(climb_ctx *c, wm_arcade_actor_t *actor,
                           int32_t rope_x) {
    if (!c->have_self) return;
    climb_sync_self(c, actor);
    climb_record(c, actor,
                 wm_ring_ck_climb_out_side(&c->players[c->self_index],
                                           c->players, c->count, c->pcnt,
                                           (int16_t)rope_x, NULL, NULL));
}

static void climb_in_top(climb_ctx *c, wm_arcade_actor_t *actor) {
    if (!c->have_self) return;
    climb_sync_self(c, actor);
    climb_record(c, actor,
                 wm_ring_ck_climb_in_top(&c->players[c->self_index],
                                         c->pcnt));
}

static void climb_in_bot(climb_ctx *c, wm_arcade_actor_t *actor) {
    if (!c->have_self) return;
    climb_sync_self(c, actor);
    climb_record(c, actor,
                 wm_ring_ck_climb_in_bot(&c->players[c->self_index],
                                         c->pcnt));
}

static void climb_in_side(climb_ctx *c, wm_arcade_actor_t *actor,
                          int32_t line_x) {
    if (!c->have_self) return;
    climb_sync_self(c, actor);
    climb_record(c, actor,
                 wm_ring_ck_climb_in_side(&c->players[c->self_index],
                                          c->pcnt, (int16_t)line_x));
}

/* ---------------------------------------------------------------- */
/* WRESTLE.ASM:3090-3425, the wrestler is INSIDE the ring.           */
/* ---------------------------------------------------------------- */
static int32_t confine_in_ring(wm_arcade_actor_t *actor, climb_ctx *c) {
    int32_t bits = 0;
    int32_t z = actor->z_int;
    int32_t collx1, collx2, rope_x;

    /* WRESTLE.ASM:3091-3129: top/bottom rope Z bounds. */
    if (z <= WM_RING_TOP) {
        if (z < WM_RING_TOP) {
            actor->z_int = WM_RING_TOP;
            actor->z_fixed = (int32_t)WM_RING_TOP << 16;
            actor->z_bound = WM_RING_TOP;
            /* `calla ck_climb_out_top` -- only on the clamp, because
               `jreq #no_u` jumps past it when he is exactly on the line. */
            climb_out_top(c, actor);
        }
        bits |= WM_MOVE_UP;
    } else if (z >= WM_RING_BOT) {
        if (z > WM_RING_BOT) {
            actor->z_int = WM_RING_BOT;
            actor->z_fixed = (int32_t)WM_RING_BOT << 16;
            actor->z_bound = WM_RING_BOT;
            climb_out_bot(c, actor);
        }
        bits |= WM_MOVE_DOWN;
    } else {
        /* `#zd_ok clr a14 / move a14,*a13(Z_BOUND)`. */
        actor->z_bound = 0;
    }

    /* WRESTLE.ASM:3132-3274: left rope, using OBJ_COLLX1 (hurt_box.x1). */
    collx1 = actor->hurt_box.x1;
    rope_x = wm_ring_calc_line_x(
        wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_ROPE), actor->z_int);
    if (collx1 <= rope_x) {
        if (collx1 < rope_x) {
            int32_t delta = rope_x - collx1;
            actor->x_int += delta;
            actor->x_fixed += delta << 16;
            /* `#not`: `move *a13(OBJ_XPOSINT),a14 / sub a5,a14 /
               add a14,a0 / move a0,*a13(X_BOUND)` -- the line's X at
               this Z, offset by how far the collision box sits from
               the wrestler's own X. */
            actor->x_bound = actor->x_int;
            /* `move *a13(INRING),a0 / jrnz #no_l` guards it, and we are
               inside by construction here. */
            climb_out_side(c, actor, rope_x);
        }
        return bits | WM_MOVE_LEFT;
    }

    /* WRESTLE.ASM:3277-3421: right rope, using OBJ_COLLX2 (hurt_box.x2). */
    collx2 = actor->hurt_box.x2;
    rope_x = wm_ring_calc_line_x(
        wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_ROPE), actor->z_int);
    if (collx2 >= rope_x) {
        if (collx2 > rope_x) {
            int32_t delta = collx2 - rope_x;
            actor->x_int -= delta;
            actor->x_fixed -= delta << 16;
            actor->x_bound = actor->x_int;
            climb_out_side(c, actor, rope_x);
        }
        return bits | WM_MOVE_RIGHT;
    }

    return bits;
}

/*
 * Clamp X and drag an attached partner the same distance -- "if we're
 * attached, move our opponent too", which the source says twice and
 * means slightly differently each time.
 *
 * `bound_partner` is that difference, and it is not a tidy-up: the FENCE
 * clamp writes X_BOUND onto the partner as well (`move a1,*a5(X_BOUND)`)
 * while the MAT-EDGE clamp moves only his OBJ_XPOSINT and leaves his
 * X_BOUND alone.
 */
static void confine_shift_x(wm_arcade_actor_t *actor, int32_t delta,
                            int32_t bound, bool bound_partner) {
    actor->x_int += delta;
    actor->x_fixed += delta << 16;
    actor->x_bound = bound;
    if (actor->attach_proc) {
        actor->attach_proc->x_int += delta;
        actor->attach_proc->x_fixed += delta << 16;
        if (bound_partner) actor->attach_proc->x_bound = bound;
    }
}

static void confine_set_z(wm_arcade_actor_t *actor, int32_t z) {
    actor->z_int = z;
    actor->z_bound = z;
    actor->z_fixed = z << 16;
}

/* ---------------------------------------------------------------- */
/* WRESTLE.ASM:3429-3655, the wrestler is OUTSIDE the ring.          */
/* ---------------------------------------------------------------- */
static int32_t confine_out_ring(wm_arcade_actor_t *actor, climb_ctx *c) {
    int32_t bits = 0;
    int32_t z = actor->z_int;
    const WmRingBoundarySeed *seed;
    int32_t line_x, xov, zov, a4;

    /*
     * `#outring`: the same shape as the in-ring Z test against
     * ARENA_TOP/ARENA_BOT instead of the ropes -- with one difference
     * worth noticing. The in-ring branch CLEARS Z_BOUND when the
     * wrestler is comfortably between the ropes (`#zd_ok`); this one
     * has no such clear, so Z_BOUND outside the ring keeps whatever the
     * last clamp left in it.
     */
    if (z <= (int32_t)WM_ARENA_TOP) {
        if (z < (int32_t)WM_ARENA_TOP) confine_set_z(actor, WM_ARENA_TOP);
        bits |= WM_MOVE_UP;
    } else if (z >= (int32_t)WM_ARENA_BOT) {
        if (z > (int32_t)WM_ARENA_BOT) confine_set_z(actor, WM_ARENA_BOT);
        bits |= WM_MOVE_DOWN;
    }

    /*
     * `#check_x2`: the fences. `move *a6,a0` reads the seed's FIRST
     * word -- top_x -- as a cheap "are we even in the ballpark" test
     * before paying for calc_line_x, exactly as the rope check does.
     */
    seed = wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_FENCE);
    if (actor->hurt_box.x1 <= (int32_t)seed->top_x) {
        line_x = wm_ring_calc_line_x(seed, actor->z_int);
        if (actor->hurt_box.x1 <= line_x) {
            if (actor->hurt_box.x1 < line_x)
                confine_shift_x(actor, line_x - actor->hurt_box.x1, line_x, true);
            bits |= WM_MOVE_LEFT;
            goto cont_x;
        }
    }
    seed = wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_FENCE);
    if (actor->hurt_box.x2 >= (int32_t)seed->top_x) {
        line_x = wm_ring_calc_line_x(seed, actor->z_int);
        if (actor->hurt_box.x2 >= line_x) {
            if (actor->hurt_box.x2 > line_x)
                confine_shift_x(actor, line_x - actor->hurt_box.x2, line_x, true);
            bits |= WM_MOVE_RIGHT;
        }
    }

cont_x:
    /*
     * `#cont_x`: the mat edge, and this is the interesting half. He is
     * outside the ring and walking into the apron; the routine works
     * out whether he is overlapping it more in X or more in Z and acts
     * on the smaller correction -- push him back along X, or stand him
     * on the mat edge along Z and offer him the climb back in.
     *
     * `calc_line_x` returning zero is the source's own out-of-range
     * answer, and `jrz #done2` takes it as "nowhere near the mat".
     */
    seed = wm_ring_boundary_seed(
        actor->x_int > (int32_t)WM_RING_X_CENTER
            ? WM_RING_BOUNDARY_RIGHT_MAT2 : WM_RING_BOUNDARY_LEFT_MAT2);
    line_x = wm_ring_calc_line_x(seed, actor->z_int);
    if (line_x == 0) return bits;

    if (actor->x_int > (int32_t)WM_RING_X_CENTER)
        xov = line_x - actor->hurt_box.x1;      /* `#right_side` */
    else
        xov = actor->hurt_box.x2 - line_x;      /* left side */
    if (xov < 0) return bits;                   /* `jrn #done2` */

    a4 = actor->z_int;
    if (a4 > (int32_t)WM_RING_Z_CENTER) {
        /* `#bot_left` / `#bot_right`: overlap measured off bottom_z. */
        zov = (int32_t)seed->bottom_z - a4;
        if (zov <= xov) {
            confine_set_z(actor, a4 + zov);
            bits |= WM_MOVE_UP;
            climb_in_bot(c, actor);
            return bits;
        }
    } else {
        /* `top left` / `top right`: overlap measured off top_z. */
        zov = a4 - (int32_t)seed->top_z;
        if (zov <= xov) {
            confine_set_z(actor, a4 - zov);
            bits |= WM_MOVE_DOWN;
            climb_in_top(c, actor);
            return bits;
        }
    }

    /*
     * `#no_r3` / `#no_l3`: X is the smaller correction, so push him off
     * the mat that way instead and let ck_climb_in_side have a look.
     */
    if (actor->x_int > (int32_t)WM_RING_X_CENTER) {
        confine_shift_x(actor, xov, actor->x_int + xov, false);
        bits |= WM_MOVE_LEFT;
    } else {
        confine_shift_x(actor, -xov, actor->x_int - xov, false);
        bits |= WM_MOVE_RIGHT;
    }
    climb_in_side(c, actor, line_x);
    return bits;
}

/*
 * WRESTLE.ASM:3656-3730, what `#done2` does AFTER storing CAN_MOVE_DIR.
 * Only the out-of-ring branch reaches it; the in-ring one returns at
 * `#done` without any of this.
 */
static void confine_out_ring_tail(wm_arcade_actor_t *actor, int32_t bits,
                                  uint32_t pcnt, wm_confine_result_t *out) {
    int32_t into_gate;

    if (actor->player_mode == WM_PMODE_DEAD) {
        /*
         * `#dead`: "if we're a zombie, hitting the edge is our cue to
         * transform". All three have to hold -- he must have actually
         * been stopped in X this pass, he must be a zombie, and
         * mode_dead's run-to-the-side must already have set CAN_XFORM.
         * This is the faster of the two ways out of zombie mode; the
         * other is that routine's own ten-second timeout.
         */
        if (!(bits & (WM_MOVE_LEFT | WM_MOVE_RIGHT))) return;
        if (!(actor->status_flags & WM_STATUS_ZOMBIE)) return;
        if (!(actor->status_flags & WM_STATUS_CAN_XFORM)) return;
        if (out) out->zombie_transform = true;
        return;
    }

    /* `cmpi MODE_RUNNING,a0 / jrne just_ignore_me`. */
    if (actor->player_mode != WM_PMODE_RUNNING) return;

    /*
     * `movi [3,0],a2 / btst MOVE_LEFT_BIT,a7 / jrnz we_going_left /
     * neg a2 / btst MOVE_RIGHT_BIT,a7 / jrz just_ignore_me` -- bounce
     * back out of the gate he hit, so a left-side stop bounces him
     * right (+3) and a right-side stop bounces him left (-3).
     */
    if (bits & WM_MOVE_LEFT) into_gate = 3 << 16;
    else if (bits & WM_MOVE_RIGHT) into_gate = -(3 << 16);
    else return;

    /*
     * "It's possible, however, that we're right up against a gate and
     * have just started running in the opposite direction, in which
     * case we shouldn't crash or anything. blow this off if we're
     * running AWAY from the gate we've hit." The test is CAN_MOVE_DIR
     * AND FACING_DIR, masked to left/right -- he is only crashing if he
     * is facing the way he is blocked.
     */
    if (((actor->can_move_dir & actor->facing_dir) &
         (int32_t)(WM_MOVE_LEFT | WM_MOVE_RIGHT)) == 0) return;

    actor->x_vel = into_gate;
    actor->player_mode = WM_PMODE_NORMAL;
    actor->y_vel = 3 << 16;              /* `movi [3,0],a0 / OBJ_YVEL` */
    actor->run_time = 0;

    /*
     * "if we've hit the gate in the last three seconds, fall back
     * instead" -- `move @PCNT,a14,L / move *a13(HIT_GATE_TIME),a0,L /
     * sub a0,a14 / cmpi TSEC*3,a14 / jrge #bnc`. Note the sense: a
     * difference of three seconds or MORE bounces, so it is a recent
     * hit that makes him fall back.
     */
    if (out) {
        out->gate_crash = true;
        out->gate_fall_back =
            (int32_t)(pcnt - actor->hit_gate_time) < WM_CONFINE_GATE_REHIT;
    }

    /* `move @PCNT,a14,L / move a14,*a13(HIT_GATE_TIME),L`, and
       `movi -D_GATE_CRASH,a0 / calla adjust_health`. */
    actor->hit_gate_time = pcnt;
    actor->life -= WM_CONFINE_GATE_DAMAGE;
    if (actor->life < 0) actor->life = 0;
}

void wm_arcade_confine_wrestler_ex(wm_arcade_actor_t *actor,
                                   wm_arcade_actor_t *const *actors,
                                   size_t actor_count,
                                   uint32_t pcnt,
                                   wm_confine_result_t *out) {
    climb_ctx c;
    int32_t bits;
    bool outside;

    if (out) memset(out, 0, sizeof *out);
    if (!actor) return;

    /* WRESTLE.ASM:3078-3084 #no_confine early-out: MODE_NOCONFINE or
       PLYRMODE==ATTACHED both skip straight to storing CAN_MOVE_DIR=0
       (the a7 accumulator's untouched initial value). */
    if ((actor->anim_mode & WM_MODE_NOCONFINE) ||
        actor->player_mode == WM_PMODE_ATTACHED) {
        actor->can_move_dir = 0;
        return;
    }

    climb_ctx_init(&c, actor, actors, actor_count, pcnt, out);

    /* `move *a13(INRING),a0 / jrnz #outring` -- and PLYR.EQU's INRING
       is 1 for OUTSIDE, which this port's field inverts. */
    outside = !actor->in_ring;
    bits = outside ? confine_out_ring(actor, &c) : confine_in_ring(actor, &c);
    actor->can_move_dir = bits;

    if (outside) confine_out_ring_tail(actor, bits, pcnt, out);
}

void wm_arcade_confine_wrestler(wm_arcade_actor_t *actor) {
    wm_arcade_confine_wrestler_ex(actor, NULL, 0, 0u, NULL);
}

/*
 * WRESTLE.ASM:6163 final_confine. See the header for why it only
 * touches the attached ones.
 */
void wm_arcade_final_confine(wm_arcade_actor_t *const *actors,
                             size_t actor_count)
{
    size_t i;

    if (!actors) return;

    for (i = 0; i < actor_count; ++i) {
        wm_arcade_actor_t *a = actors[i];
        if (!a) continue;                       /* `jrz #inactive` */
        if (!a->attach_proc) continue;          /* `jrz #no_attach` */

        /*
         * `calla set_collision_boxes`. The hurt box depends on the
         * frame AND the position, and the position is what moved, so
         * it really does have to be redone rather than left alone.
         */
        if (a->hurt_frame_valid) wm_arcade_set_hurt_box(a, &a->hurt_frame);
        wm_arcade_set_attack_box(a);

        wm_arcade_confine_wrestler(a);
    }
}

/*
 * mode_waitanim's `call a0`, for the three climb continuations. See
 * wm/arcade/wm_arcade_confine.h.
 *
 * This is deliberately in this file rather than beside the climb module
 * itself: climb_from_actor/climb_to_actor are the one place that knows
 * how a wm_arcade_actor_t maps onto a WmRingClimbPlayer -- INRING's
 * inverted polarity above included -- and the continuation has to go
 * through exactly that mapping or it would be a second, drifting copy
 * of it.
 */
const char *wm_arcade_climb_continue(wm_arcade_actor_t *actor,
                                     WmRingClimbContinuation cont) {
    WmRingClimbPlayer p;
    WmRingClimbResult r;

    if (!actor) return NULL;
    if (cont == WM_RING_CLIMB_CONT_NONE) return NULL;

    climb_from_actor(&p, actor);
    r = wm_ring_climb_continue(&p, cont);
    climb_to_actor(actor, &p);

    /* SOURCE_NULL_ANIMATION is a real 0 in the source's own table, not
       a missing translation, and the source would have branched to it.
       PLYRMODE has already been applied, so he still leaves WAITANIM. */
    if (r.action != WM_RING_CLIMB_ACTION_START_ANIMATION) return NULL;
    return r.source_animation_label;
}

/*
 * climb_turnbuckle (WRESTLE2.ASM:103). See wm/arcade/wm_arcade_confine.h.
 *
 * The roster is passed whole because the check's own #climbit loop
 * sweeps process_ptrs for somebody already on or climbing the SAME
 * turnbuckle -- the left one if the climber is left of RING_X_CENTER,
 * the right one otherwise -- and refuses if it finds him.
 */
wm_climb_turnbuckle_result_t wm_arcade_climb_turnbuckle(
    wm_arcade_actor_t *actor,
    wm_arcade_actor_t *const *actors,
    size_t actor_count) {
    wm_climb_turnbuckle_result_t out;
    climb_ctx c;
    WmRingClimbResult r;
    int32_t rope_x;

    memset(&out, 0, sizeof out);
    if (!actor) return out;

    climb_ctx_init(&c, actor, actors, actor_count, 0u, NULL);
    if (!c.have_self) {
        /* Called without the actor in the list: run him alone rather
           than against a roster he is not in, which would read the
           wrong row as "self". */
        climb_from_actor(&c.players[0], actor);
        c.count = 1u;
        c.self_index = 0u;
    }

    /* `calla get_rope_x` -- the same value at both comparison sites. */
    rope_x = wm_ring_get_rope_x(actor->x_int, actor->z_int);

    r = wm_ring_climb_turnbuckle(&c.players[c.self_index],
                                 c.players, c.count, (int16_t)rope_x);
    if (r.action == WM_RING_CLIMB_ACTION_NONE) return out;

    /* The check glitches a near miss onto RING_TOP itself, so the Z it
       wrote has to come back with the rest. */
    actor->z_int = c.players[c.self_index].z_int;
    actor->z_fixed = c.players[c.self_index].z_fp16;
    climb_to_actor(actor, &c.players[c.self_index]);

    out.handled = true;                /* `setc` */
    out.facing = r.target_facing;
    if (r.action == WM_RING_CLIMB_ACTION_ROTATE_THEN_CONTINUE)
        out.rotate_then = r.continuation;
    else if (r.action == WM_RING_CLIMB_ACTION_START_ANIMATION)
        out.anim = r.source_animation_label;
    return out;
}
