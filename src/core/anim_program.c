#include "wm/anim_program.h"
#include "wm/arcade/wm_arcade_veladd.h"
#include "wm/arcade/wm_arcade_roll.h"
#include "wm/arcade/wm_arcade_combo.h"
#include "wm/arcade/wmania_ring_geometry.h"
#include "wm/arcade/wmania_rope_command.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wm_arcade_anim_combat.h"
#include "wm/arcade/wm_arcade_butcount.h"
#include "wm/frame_geometry.h"
#include "wm/arcade/wm_arcade_lifebar.h"

#include <string.h>

/* ANIM.ASM:1626 _ani_set_xvel / :1829 _ani_set_zvel: absolute, or negated
   when the direction bit the AM_* mode names is clear. */
static int32_t directional(const wm_arcade_actor_t *actor, int32_t value,
                           uint8_t mode, uint16_t bit) {
    uint16_t dir;
    switch (mode) {
        case 1: dir = (uint16_t)actor->facing_dir; break;
        case 2: dir = (uint16_t)actor->hit_side; break;
        case 3: dir = (uint16_t)actor->new_facing_dir; break;
        default: return value;
    }
    return (dir & bit) ? value : -value;
}

static void run_command(const wm_anim_op *o, wm_arcade_actor_t *actor,
                        uint16_t round_tickcount, const wm_anim_env *env,
                        const char *source_file) {
    switch (o->op) {
        case WM_AOP_SETMODE:
            /* ANI_SETMODE is absolute -- it replaces ANIMODE rather than
               OR-ing, which is how a trailing MODE_NORMAL clears everything
               the header set. */
            actor->anim_mode = (uint16_t)o->a;
            break;
        case WM_AOP_SETPLYRMODE:
            if (actor->player_mode != WM_PMODE_DEAD)
                actor->player_mode = (uint16_t)o->a;
            break;
        case WM_AOP_SETFACING:
            actor->facing_dir = actor->new_facing_dir;
            break;
        case WM_AOP_XFLIP:
            actor->obj_control = (uint16_t)(actor->obj_control ^ WM_OBJ_FLIPH);
            break;
        case WM_AOP_ZEROVELS:
            actor->x_vel = 0; actor->y_vel = 0; actor->z_vel = 0;
            break;
        case WM_AOP_ZERO_XZVELS:
            actor->x_vel = 0; actor->z_vel = 0;
            break;
        case WM_AOP_SET_XVEL:
            actor->x_vel = directional(actor, o->a, o->mode,
                                       (uint16_t)WM_MOVE_RIGHT);
            break;
        case WM_AOP_SET_YVEL:
            actor->y_vel = o->a;
            break;
        case WM_AOP_SET_ZVEL:
            actor->z_vel = directional(actor, o->a, o->mode,
                                       (uint16_t)WM_MOVE_DOWN);
            break;
        case WM_AOP_MIN_YVEL:
            if (actor->y_vel < o->a) actor->y_vel = o->a;
            break;
        case WM_AOP_FRICTION:
            actor->friction = o->a;
            actor->anim_mode |= (uint16_t)WM_MODE_FRICTION;
            break;
        case WM_AOP_OFFSET: {
            int32_t dx = (actor->facing_dir & WM_MOVE_RIGHT) ? o->a : -o->a;
            actor->x_int += dx;
            actor->y_int += o->b;
            actor->z_int += o->c;
            actor->x_fixed = actor->x_int << 16;
            actor->y_fixed = actor->y_int << 16;
            actor->z_fixed = actor->z_int << 16;
            break;
        }
        case WM_AOP_SETSPEED:
            actor->ani_speed = (uint16_t)o->a;
            break;
        case WM_AOP_STARTATTACK:
            actor->attack_type = (uint16_t)o->a;
            actor->attack_time = (uint16_t)(round_tickcount +
                                            (o->b > 0 ? o->b : 30));
            break;
        case WM_AOP_FACEUP:
            actor->facing_dir = (actor->obj_control & WM_OBJ_FLIPH)
                ? WM_MOVE_UP_LEFT : WM_MOVE_UP_RIGHT;
            break;
        case WM_AOP_FACEDOWN:
            actor->facing_dir = (actor->obj_control & WM_OBJ_FLIPH)
                ? WM_MOVE_DOWN_LEFT : WM_MOVE_DOWN_RIGHT;
            break;
        case WM_AOP_SET_WRESTLER_XFLIP:
            if (actor->facing_dir & WM_MOVE_RIGHT)
                actor->obj_control &= (uint16_t)~WM_OBJ_FLIPH;
            else
                actor->obj_control |= (uint16_t)WM_OBJ_FLIPH;
            break;
        case WM_AOP_CLR_BUTCOUNT:
            /* ANIM.ASM:3512 clears all five, not three: the two `,L` writes
               are 32-bit and each covers two adjacent PLYR.EQU WORDs
               (punch+block, super punch+kick), with a plain 16-bit write
               for super kick. See wm_arcade_combat.h's own note -- the
               commented-out single-WORD lines above them in the source are
               the unoptimised version of the same clear, not evidence that
               block and kick were dropped from it. */
            wm_arcade_clear_button_presses(actor);
            break;
        case WM_AOP_SAFE_TIME:
            actor->safe_time = o->a;
            break;
        case WM_AOP_GRAVITY_ON:
            actor->anim_mode &= (uint16_t)~WM_MODE_NOGRAVITY;
            break;
        case WM_AOP_BOUNCE:
            /* ANIM.ASM:950: `move *a4+,a0 / sll 16,a0` -- the operand is a
               whole-pixel-per-tick upward kick. */
            actor->y_vel = o->a << 16;
            break;
        case WM_AOP_WAITHITOPP:
            actor->anim_mode |= (uint16_t)WM_MODE_WAITHITOPP;
            break;
        case WM_AOP_GETUP:
            /* `move *a13(PLYR_DIZZY),a14 / jrnz #skip` -- a dizzy wrestler
               keeps whatever getup time he already had. */
            if (!actor->plyr_dizzy) actor->getup_time = o->a;
            break;
        /*
         * ANIM.ASM:4247 _ani_add_move -- it touches no velocity at all,
         * despite the name: it reads three operands and grows the combo
         * meter, unless the man he hit is already dead.
         */
        case WM_AOP_ADD_MOVE:
            if (actor->who_i_hit &&
                actor->who_i_hit->player_mode != WM_PMODE_DEAD)
                wm_arcade_add_to_combo_count(actor, o->a);
            break;
        /*
         * ANIM.ASM:114 _ani_inc_combo_count. Two things beyond the count:
         * at exactly 8 the announcer is asked for HES_JUST_GONE_BERSERK
         * (the speech queue, which this port does not have -- so the
         * threshold is reached and nothing is said, rather than a sound
         * being invented), and the man he hit is immobilised for 30 ticks,
         * which is the combo lock.
         */
        case WM_AOP_INC_COMBO:
            ++actor->combo_count;
            if (actor->who_i_hit) actor->who_i_hit->immobilize_time = 30;
            break;
        /*
         * ANIM.ASM:115 _ani_clear_combo_count is BOTH ends of a combo.
         *
         * With COMBO_COUNT already set it is the end: zero it, and free
         * the victim -- immobilize and getup cleared, but DELAY_METER set
         * to 10*60 so his getup meter stays away for ten seconds.
         *
         * With COMBO_COUNT zero it is the start (`#start_combo`): the
         * victim gets an 80-tick IMMOBILIZE_TIME, which the source labels
         * "Time opponent has to execute combo breaker", plus a PCNT stamp.
         * COMBO_COUNT is written 0 either way -- the source's own comment
         * argues about whether it should be 1 and settles on 0.
         *
         * The target is ATTACH_PROC if there is one, else WHOIHIT; the
         * source LOCKUPs when there is neither, which is an assert, so
         * here it simply does nothing to nobody.
         */
        case WM_AOP_CLEAR_COMBO: {
            int ending = actor->combo_count != 0;
            wm_arcade_actor_t *victim = actor->attach_proc
                ? actor->attach_proc : actor->who_i_hit;
            actor->combo_count = 0;
            if (!victim) break;
            if (ending) {
                victim->immobilize_time = 0;
                victim->getup_time = 0;
                victim->delay_meter = 10 * 60;
            } else {
                victim->immobilize_time = 80;
                victim->anti_combo_time = env ? env->pcnt : 0u;
                victim->getup_time = 0;
            }
            break;
        }
        case WM_AOP_SETLONG:
            if (o->a == 0) actor->gravity = o->b;
            else actor->debris_x = o->b;
            break;
        case WM_AOP_SETWORD:
            if (o->a == 0) actor->usr_var1 = o->b;
            else if (o->a == 1) actor->usr_var2 = o->b;
            else actor->delay_meter = o->b;
            break;
        /* ANIM.ASM:50 _ani_face: the operand is a facing, XOR'd across
           left/right when the sprite is mirrored so it means the same
           direction on screen either way. */
        case WM_AOP_FACE: {
            int32_t dir = o->a;
            if (actor->obj_control & WM_OBJ_FLIPH)
                dir ^= (int32_t)(WM_MOVE_LEFT | WM_MOVE_RIGHT);
            actor->facing_dir = dir;
            break;
        }
        /* ANIM.ASM:26 _ani_sound -- see wm/anim_program.h. */
        case WM_AOP_SOUND: {
            uint16_t call = (uint16_t)o->a;
            if (!env || !env->sound) break;
            if (call == WM_SND_RUN) {
                uint16_t now = (uint16_t)env->pcnt;
                int32_t since = (int32_t)now - (int32_t)actor->foot_pcnt;
                if (since < 0) since = -since;
                if (since < 12) break;
                actor->foot_pcnt = now;
            }
            env->sound(env->sound_user, call);
            break;
        }
        /*
         * ANIM.ASM:35/:37 -- the bank is chosen by which side of the ring
         * he is on, and a NEGATIVE operand means the release variant.
         * `movi ROPE_LEFT / cmpi RING_X_CENTER / jrle #dir_set`: at or
         * left of centre is the LEFT bank.
         */
        case WM_AOP_BOUNCEROPE:
        case WM_AOP_BENDROPE: {
            int bank = (actor->x_int <= WM_RING_X_CENTER)
                ? WM_ROPE_LEFT : WM_ROPE_RIGHT;
            int release = o->a < 0;
            int action = (o->op == WM_AOP_BOUNCEROPE)
                ? (release ? WM_ROPE_SIDE_SPRING_RELEASE : WM_ROPE_SIDE_SPRING)
                : (release ? WM_ROPE_DOWN_SPRING_RELEASE : WM_ROPE_DOWN_SPRING);
            /* The release tables ignore the selector -- rope_command routes
               them to a fixed transition script -- so the source's
               negative a2 never reaches a table index. */
            int selector = release ? 0 : o->a;
            if (env && env->rope_command)
                env->rope_command(env->rope_user, bank, action, selector,
                                  actor->z_fixed);
            /* `movi 3ch,a0 / calla triple_sound` -- only the bounce. */
            if (o->op == WM_AOP_BOUNCEROPE && env && env->sound)
                env->sound(env->sound_user, 0x3Cu);
            break;
        }
        case WM_AOP_ROPE_Z: {
            /* :41 picks the bank from OBJ_XPOS against RING_X_CENTER with
               `jrgt #right`, so strictly greater is the RIGHT bank -- the
               opposite boundary case from the two above, as written. */
            int bank = (actor->x_int > WM_RING_X_CENTER)
                ? WM_ROPE_RIGHT : WM_ROPE_LEFT;
            if (env && env->rope_set_z)
                env->rope_set_z(env->rope_user, bank, o->a, o->b);
            break;
        }
        case WM_AOP_XFLIP_OPP: {
            wm_arcade_actor_t *opp = actor->attach_proc;
            if (!opp || opp->attach_proc != actor) break;
            opp->obj_control = (uint16_t)(opp->obj_control ^ WM_OBJ_FLIPH);
            break;
        }
        case WM_AOP_SETOPPFACING: {
            /* :85 checks both pointers are set but NOT that the link is
               mutual, unlike :106 beside it. Kept as written. */
            wm_arcade_actor_t *opp = actor->attach_proc;
            if (!opp || !opp->attach_proc) break;
            opp->facing_dir = opp->new_facing_dir;
            break;
        }
        case WM_AOP_SET_OPP_XVEL: {
            wm_arcade_actor_t *opp = actor->attach_proc;
            if (!opp || opp->attach_proc != actor) break;
            opp->x_vel = directional(opp, o->a, o->mode,
                                     (uint16_t)WM_MOVE_RIGHT);
            break;
        }
        case WM_AOP_CLEAR_CLIMB:
            actor->climbing_thru = 0;
            actor->safe_time = 1;
            break;
        case WM_AOP_GRAVITY_OFF:
            actor->anim_mode |= (uint16_t)WM_MODE_NOGRAVITY;
            break;
        /* ANIM.ASM:56 _ani_damage: `neg a0` first -- the source's comment
           is "positive a0 = health increase", so the operand is the damage
           and adjust_health takes it as a negative delta. */
        case WM_AOP_DAMAGE:
            wm_arcade_adjust_health(actor, (int16_t)-o->a, NULL, false,
                                    env ? env->pcnt : 0u, NULL, NULL);
            break;
        /* ANIM.ASM:105 -- the held wrestler's PLYRMODE, behind the same
           mutual-link check as the rest of the group, and refused on a
           wrestler who is already dead. */
        case WM_AOP_SETOPP_PLYRMODE: {
            wm_arcade_actor_t *opp = actor->attach_proc;
            if (!opp || opp->attach_proc != actor) break;
            if (opp->player_mode == WM_PMODE_DEAD) break;
            opp->player_mode = (uint16_t)o->a;
            break;
        }
        /*
         * ANIM.ASM:76 _ani_opp_getup. Target is ATTACH_PROC if there is
         * one, else WHOIHIT -- note it does NOT check the link is mutual,
         * unlike its neighbours. A NEGATIVE operand means "clear his
         * DELAY_METER as well", and the time used is its absolute value;
         * a dizzy victim keeps whatever getup time he had.
         */
        case WM_AOP_OPP_GETUP: {
            wm_arcade_actor_t *opp = actor->attach_proc
                ? actor->attach_proc : actor->who_i_hit;
            int32_t time = o->a;
            if (!opp) break;
            if (time < 0) {
                opp->delay_meter = 0;
                time = -time;
            }
            if (opp->plyr_dizzy) break;
            opp->getup_time = time;
            break;
        }
        /* ANIM.ASM:24 _ani_attachvel: the held wrestler's three
           velocities. Y and Z are absolute; X is always relative to the
           ATTACKER's facing, negated when he faces left. */
        case WM_AOP_ATTACHVEL: {
            wm_arcade_actor_t *opp = actor->attach_proc;
            if (!opp || !opp->attach_proc) break;
            opp->y_vel = o->b;
            opp->z_vel = o->c;
            opp->x_vel = (actor->facing_dir & WM_MOVE_RIGHT) ? o->a : -o->a;
            break;
        }
        case WM_AOP_CLR_STATUS:
            actor->anim_mode &= (uint16_t)~WM_MODE_STATUS;
            break;
        /* ANIM.ASM:4156 _ani_set_attach -- the link that STARTS a grapple:
           this wrestler and the one he just hit each point at the other. */
        case WM_AOP_SET_ATTACH:
            if (actor->who_i_hit) {
                actor->attach_proc = actor->who_i_hit;
                actor->who_i_hit->attach_proc = actor;
            }
            break;
        /*
         * ANIM.ASM:851 _ani_detach -- the link that ENDS it. The source
         * clears its own side first, then only clears the victim's if the
         * victim really was pointing back (an unmatched pair is left
         * alone), and finally rescues a victim still in a held mode by
         * putting him ONGROUND rather than leaving him puppeted with
         * nobody driving.
         *
         * MODE_HEADHELD is deliberately NOT in that list. The source had
         * it and commented it out, with the reason: "This was fucking up
         * the shawn franknsteiner move from headhold! Forcing him to dive
         * down too low!"
         */
        case WM_AOP_DETACH: {
            wm_arcade_actor_t *victim = actor->attach_proc;
            if (!victim) break;
            actor->attach_proc = 0;
            if (victim->attach_proc != actor) break;
            victim->attach_proc = 0;
            if (victim->player_mode == WM_PMODE_PUPPET ||
                victim->player_mode == WM_PMODE_PUPPET2 ||
                victim->player_mode == WM_PMODE_ATTACHED)
                victim->player_mode = WM_PMODE_ONGROUND;
            victim->puppet_frame = 0;
            break;
        }
        /* ANIM.ASM:1039 _ani_attachz -- where the held wrestler hangs.
           The source reads x and y as one long and z as a word, which
           lands on PLYR.EQU:74-76's three adjacent words. */
        case WM_AOP_ATTACHZ:
            actor->attach_xoff = o->a;
            actor->attach_yoff = o->b;
            actor->attach_zoff = o->c;
            break;
        /* ANIM.ASM:2888/:2912 -- set or clear bits in the HELD wrestler's
           ANIMODE, both behind the same mutual-link check: an animation
           cannot reach into someone who is not actually held. */
        case WM_AOP_SETOPPMODE:
            if (actor->attach_proc && actor->attach_proc->attach_proc)
                actor->attach_proc->anim_mode |= (uint16_t)o->a;
            break;
        case WM_AOP_CLROPPMODE:
            if (actor->attach_proc && actor->attach_proc->attach_proc)
                actor->attach_proc->anim_mode &= (uint16_t)~o->a;
            break;
        /* ANIM.ASM:3933 _ani_immobilize -- hold the victim still. Skipped
           when this wrestler is dizzy, and the source's own comment for
           the other guard: "don't immobilize blockers!" */
        /*
         * ANIM.ASM:3997 _ani_setoppvels -- the throw itself: the launch
         * velocity handed to the wrestler being thrown. It works on the
         * held wrestler if the link is mutual and on WHOIHIT otherwise, and
         * the x and z signs come from the ATTACKER's facing, not the
         * victim's, so a throw always goes the way the thrower is facing.
         */
        case WM_AOP_SETOPPVELS: {
            wm_arcade_actor_t *v = actor->attach_proc;
            if (!v || v->attach_proc != actor) v = actor->who_i_hit;
            if (!v) break;
            v->y_vel = o->b;
            v->x_vel = (actor->facing_dir & WM_MOVE_RIGHT) ? o->a : -o->a;
            v->z_vel = (actor->facing_dir & WM_MOVE_DOWN) ? o->c : -o->c;
            break;
        }
        /*
         * ANIM.ASM:2175 _ani_damageopp, "works on attached proc, or WHOIHIT
         * if there isn't one". Full damage normally; the REDUCED figure
         * when this victim was already hurt within the last 30 ticks, which
         * is what stops a combo doing full damage on every blow; and the
         * attacker's own NEXT_DAMAGE instead when he has one set and its
         * SPECIAL_DAMAGE_TIME has not passed.
         *
         * The source then does the awards, the first-unblocked-hit message
         * and the taunt-style RISK multiplier before calling adjust_health.
         * Those need the award and message systems; the damage itself is
         * applied through the same real LIFEBAR.ASM adjust_health every
         * other caller in this port shares.
         */
        case WM_AOP_DAMAGEOPP: {
            wm_arcade_actor_t *v = actor->attach_proc;
            int32_t dmg = o->a;
            if (!v) v = actor->who_i_hit;
            if (!v) break;
            if (v->last_damage &&
                (int32_t)((env ? env->pcnt : 0u) - v->last_damage) <= 30)
                dmg = o->b;
            if (actor->next_damage &&
                (env ? env->pcnt : 0u) <= actor->special_damage_time)
                dmg = actor->next_damage;
            wm_arcade_adjust_health(v, (int16_t)-dmg, actor, false,
                                    env ? env->pcnt : 0u, 0, 0);
            break;
        }
        /*
         * ANIM.ASM:2130 _ani_slaveanim -- the victim runs a whole animation
         * of his OWN. The source swaps a13 to the victim and calls
         * change_anim1a outright, which is how a slam makes him play his
         * landing rather than being posed frame by frame. The table is
         * indexed by the victim's WRESTLERNUM, so each one lands in his own
         * way. Behind the same mutual-link check as everything else here.
         */
        case WM_AOP_SLAVEANIM: {
            wm_arcade_actor_t *v = actor->attach_proc;
            const char *label;
            if (!v || v->attach_proc != actor) break;
            label = wm_anim_slave_label((size_t)o->a, v->wrestler_num);
            if (!label || !env || !env->change_opp_anim) break;
            v->puppet_frame = 0;      /* he drives himself again now */
            env->change_opp_anim(v, label, env->slave_user);
            break;
        }
        case WM_AOP_IMMOBILIZE:
            if (!actor->dizzy && actor->who_i_hit &&
                actor->who_i_hit->player_mode != WM_PMODE_BLOCK)
                actor->who_i_hit->immobilize_time = o->a;
            break;
        case WM_AOP_ATTACK_ON: {
            wm_arcade_attack_on_args_t args;
            args.attack_mode = o->mode;
            args.xoff = (int16_t)o->a; args.yoff = (int16_t)o->b;
            args.width = (int16_t)o->c; args.height = (int16_t)o->d;
            wm_arcade_ani_attack_on(actor, &args);
            break;
        }
        case WM_AOP_ATTACK_ON_Z: {
            wm_arcade_attack_on_z_args_t args;
            args.attack_mode = o->mode;
            args.xoff = (int16_t)o->a; args.yoff = (int16_t)o->b;
            args.zoff = (int16_t)o->c; args.width = (int16_t)o->d;
            args.height = (int16_t)o->e; args.depth = (int16_t)o->f;
            wm_arcade_ani_attack_on_z(actor, &args);
            break;
        }
        case WM_AOP_ATTACK_OFF:
            wm_arcade_ani_attack_off(actor, round_tickcount);
            break;
        case WM_AOP_CODE: {
            /* ANIM.ASM:1277: an ordinary call, then straight on to the
               next command. A routine this port has not translated leaves
               the op a no-op -- the same thing the flat extractor did by
               dropping the line, except that the name is now in the
               program, where it can be counted. */
            /* The file matters: a '#'-prefixed target is a local label,
               scoped to the sequence file that defines it, and those names
               are reused across files with different bodies behind them. */
            (void)wm_anim_code_run(actor, env, o->text, source_file);
            break;
        }
        default:
            /* WM_AOP_UNTRANSLATED and anything needing a subsystem this
               port does not have: carried in the program so it stays a
               faithful record, executed as a no-op. */
            break;
    }
}

