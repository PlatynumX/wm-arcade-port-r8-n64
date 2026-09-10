#include "wm/anim_program.h"
#include "wm/arcade/wm_arcade_veladd.h"
#include "wm/arcade/wm_arcade_roll.h"
#include "wm/arcade/wm_arcade_combo.h"
#include "wm/arcade/wm_arcade_announcer.h"
#include "wm/arcade/wmania_ring_geometry.h"
#include "wm/arcade/wmania_rope_command.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wm_arcade_anim_combat.h"
#include "wm/arcade/wm_arcade_butcount.h"
#include "wm/arcade/wm_arcade_target.h"
#include "wm/frame_geometry.h"
#include "wm/arcade/wm_arcade_lifebar.h"

#include <string.h>

/* ANIM.ASM:1626 _ani_set_xvel / :1829 _ani_set_zvel: absolute, or negated
   when the direction bit the AM_* mode names is clear. */
/* `movi 3ch,a0 / calla triple_sound` -- the rope thump, shared by the
   bounce and by the three shake commands that make one. */
#define WM_ROPE_THUMP_SOUND 0x3Cu

/* ANIM.ASM:4405 `movi 80,a0 / move a0,@allow_offscrn`. */
#define WM_ALLOW_OFFSCRN_TICKS 80

/* ANIM.ASM:1683 `cmpi >0f0000,a1` -- the leap's Y velocity cap. */
#define WM_LEAP_MAX_YVEL 0x0F0000

