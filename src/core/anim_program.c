#include "wm/anim_program.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wm_arcade_anim_combat.h"
#include "wm/arcade/wm_arcade_butcount.h"

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
                        uint16_t round_tickcount) {
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
        case WM_AOP_CLR_STATUS:
            actor->anim_mode &= (uint16_t)~WM_MODE_STATUS;
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
        default:
            /* WM_AOP_UNTRANSLATED and anything needing a subsystem this
               port does not have: carried in the program so it stays a
               faithful record, executed as a no-op. */
            break;
    }
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
            /* ANIM.ASM:3214 `cmp a0,a14 / jrlt #fail`: the branch is taken
               when the count is at least the operand -- and :3239's
               `jrge #fail2` is its exact complement. */
            case WM_AOP_IF_BUTCOUNT_GE:
                pc = (actor &&
                      wm_arcade_button_count(actor, (int)o->a) >= o->b)
                    ? (size_t)o->target : pc + 1;
                continue;
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
                if (actor) run_command(o, actor, round_tickcount);
                pc += 1;
                continue;
        }
    }
    exec->ended = true;
}

void wm_anim_exec_start(wm_anim_exec *exec, const wm_anim_program *program,
                        wm_arcade_actor_t *actor, uint16_t round_tickcount) {
    if (!exec) return;
    memset(exec, 0, sizeof(*exec));
    exec->program = program;
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