/*
 * ANIM.ASM:2681 _ani_superslave2 -- the puppet step.
 *
 * The source verifies the links first ("move *a13(ATTACH_PROC),a11 / move
 * *a11(ATTACH_PROC),a0 / cmp a13,a0 / jrne #done"): both wrestlers must
 * point at each other, so a grapple that has already been broken quietly
 * does nothing rather than driving someone who is no longer held.
 *
 * Then it sets its own frame, looks the defender's up by the DEFENDER's
 * WRESTLERNUM, and hangs it at an offset built from the raw table values
 * adjusted by both frames' animation origins:
 *
 *     attach Y = raw y - defender aniY + attacker aniY
 *     attach X = raw x + defender part - attacker aniX
 *
 * where the defender part is its aniX, or (xsize - aniX) when the table's
 * flip disagrees with the attacker's own -- the source's own
 * #attacker_flip_test. This port has real per-frame geometry for every
 * wrestler (wm/frame_geometry.h), so those are read rather than guessed.
 */
static void run_superslave2(const wm_anim_op *o, wm_arcade_actor_t *actor,
                            const wm_anim_env *env) {
    wm_arcade_actor_t *def = env ? env->opponent : 0;
    const wm_anim_puppet_row *row;
    const wm_frame_geometry_t *ag, *dg;
    int32_t part;

    /* "verify the links" -- a13 and a11 must hold each other. */
    if (!def || actor->attach_proc != def || def->attach_proc != actor) return;

    row = wm_anim_puppet_row_at((size_t)o->b, def->wrestler_num,
                                (size_t)o->c);
    if (!row) return;

    def->puppet_frame = row->frame;
    def->puppet_flip = row->flip;

    ag = wm_frame_geometry_find(o->text);
    dg = wm_frame_geometry_find(row->frame);
    if (!ag || !dg) return;

    def->attach_yoff = row->yoff - dg->yani + ag->yani;

    /* The defender's own part, mirrored when the table's flip disagrees
       with which way the attacker is facing. */
    part = dg->xani;
    if ((row->flip != 0) != ((actor->obj_control & WM_OBJ_FLIPH) != 0))
        part = dg->width - part;
    def->attach_xoff = row->xoff + part - ag->xani;
}