static void play_sound(const wm_anim_env *env, uint16_t call) {
    if (env && env->sound) env->sound(env->sound_user, call);
}

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
         * through DCSSOUND.ASM's IF_SILENT_ADD_VOICE (wm/arcade/
         * wm_arcade_announcer.h) rather than a direct sound, and the man
         * he hit is immobilised for 30 ticks, which is the combo lock.
         */
        case WM_AOP_INC_COMBO:
            ++actor->combo_count;
            /* `CMPI 8,A0 / JRNE NO_BESERKER` -- at exactly eight, and
               through IF_SILENT_ADD_VOICE rather than a direct sound. */
            if (actor->combo_count == 8 && env && env->announcer)
                (void)wm_announcer_add_if_silent(
                    env->announcer, WM_VOICE_HES_JUST_GONE_BERSERK);
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
            if (o->op == WM_AOP_BOUNCEROPE)
                play_sound(env, WM_ROPE_THUMP_SOUND);
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
        case WM_AOP_XFLIP_TBL: {
            /* Behind the mutual-link check, and indexed by the HELD
               wrestler's number -- a table row of 0 means no flip. */
            wm_arcade_actor_t *opp = actor->attach_proc;
            if (!opp || opp->attach_proc != actor) break;
            if (!wm_anim_xflip_for((size_t)o->a, opp->wrestler_num)) break;
            opp->obj_control = (uint16_t)(opp->obj_control ^ WM_OBJ_FLIPH);
            break;
        }
        case WM_AOP_OPPOFFSET: {
            /*
             * ANIM.ASM:82 checks both pointers are set (not that the link
             * is mutual) and reads an x/y pair for the held wrestler. The
             * source then positions him from his own X and FACING_DIR;
             * this port applies the pair as an offset from where he is,
             * which is the part the table actually carries.
             */
            wm_arcade_actor_t *opp = actor->attach_proc;
            int16_t dx = 0, dy = 0;
            if (!opp || !opp->attach_proc) break;
            if (!wm_anim_oppoffset_for((size_t)o->a, opp->wrestler_num,
                                       &dx, &dy))
                break;
            opp->x_int += (opp->facing_dir & WM_MOVE_RIGHT) ? dx : -dx;
            opp->y_int += dy;
            opp->x_fixed = opp->x_int << 16;
            opp->y_fixed = opp->y_int << 16;
            break;
        }
        case WM_AOP_SETFLAG:
            actor->status_flags |= (uint32_t)o->a;
            break;
        case WM_AOP_CHECKWORD: {
            int32_t v = (o->a == 0) ? actor->usr_var1
                      : (o->a == 1) ? actor->usr_var2 : actor->delay_meter;
            if (v) actor->anim_mode |= (uint16_t)WM_MODE_STATUS;
            else actor->anim_mode &= (uint16_t)~WM_MODE_STATUS;
            break;
        }
        case WM_AOP_IFOPP: {
            /* The source walks CLOSEST_NUM's process, not ATTACH_PROC --
               this asks who he is FIGHTING, not who he is holding. */
            const wm_arcade_actor_t *opp = env ? env->opponent : 0;
            int hit = opp && opp->wrestler_num >= 0 &&
                      opp->wrestler_num < 32 &&
                      (o->a & (1 << opp->wrestler_num));
            if (hit) actor->anim_mode |= (uint16_t)WM_MODE_STATUS;
            else actor->anim_mode &= (uint16_t)~WM_MODE_STATUS;
            break;
        }
        case WM_AOP_FACE_OPP: {
            wm_arcade_actor_t *opp = actor->attach_proc;
            int32_t dir = o->a;
            if (!opp || opp->attach_proc != actor) break;
            if (opp->obj_control & WM_OBJ_FLIPH)
                dir ^= (int32_t)(WM_MOVE_LEFT | WM_MOVE_RIGHT);
            opp->facing_dir = dir;
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
        /*
         * ANIM.ASM:3753 _ani_target -- aim at whichever of two body parts
         * is nearer, and "nearer" is decided by the two wrestlers' flip
         * bits rather than by distance. The grid it reads is
         * wm/arcade/wm_arcade_target.h; the source's own caveat comes with
         * it: "This assumes that victim is on the ground. If he's not, the
         * results will be screwy."
         */
        case WM_AOP_TARGET: {
            wm_arcade_actor_t *opp = env ? env->opponent : NULL;
            if (!actor || !opp) break;
            wm_arcade_anim_target(actor, opp, (int)o->a, (int)o->b,
                                  (int)o->c);
            break;
        }
        /* ANIM.ASM:4389 _ani_draw_name -- the move's name flashed up.
           The source's own note above it is "This is bog! Check to see if
           we want messages before CREATE!", which is what the gates in
           wm_move_name_should_draw do. */
        case WM_AOP_DRAW_NAME:
            if (actor && env && env->draw_move_name)
                env->draw_move_name(env->screen_user,
                                    (int)actor->player_side, (int)o->a);
            break;
        /*
         * ANIM.ASM:1287 _ani_shaker -- one operand doing two jobs, "#
         * ticks to shake and power of shake", straight into SHAKER2.
         */
        case WM_AOP_SHAKER:
            if (env && env->screen_shake)
                env->screen_shake(env->screen_user, o->a);
            break;

        /*
         * ANIM.ASM:1962 _ani_shakeall (:55) -- the back rope plus ONE
         * side, chosen by this wrestler's own flip: `btst B_FLIPH` picks
         * LEFT, otherwise RIGHT. Skipped entirely when he is outside the
         * ring (INRING's polarity is the source's; the port's boolean
         * inverts the test), and it thumps.
         */
        case WM_AOP_SHAKEALL: {
            int sel = (int)(o->a & 3);      /* "force a2 into range" */
            if (!actor || !actor->in_ring) break;
            if (!env || !env->rope_command) break;
            env->rope_command(env->rope_user, WM_ROPE_BACK,
                              WM_ROPE_BOUNCE_UD, sel, actor->z_fixed);
            env->rope_command(env->rope_user,
                              (actor->obj_control & WM_OBJ_FLIPH)
                                  ? WM_ROPE_LEFT : WM_ROPE_RIGHT,
                              WM_ROPE_BOUNCE_UD, sel, actor->z_fixed);
            play_sound(env, WM_ROPE_THUMP_SOUND);
            break;
        }

        /*
         * ANIM.ASM:1361 _ani_shakeropes (:36) -- all four banks. Gated on
         * being in the ring AND on reduce_bog, which _ani_shakeall is
         * not.
         */
        case WM_AOP_SHAKEROPES: {
            int sel = (int)(o->a & 3);
            int bank;
            if (!actor || !actor->in_ring) break;
            if (!env || env->reduce_bog || !env->rope_command) break;
            for (bank = WM_ROPE_FRONT; bank <= WM_ROPE_RIGHT; ++bank)
                env->rope_command(env->rope_user, bank, WM_ROPE_BOUNCE_UD,
                                  sel, actor->z_fixed);
            play_sound(env, WM_ROPE_THUMP_SOUND);
            break;
        }

        /*
         * ANIM.ASM:2645 _ani_shakecorner (:77) -- the back rope and
         * whichever SIDE he is on, by X against the ring's centre. No
         * in-ring gate, no sound, and its selector is a fixed 1.
         */
        case WM_AOP_SHAKECORNER:
            if (!actor || !env || !env->rope_command) break;
            env->rope_command(env->rope_user, WM_ROPE_BACK,
                              WM_ROPE_BOUNCE_UD, 1, actor->z_fixed);
            env->rope_command(env->rope_user,
                              (actor->x_int <= WM_RING_X_CENTER)
                                  ? WM_ROPE_LEFT : WM_ROPE_RIGHT,
                              WM_ROPE_BOUNCE_UD, 1, actor->z_fixed);
            break;

        /*
         * ANIM.ASM:4403 _ani_set_idiot -- "Allow players off screen on
         * toss outs", 80 ticks of it.
         */
        case WM_AOP_SET_IDIOT:
            if (env && env->set_allow_offscrn)
                env->set_allow_offscrn(env->screen_user,
                                       WM_ALLOW_OFFSCRN_TICKS);
            break;

        /*
         * ANIM.ASM:4448 _ani_scroll_ctrl -- hand the camera a Y of its
         * own. A NEGATIVE operand sets the flag WITHOUT writing the value
         * (`jrn #cont`), which is how a routine says "keep following what
         * you were following".
         */
        case WM_AOP_SCROLL_CTRL:
            if (!actor) break;
            if (o->a >= 0) actor->scroll_y = o->a;
            actor->status_flags |= (uint32_t)WM_STATUS_SCROLL_CTRL;
            break;

        /*
         * ANIM.ASM:2005 _ani_start_dizzy -- the stars. The operand says
         * WHICH offset slot ("stand, on stomach, on back"), and
         * create_dizzy_proc's own STARS_FLAG makes it once at a time.
         */
        case WM_AOP_START_DIZZY: {
            const wm_dizzy_offset *off;
            int32_t x;
            if (!actor || actor->stars_flag) break;   /* `jrnz #x` */
            if (actor->wrestler_num < 0 ||
                actor->wrestler_num >= WM_DIZZY_ROWS) break;
            if (o->a < 0 || o->a >= WM_DIZZY_SLOTS) break;
            off = &wm_dizzy_offsets[actor->wrestler_num][o->a];
            actor->stars_flag = 1;
            /* `btst B_FLIPH / neg a1` -- the X offset mirrors with him. */
            x = (actor->obj_control & WM_OBJ_FLIPH) ? -off->x : off->x;
            if (env && env->create_dizzy)
                env->create_dizzy(env->screen_user, actor, x, off->y);
            break;
        }

        /*
         * ANIM.ASM:1633 _ani_leapatpos -- jump so as to ARRIVE at the
         * target in `a` ticks, gravity included. The source's own note
         * above it: "user must set TGT_XOFF,YOFF & ZOFF <- these are the
         * actual target", so this reads the offsets some earlier command
         * (ANI_TARGET, set_target_offsets) already put there.
         *
         * X and Z are the plain delta over the tick count. Y solves
         * `y - y0 = v0*t + 0.5*a*t^2` for v0, and is CLAMPED at 0F0000h --
         * fifteen units a tick, which is what stops a long drop turning
         * into a launch. Then, if the flight would exceed `b`, both
         * horizontal velocities are scaled down by maxdist/distance in
         * 8-bit fixed point.
         */
        case WM_AOP_LEAPATPOS: {
            int32_t ticks = o->a, maxdist = o->b;
            int32_t ax, ay, az, dx, dz, dist;
            if (!actor || ticks <= 0) break;

            /* `btst B_FLIPH / neg` on the attack X offset. */
            ax = o->c << 16;
            if (actor->obj_control & WM_OBJ_FLIPH) ax = -ax;
            dx = (actor->tgt_xoff << 16) - (actor->x_fixed + ax);
            actor->x_vel = dx / ticks;

            /* `mpyu a8,a1 / mpyu a0,a1 / srl 1,a1` -- t^2 * gravity / 2,
               all with odd destinations, so plain 32-bit products. */
            ay = o->d << 16;
            {
                int32_t half_at2 =
                    (int32_t)(((uint32_t)(ticks * ticks) *
                               (uint32_t)actor->gravity) >> 1);
                int32_t dy = (actor->tgt_yoff << 16) - (actor->y_fixed - ay);
                int32_t v0 = (int32_t)(((uint32_t)(dy + half_at2)) /
                                       (uint32_t)ticks);
                if (v0 >= WM_LEAP_MAX_YVEL) v0 = WM_LEAP_MAX_YVEL;
                actor->y_vel = v0;
            }

            az = o->e << 16;
            dz = (actor->tgt_zoff << 16) - (actor->z_fixed + az);
            actor->z_vel = dz / ticks;

            /* `abs / srl 16 / mpyu` on each, then square_root of the sum
               -- the distance in whole units, not 16.16. */
            {
                int32_t ux = dx < 0 ? -dx : dx;
                int32_t uz = dz < 0 ? -dz : dz;
                uint32_t sx = (uint32_t)(ux >> 16), sz = (uint32_t)(uz >> 16);
                dist = wm_arcade_square_root(sx * sx + sz * sz);
            }
            /* `cmp a0,a9 / jrgt #ok` -- only scale when the distance is
               at least the maximum. */
            if (dist > 0 && maxdist <= dist) {
                int32_t scale = (int32_t)(((uint32_t)maxdist << 8) /
                                          (uint32_t)dist);
                actor->x_vel = (actor->x_vel * scale) >> 8;
                actor->z_vel = (actor->z_vel * scale) >> 8;
            }
            break;
        }

        /*
         * ANIM.ASM:3838 _ani_slideatopp -- slide along the floor at the
         * man you are aiming at.
         *
         * Most of what it computes it then throws away: it integrates the
         * opponent's velocity forward MAX_TICKS ticks into oppx/oppy/oppz,
         * works out a target X from that, and never uses the answer -- the
         * `move a0,*a10(OANICNT)` that would have consumed it is commented
         * out, and so is the X it built. What actually SHIPS is the
         * INRING-match gate, the target selection, and one velocity.
         */
        case WM_AOP_SLIDEATOPP: {
            wm_arcade_actor_t *opp = env ? env->opponent : NULL;
            if (!actor) break;
            if (actor->smart_target) opp = actor->smart_target;
            if (!opp) break;
            /* "make sure both have the same INRING value" */
            if ((opp->in_ring != 0) != (actor->in_ring != 0)) break;
            if (o->b >= 0)
                wm_arcade_set_target_offsets(actor, opp, (int)o->b);
            actor->x_vel = (actor->facing_dir & WM_MOVE_RIGHT) ? o->a : -o->a;
            break;
        }

        case WM_AOP_CODE: {
            /* ANIM.ASM:1277: an ordinary call, then straight on to the
               next command. A routine this port has not translated leaves
               the op a no-op -- the same thing the flat extractor did by
               dropping the line, except that the name is now in the
               program, where it can be counted. */
            /* The file matters: a '#'-prefixed target is a local label,
               scoped to the sequence file that defines it, and those names
               are reused across files with different bodies behind them. */
            /* ...and `a` says WHICH definition in that file: the same
               local name is defined more than once, with a different body
               each time. */
            (void)wm_anim_code_run(actor, env, o->text, source_file, o->a);
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
/*
 * ANIM.ASM:151. How long a frame is held is NOT simply the tick count in
 * the stream:
 *
 *     move  *a4+,a0            ; the frame's own count
 *     move  *a13(ANI_SPEED),a1
 *     mpyu  a0,a1              ; * ANI_SPEED
 *     srl   8,a1               ; / 256, so 100h is identity
 *     move  @hyper_speed_on,a14
 *     srl   a14,a1             ; the powerup halves it again
 *     move  a1,*a10(OANICNT)
 *
 * Only a FRAME goes through this. ANI_PAUSE (:1252) and ANI_HMBWAIT both
 * write OANICNT directly, unscaled, so neither is affected.
 *
 * Every ANI_SETSPEED in the source drop is 100h and nothing sets
 * hyper_speed, so this is identity for the whole shipped game -- which is
 * exactly why its absence was invisible. It is here so the port is right
 * by construction rather than by coincidence.
 */
#define WM_ANI_SPEED_NORMAL 0x100u

static uint16_t frame_ticks(const wm_arcade_actor_t *actor, int32_t ticks) {
    uint32_t speed = WM_ANI_SPEED_NORMAL;
    uint32_t held;

    if (ticks <= 0) ticks = 1;
    if (actor && actor->ani_speed) speed = actor->ani_speed;
    held = ((uint32_t)ticks * speed) >> 8;
    if (actor && actor->hyper_speed) held >>= actor->hyper_speed;
    /*
     * A count of 0 means "move on next tick" in the source, which its
     * `dec / jrgt` reaches naturally. This interpreter counts down from
     * ticks_left instead, so the floor is one tick. Only reachable when
     * the hyper-speed powerup halves a single-tick frame, which nothing
     * here enables.
     */
    return (uint16_t)(held ? held : 1);
}

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
                exec->ticks_left = frame_ticks(actor, o->a);
                exec->waiting = false;
                return;
            case WM_AOP_CHANGEANIM_TBL: {
                /* :118 writes both OANIBASE and OANIPC, so it is the same
                   hand-off ANI_CHANGEANIM makes -- from a table indexed by
                   the running wrestler's own number. */
                const char *label = actor
                    ? wm_anim_changeanim_label((size_t)o->a,
                                               actor->wrestler_num)
                    : 0;
                if (!label) { pc = pc + 1; continue; }
                exec->become = label;
                exec->ended = true;
                return;
            }
            case WM_AOP_HMBWAIT: {
                /* Blocked wins over hit, and hit over missed -- the order
                   the source checks them in. */
                int32_t ticks = actor
                    ? (actor->hitblocker ? o->c
                       : (actor->anim_mode & WM_MODE_STATUS) ? o->a : o->b)
                    : o->b;
                if (ticks <= 0) { pc = pc + 1; continue; }
                exec->next_pc = pc + 1;
                exec->ticks_left = (uint16_t)ticks;
                return;
            }
            case WM_AOP_WAITRELEASE: {
                int held = actor &&
                    (actor->but_val_cur & (uint16_t)(1u << o->a));
                if (!held) { pc = pc + 1; continue; }
                /* "since we do the flip here, we have to update FACING_DIR
                   too" -- both follow NEW_FACING_DIR while parked. */
                actor->facing_dir = actor->new_facing_dir;
                if (actor->new_facing_dir & WM_MOVE_RIGHT)
                    actor->obj_control &= (uint16_t)~WM_OBJ_FLIPH;
                else
                    actor->obj_control |= (uint16_t)WM_OBJ_FLIPH;
                exec->next_pc = pc;
                exec->ticks_left = 1;
                return;
            }
            case WM_AOP_IF_RPTCOUNT_GE:
                pc = ((int32_t)exec->rpt_count >= o->a)
                    ? (size_t)o->target : pc + 1;
                continue;
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
                                            NULL, 0);
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
            /*
             * ANIM.ASM:2504/:2509 _ani_ifrope / _ani_ifnotrope -- "is
             * there a rope within `b` of me", branching when there is (or
             * when there is not).
             *
             * Two details that are easy to lose. The MODE's HIGH byte
             * chooses WHOSE position is measured (RC_OPPONENT), and the
             * LOW byte which rope: RC_FRONT by his facing, RC_BACK the
             * other way, RC_EITHER by which half of the ring he is in.
             *
             * And the out-of-the-ring case is NOT symmetric: `move
             * *a13(INRING),a0 / jrnz #definitly_too_far` jumps PAST the
             * inversion, so an IFNOTROPE outside the ring falls through
             * rather than branching. That is the source's own asymmetry,
             * not a simplification here. (INRING's polarity is the
             * source's; the port stores the boolean, so the test flips.)
             */
            case WM_AOP_IFROPE:
            case WM_AOP_IFNOTROPE: {
                const wm_arcade_actor_t *who = actor;
                int invert = (o->op == WM_AOP_IFNOTROPE);
                int right, close;
                int32_t rope_x, dist;

                if (!actor || !actor->in_ring) {
                    pc = pc + 1;            /* `#definitly_too_far` */
                    continue;
                }
                if ((o->a >> 8) != 0) {     /* RC_OPPONENT */
                    who = actor->smart_target ? actor->smart_target
                        : (exec->env ? exec->env->opponent : NULL);
                    if (!who) { pc = pc + 1; continue; }
                }
                switch (o->a & 0xFF) {
                case 0:                     /* RC_FRONT */
                    right = (who->facing_dir & WM_MOVE_RIGHT) != 0;
                    break;
                case 1:                     /* RC_BACK */
                    right = (who->facing_dir & WM_MOVE_LEFT) != 0;
                    break;
                default:                    /* RC_EITHER: the nearer side */
                    right = who->x_int > WM_RING_X_CENTER;
                    break;
                }
                rope_x = wm_ring_calc_line_x(
                    wm_ring_boundary_seed(right ? WM_RING_BOUNDARY_RIGHT_ROPE
                                                : WM_RING_BOUNDARY_LEFT_ROPE),
                    who->z_int);
                dist = who->x_int - rope_x;
                if (dist < 0) dist = -dist;         /* `abs a1` */
                close = dist <= o->b;               /* `jrle #close_enough` */
                pc = (close != invert) ? (size_t)o->target : pc + 1;
                continue;
            }
            /*
             * ANIM.ASM:1568 _ani_loop -- the pin hold. It parks on its
             * frame forever (OANICNT 1 and `rets`, so the program counter
             * never moves), and on the way it does one other thing: if
             * this wrestler is PINNED and announce_rnd_winner is asleep at
             * arw_bwait waiting to see whether anyone bucks off, it wakes
             * it now. The p1rounds/p2rounds test above that is the
             * match-over case, where the round is already decided.
             */
            case WM_AOP_LOOP:
                if (actor && (actor->status_flags & WM_STATUS_PINNED) &&
                    exec->env && exec->env->wake_round_announce)
                    exec->env->wake_round_announce(exec->env->screen_user);
                /* It stops the counter without touching OANIPC, so the
                   frame already showing keeps showing -- exec->pc is left
                   where the last frame op put it. */
                exec->next_pc = pc;
                exec->ticks_left = 1;
                exec->waiting = true;
                return;
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
                /*
                 * ANIM.ASM lets an ANI_CODE routine write the program
                 * counter itself -- HRTSEQ3.ASM's `#rope_check` does
                 * `movi #stand,a14 / move a14,*a13(ANIPC),L`. ANIPC is a
                 * field on the wrestler, so the routine sets it and the
                 * interpreter picks it up the moment it returns.
                 */
                if (actor && actor->anipc_label) {
                    const char *want_prog = actor->anipc_program;
                    const char *want_label = actor->anipc_label;
                    int at = -1;
                    if (!want_prog || (p && p->source_label &&
                            strcmp(want_prog, p->source_label) == 0))
                        at = wm_anim_program_label(p, want_label);
                    if (at >= 0) {
                        actor->anipc_program = 0;
                        actor->anipc_label = 0;
                        pc = (size_t)at;
                        continue;
                    }
                    /*
                     * The label lives in ANOTHER routine, and that is not
                     * a mistake: the assembler resolves it to a flat
                     * address and the machine runs from there, so it is a
                     * jump into the middle of a different animation. This
                     * hands it over as a `become` of that program, and
                     * leaves ANIPC set so wm_anim_exec_start enters at
                     * the label rather than the top.
                     */
                    if (want_prog && wm_anim_program_find(want_prog)) {
                        exec->become = want_prog;
                        exec->ended = true;
                        return;
                    }
                    actor->anipc_program = 0;
                    actor->anipc_label = 0;
                }
                pc += 1;
                continue;
        }
    }
    exec->ended = true;
}