/* Walk from `pc` executing commands and taking branches until a frame is
   reached (which is what a tick shows) or the program stops. */
static void advance(wm_anim_exec *exec, wm_arcade_actor_t *actor,
                    uint16_t round_tickcount, size_t pc) {
    const wm_anim_program *p = exec->program;
    /* A branch-only cycle would spin forever; the real machine cannot do
       that either, since every loop it takes passes through a frame. */
    size_t guard = 0;
    const size_t limit = p ? p->op_count * 4 + 64 : 0;

    while (p && pc < p->op_count && guard++ < limit) {
        const wm_anim_op *o = &p->ops[pc];
        switch (o->op) {
            case WM_AOP_FRAME:
                exec->pc = pc;
                exec->next_pc = pc + 1;
                exec->ticks_left = (uint16_t)(o->a > 0 ? o->a : 1);
                exec->waiting = false;
                return;
            case WM_AOP_PAUSE:
                /* ANIM.ASM:28 -- OANICNT without touching the frame, so
                   whatever is showing stays up for the operand's ticks. */
                exec->next_pc = pc + 1;
                exec->ticks_left = (uint16_t)(o->a > 0 ? o->a : 1);
                return;
            case WM_AOP_WAITHITGND: {
                /*
                 * ANIM.ASM:890 _ani_waithitgnd. "must have down velocity":
                 * a POSITIVE OBJ_YVEL is rising, so it is never a landing.
                 * Then, if this wrestler is the master of a live grapple
                 * and his victim is not MODE_GHOST, HIS victim hitting the
                 * ground counts as the landing -- a slam ends when the man
                 * being slammed lands, not when the slammer does.
                 */
                int landed = 0;
                if (actor && actor->y_vel <= 0) {
                    wm_arcade_actor_t *opp = actor->attach_proc;
                    if ((actor->anim_mode & WM_MODE_KEEPATTACHED) && opp &&
                        opp->attach_proc == actor &&
                        !(opp->anim_mode & WM_MODE_GHOST) &&
                        opp->y_int <= opp->ground_y) {
                        landed = 1;
                    } else if (actor->y_int <= actor->ground_y) {
                        landed = 1;
                    }
                }
                if (!landed) {
                    /* `movk 1,a0 / move a0,*a10(OANICNT)`: hold the frame
                       already showing for one tick and come back here.
                       OANIPC is untouched, so exec->pc must not move. */
                    exec->next_pc = pc;
                    exec->ticks_left = 1;
                    exec->waiting = true;
                    return;
                }
                exec->waiting = false;
                if (actor) wm_anim_code_run(actor, exec->env, "SMALL_BOUNCE",
                                            NULL);
                pc = pc + 1;
                continue;
            }
            case WM_AOP_ROT:
                /* Park here: the frame showing stays up and this op comes
                   back round every tick, exactly as OANICNT=1 with OANIPC
                   untouched does. Not an end -- the animation is still
                   running, it just never goes anywhere. */
                exec->next_pc = pc;
                exec->ticks_left = 1;
                return;
            case WM_AOP_WAITROLL: {
                /*
                 * ANIM.ASM:2990 _ani_waitroll. The order is the source's:
                 * a zombie always rolls up; a wrestler already MODE_DEAD,
                 * or one whose I_WILL_DIE has come due with IMMOBILIZE_TIME
                 * spent, becomes his dead animation; everyone else is put
                 * MODE_ONGROUND ("just to be safe", the source says) and
                 * waits out IMMOBILIZE_TIME and GETUP_TIME before rolling.
                 *
                 * NOT translated, and unreachable in this port's match
                 * mode rather than skipped: the `#dead` path's drone
                 * branches (is_8_on_1, royal_rumble, FINAL_PTR's zombie
                 * promotion, adjust_health). In an ordinary match a player
                 * takes `jreq #die` straight away, and a drone reaches the
                 * same `jruc #die` with is_8_on_1 false and royal_rumble
                 * zero -- so both land on the same hand-off this runs.
                 */
                int rolled;
                if (!actor) { exec->ended = true; return; }

                if (actor->status_flags & WM_STATUS_ZOMBIE) {
                    /* `movi J_UP,a14` into both DRN_JOY and STICK_VAL_CUR:
                       a zombie rolls up whatever the player does. */
                    actor->stick_val_cur = (uint16_t)WM_MOVE_UP;
                } else if (actor->player_mode == WM_PMODE_DEAD) {
                    exec->become = "xxx_dead_anim";
                    exec->ended = true;
                    return;
                } else if (actor->i_will_die) {
                    if (actor->immobilize_time) goto waitroll_repeat;
                    actor->immobilize_time = 0;
                    actor->i_will_die = 0;
                    actor->player_mode = WM_PMODE_DEAD;
                    wm_arcade_clear_lifebar(actor);
                    exec->become = "xxx_dead_anim";
                    exec->ended = true;
                    return;
                } else {
                    actor->player_mode = WM_PMODE_ONGROUND;
                    if (actor->immobilize_time) goto waitroll_repeat;
                    if (actor->getup_time) goto waitroll_repeat;
                }

                actor->stars_flag = 0;
                rolled = wm_arcade_do_roll(actor);
                if (!rolled) {
                    /* `jrz #getup`: he stopped rolling, so he gets up --
                       the animation runs on past this op. */
                    exec->waiting = false;
                    actor->roll_frame = 0;
                    pc = pc + 1;
                    continue;
                }
            waitroll_repeat:
                /* `#repeat`: clear Z_BOUND, hold the frame one tick. */
                actor->z_bound = 0;
                exec->next_pc = pc;
                exec->ticks_left = 1;
                exec->waiting = true;
                return;
            }
            case WM_AOP_SUPERSLAVE2:
                /* It sets OANICNT and stops, so it yields exactly like a
                   frame -- it IS the attacker's frame, and it chooses the
                   defender's at the same time. */
                exec->pc = pc;
                exec->next_pc = pc + 1;
                exec->ticks_left = (uint16_t)(o->a > 0 ? o->a : 1);
                if (actor) run_superslave2(o, actor, exec->env);
                return;
            case WM_AOP_END:
            case WM_AOP_REPEAT:
                exec->ended = true;
                return;
            case WM_AOP_GOTO:
                pc = (size_t)o->target;
                continue;
            case WM_AOP_IFSTATUS:
                pc = (actor && (actor->anim_mode & WM_MODE_STATUS))
                    ? (size_t)o->target : pc + 1;
                continue;
            case WM_AOP_IFNOTSTATUS:
                pc = (actor && !(actor->anim_mode & WM_MODE_STATUS))
                    ? (size_t)o->target : pc + 1;
                continue;
            case WM_AOP_IFBLOCKED:
                pc = (actor && actor->hitblocker) ? (size_t)o->target : pc + 1;
                continue;
            case WM_AOP_SLIDE_BACK:
                /* "was there a collision? jrz #no_slide": the branch is the
                   MISS path. The slide itself needs the opponent's position
                   and the ring bounds, so only the fork is modelled. */
                pc = (actor && !(actor->anim_mode & WM_MODE_STATUS))
                    ? (size_t)o->target : pc + 1;
                continue;
            case WM_AOP_SET_RPTCOUNT:
                /* A negative operand means RNDRNG0(-n) at runtime, which a
                   static program cannot carry; tools/wlprogram.py refuses
                   those, so anything here is a real fixed count. */
                exec->rpt_count = (uint16_t)(o->a > 0 ? o->a : 0);
                pc += 1;
                continue;
            case WM_AOP_DEC_RPTCOUNT:
                if (exec->rpt_count) --exec->rpt_count;
                pc += 1;
                continue;
            case WM_AOP_IF_RPTCOUNT:
                pc = exec->rpt_count ? (size_t)o->target : pc + 1;
                continue;
            case WM_AOP_IFNOT_RPTCOUNT:
                /* ANIM.ASM:91 `jrnz #fail2` -- the same test inverted. */
                pc = !exec->rpt_count ? (size_t)o->target : pc + 1;
                continue;
            /* ANIM.ASM:3214 `cmp a0,a14 / jrlt #fail`: the branch is taken
               when the count is at least the operand -- and :3239's
               `jrge #fail2` is its exact complement. */
            case WM_AOP_IF_BUTCOUNT_GE:
                pc = (actor &&
                      wm_arcade_button_count(actor, (int)o->a) >= o->b)
                    ? (size_t)o->target : pc + 1;
                continue;
            /*
             * ANIM.ASM:2417 _ani_ifoppmode, with the source's own summary:
             * "If opponent PLYRMODE is #MODE, jump to #BRANCH. If the high
             * bit of #MODE is set, jump on PLYRMODE != ~#MODE." So one
             * operand encodes both the equality and the inequality test,
             * and the negative form is the ones' complement.
             */
            case WM_AOP_IFOPPMODE: {
                const wm_arcade_actor_t *opp = (exec->env) ? exec->env->opponent : 0;
                bool take = false;
                if (opp) {
                    if (o->a < 0) take = opp->player_mode != (uint16_t)(~o->a);
                    else          take = opp->player_mode == (uint16_t)o->a;
                }
                pc = take ? (size_t)o->target : pc + 1;
                continue;
            }
            case WM_AOP_IF_BUTCOUNT_LT:
                pc = (actor &&
                      wm_arcade_button_count(actor, (int)o->a) < o->b)
                    ? (size_t)o->target : pc + 1;
                continue;
            case WM_AOP_CHANGEANIM:
                exec->become = o->text;
                exec->ended = true;
                return;
            case WM_AOP_IFBUTTONS:
                if (actor &&
                    ((uint32_t)actor->but_val_cur & (uint32_t)o->a) ==
                        (uint32_t)o->a) {
                    exec->become = o->text;
                    exec->ended = true;
                    return;
                }
                pc += 1;
                continue;
            default:
                if (actor) run_command(o, actor, round_tickcount, exec->env,
                                       p ? p->source_file : 0);
                pc += 1;
                continue;
        }
    }
    exec->ended = true;
}

void wm_anim_exec_start(wm_anim_exec *exec, const wm_anim_program *program,
                        wm_arcade_actor_t *actor, uint16_t round_tickcount,
                        const wm_anim_env *env) {
    if (!exec) return;
    memset(exec, 0, sizeof(*exec));
    exec->program = program;
    exec->env = env;
    /*
     * ANIM.ASM:4553 change_anim1 (and :4520 change_anim_anim) both do
     * `movi GRAVITY,a0 / move a0,*a13(OBJ_GRAVITY),L` -- "reset gravity",
     * their own comment. Every animation therefore starts at the default
     * fall rate, and an animation that wants its own must say so with
     * ANI_SETLONG,OBJ_GRAVITY. Without this reset a single heavy fall
     * would make the wrestler heavy for the rest of the match.
     */
    if (actor) actor->gravity = WM_GRAVITY;
    if (!program || program->op_count == 0) {
        exec->ended = true;
        return;
    }
    advance(exec, actor, round_tickcount, 0);
    /* The loop shows a frame before consuming a tick of it, same as
       wm_visual_start's own just_started. */
    exec->just_started = true;
}

void wm_anim_exec_tick(wm_anim_exec *exec, wm_arcade_actor_t *actor,
                       uint16_t round_tickcount) {
    if (!exec || exec->ended || !exec->program) return;
    if (exec->just_started) {
        exec->just_started = false;
        return;
    }
    if (exec->ticks_left > 1) {
        --exec->ticks_left;
        return;
    }
    advance(exec, actor, round_tickcount, exec->next_pc);
}

const char *wm_anim_exec_frame(const wm_anim_exec *exec) {
    if (!exec || exec->ended || !exec->program) return NULL;
    if (exec->pc >= exec->program->op_count) return NULL;
    return exec->program->ops[exec->pc].text;
}