int wm_anim_program_label(const wm_anim_program *program, const char *label) {
    size_t i;
    if (!program || !label || !program->labels) return -1;
    for (i = 0; i < program->label_count; ++i)
        if (strcmp(program->labels[i].name, label) == 0)
            return (int)program->labels[i].at;
    return -1;
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
    /* Not always op 0: a routine that branches back into shared code
       earlier in the file has that code at the head of its stream, and
       its own first command is `entry` ops in. See wm_anim_program. */
    /*
     * Not always op 0. Two reasons: a routine that branches back into
     * shared code carries it at the head of its stream (`entry`), and an
     * ANIPC write can hand this animation over to be entered at a named
     * label instead of its start.
     */
    {
        size_t at = program->entry < program->op_count ? program->entry : 0;
        if (actor && actor->anipc_label) {
            int want = wm_anim_program_label(program, actor->anipc_label);
            if (want >= 0 &&
                (!actor->anipc_program || (program->source_label &&
                 strcmp(actor->anipc_program, program->source_label) == 0))) {
                at = (size_t)want;
                actor->anipc_program = 0;
                actor->anipc_label = 0;
            }
        }
        advance(exec, actor, round_tickcount, at);
    }
    /* The loop shows a frame before consuming a tick of it, same as
       wm_visual_start's own just_started. */
    exec->just_started = true;
}

void wm_anim_exec_tick(wm_anim_exec *exec, wm_arcade_actor_t *actor,
                       uint16_t round_tickcount) {
    if (!exec || exec->ended || !exec->program) return;
    /*
     * OANICNT written from outside -- SHNSEQ3.ASM's #pause_opp stuffing a
     * hold onto the man it just hit. Applied here rather than by the
     * writer, because the counter belongs to whatever this exec is
     * currently showing.
     */
    if (actor && actor->anicnt_override) {
        exec->ticks_left = actor->anicnt_override;
        actor->anicnt_override = 0u;
    }
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
