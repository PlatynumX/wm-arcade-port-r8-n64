#include "wm/arcade/wm_arcade_special.h"
#include "wm/arcade/wm_arcade_damage.h"

#include <stdlib.h>
#include <string.h>

#define FX16(n) ((int32_t)((n) * 65536))

enum {
    WM_SP_PHASE_NONE = 0,
    WM_SP_PHASE_REAPER_GROW,
    WM_SP_PHASE_REAPER_PRECOLL,
    WM_SP_PHASE_SALT_GROW,
    WM_SP_PHASE_SALT_LIVE
};

static int32_t fixed_int(int32_t v) { return v >> 16; }

static wm_arcade_special_obj_t **head_for_side(wm_arcade_special_lists_t *lists,
                                                int side)
{
    if (!lists) return NULL;
    if (side < 0) return &lists->neutral;
    if (side == 0) return &lists->p1;
    return &lists->p2;
}

void wm_arcade_special_lists_init(wm_arcade_special_lists_t *lists)
{
    if (!lists) return;
    lists->p1 = NULL;
    lists->p2 = NULL;
    lists->neutral = NULL;
}

void wm_arcade_special_obj_init(wm_arcade_special_obj_t *obj)
{
    if (!obj) return;
    memset(obj, 0, sizeof(*obj));
    obj->player_side = WM_SP_SIDE_NEUTRAL;
}

static void reset_for_spawn_preserve_id(wm_arcade_special_obj_t *obj)
{
    int16_t inherited_id = obj->id;
    memset(obj, 0, sizeof(*obj));
    obj->player_side = WM_SP_SIDE_NEUTRAL;
    obj->id = inherited_id;
}

void wm_arcade_special_insert(wm_arcade_special_lists_t *lists,
                              wm_arcade_special_obj_t *obj)
{
    wm_arcade_special_obj_t **head;
    if (!lists || !obj) return;
    head = head_for_side(lists, obj->player_side);
    if (!head) return;
    obj->next = *head;
    *head = obj;
    obj->in_list = 1;
}

void wm_arcade_special_delete(wm_arcade_special_lists_t *lists,
                              wm_arcade_special_obj_t *obj)
{
    wm_arcade_special_obj_t **head;
    wm_arcade_special_obj_t *cur, *prev = NULL;
    if (!lists || !obj) return;
    head = head_for_side(lists, obj->player_side);
    if (!head) return;
    cur = *head;
    while (cur) {
        if (cur == obj) {
            if (prev) prev->next = cur->next;
            else *head = cur->next;
            obj->next = NULL;
            obj->in_list = 0;
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

const char *wm_arcade_special_anim_source_name(wm_arcade_special_anim_t anim)
{
    switch (anim) {
    case WM_SP_ANIM_PIE: return "pie_anim";
    case WM_SP_ANIM_FIREBALL: return "fireball_anim";
    case WM_SP_ANIM_SPIRIT: return "spirit_anim";
    case WM_SP_ANIM_REAPER_GROW: return "reaper_grow";
    case WM_SP_ANIM_REAPER: return "reaper_anim";
    case WM_SP_ANIM_SALT_GROW: return "salt_grow";
    case WM_SP_ANIM_SALT: return "salt_anim";
    case WM_SP_ANIM_SPIRIT_SPLAT: return "spiritsplat_anim";
    case WM_SP_ANIM_REAPER_SPLAT: return "reapersplat_anim";
    case WM_SP_ANIM_SALT_SPLAT: return "saltsplat_anim";
    case WM_SP_ANIM_FIRE_SPLAT: return "firesplat_anim";
    case WM_SP_ANIM_PIE_SPLAT: return "piesplat_anim";
    default: return NULL;
    }
}

static void set_collbox(wm_arcade_special_obj_t *obj,
                        int xoff, int width, int yoff, int height,
                        int zoff, int depth)
{
    obj->xoff = (int16_t)xoff;
    obj->width = (int16_t)width;
    obj->yoff = (int16_t)yoff;
    obj->height = (int16_t)height;
    obj->zoff = (int16_t)zoff;
    obj->depth = (int16_t)depth;
}

void wm_arcade_special_change_anim(wm_arcade_special_obj_t *obj,
                                   wm_arcade_special_anim_t anim)
{
    if (!obj) return;
    obj->anim = anim;
    obj->anim_count = 1;
    obj->source_phase = WM_SP_PHASE_NONE;
    obj->source_phase_ticks = 0;

    /* Immediate non-image opcodes executed by sp_change_anim -> sp_animate. */
    switch (anim) {
    case WM_SP_ANIM_PIE:
    case WM_SP_ANIM_FIREBALL:
        set_collbox(obj, -10, 20, -8, 16, -10, 20);
        break;
    case WM_SP_ANIM_SPIRIT:
        set_collbox(obj, 0, 10, -8, 16, -10, 20);
        break;
    case WM_SP_ANIM_REAPER_GROW:
        set_collbox(obj, 0, 10, -8, 16, -1000, 20);
        /* Five WL 1 frames precede ASP_CODE set_xv. */
        obj->source_phase = WM_SP_PHASE_REAPER_GROW;
        obj->source_phase_ticks = 5;
        break;
    case WM_SP_ANIM_SALT_GROW:
        set_collbox(obj, 0, 10, -8, 16, -1000, 20);
        /* One WL 1 frame precedes ASP_CODE set_xv. */
        obj->source_phase = WM_SP_PHASE_SALT_GROW;
        obj->source_phase_ticks = 1;
        break;
    case WM_SP_ANIM_SALT:
        set_collbox(obj, 0, 10, -8, 16, -40, 80);
        /* SALT01 remains collision-live for exactly WL 20. */
        obj->source_phase = WM_SP_PHASE_SALT_LIVE;
        obj->source_phase_ticks = 20;
        break;
    case WM_SP_ANIM_SPIRIT_SPLAT:
    case WM_SP_ANIM_REAPER_SPLAT:
        obj->x_vel = obj->y_vel = obj->z_vel = 0;
        break;
    case WM_SP_ANIM_SALT_SPLAT:
        obj->gravity = 0;
        set_collbox(obj, 0, 10, -8, 16, -1000, 20);
        break;
    case WM_SP_ANIM_FIRE_SPLAT:
    case WM_SP_ANIM_PIE_SPLAT:
        obj->x_vel = obj->y_vel = obj->z_vel = 0;
        break;
    default:
        break;
    }
}

void wm_arcade_special_set_boxes(wm_arcade_special_obj_t *obj)
{
    int32_t x, y, z;
    if (!obj) return;
    x = fixed_int(obj->x_fixed);
    y = fixed_int(obj->y_fixed);
    z = fixed_int(obj->z_fixed);

    obj->collision_box.y1 = y + obj->yoff;
    obj->collision_box.y2 = obj->collision_box.y1 + obj->height;
    obj->collision_box.z1 = z + obj->zoff;
    obj->collision_box.z2 = obj->collision_box.z1 + obj->depth;

    if (obj->obj_control & WM_OBJ_FLIPH) {
        obj->collision_box.x2 = x - obj->xoff;
        obj->collision_box.x1 = obj->collision_box.x2 - obj->width;
    } else {
        obj->collision_box.x1 = x + obj->xoff;
        obj->collision_box.x2 = obj->collision_box.x1 + obj->width;
    }
}

static void set_boxes_list(wm_arcade_special_obj_t *obj)
{
    while (obj) {
        wm_arcade_special_set_boxes(obj);
        obj = obj->next;
    }
}

void wm_arcade_special_set_all_boxes(wm_arcade_special_lists_t *lists)
{
    if (!lists) return;
    set_boxes_list(lists->p1);
    set_boxes_list(lists->p2);
    set_boxes_list(lists->neutral);
}

void wm_arcade_special_velocity_add(wm_arcade_special_obj_t *obj)
{
    if (!obj) return;
    obj->x_fixed += obj->x_vel;
    obj->y_vel -= obj->gravity;
    obj->y_fixed += obj->y_vel;
    obj->z_fixed += obj->z_vel;
}

void wm_arcade_special_standard_bounce(wm_arcade_special_obj_t *obj)
{
    if (!obj || obj->y_vel >= 0) return;
    if (obj->ground_y_fixed <= obj->y_fixed) return;
    obj->y_fixed = obj->ground_y_fixed;
    obj->y_vel = -(obj->y_vel >> 1);
}

static void spawn_common(wm_arcade_special_lists_t *lists,
                         wm_arcade_special_obj_t *obj,
                         wm_arcade_actor_t *owner,
                         wm_arcade_special_kind_t kind,
                         int id_is_explicit, int id,
                         int xoff, int xvel, int yoff,
                         int32_t gravity, int32_t yvel,
                         wm_arcade_special_anim_t anim)
{
    int sign = 1;
    if (!lists || !obj || !owner) return;

    /*
     * GETPRC/XFERPROC does not clear recycled PDATA.  doink_pie and
     * bam_fireball also never write SP_ID, so preserve the slot's prior ID.
     * Callers cold-initialize each process slot once with special_obj_init().
     */
    reset_for_spawn_preserve_id(obj);
    if (id_is_explicit) obj->id = (int16_t)id;
    obj->kind = kind;
    obj->owner = owner;
    obj->player_side = (int16_t)owner->player_side;
    if (owner->obj_control & WM_OBJ_FLIPH) {
        obj->obj_control |= WM_OBJ_FLIPH;
        sign = -1;
    }
    obj->x_fixed = owner->x_fixed + sign * FX16(xoff);
    obj->x_vel = sign * FX16(xvel);
    obj->z_fixed = owner->z_fixed;
    obj->y_fixed = owner->y_fixed + FX16(yoff);
    obj->in_ring = (int16_t)owner->in_ring;
    obj->ground_y_fixed = owner->ground_y << 16;
    obj->gravity = gravity;
    obj->y_vel = yvel;
    obj->z_vel = 0;
    obj->die = 0;
    wm_arcade_special_insert(lists, obj);
    wm_arcade_special_change_anim(obj, anim);
    wm_arcade_special_set_boxes(obj);
}

void wm_arcade_spawn_doink_pie(wm_arcade_special_lists_t *lists,
                               wm_arcade_special_obj_t *obj,
                               wm_arcade_actor_t *owner)
{
    spawn_common(lists, obj, owner, WM_SP_KIND_DOINK_PIE, 0, 0,
                 86, 6, 97, 0, 0, WM_SP_ANIM_PIE);
}

void wm_arcade_spawn_bam_fireball(wm_arcade_special_lists_t *lists,
                                  wm_arcade_special_obj_t *obj,
                                  wm_arcade_actor_t *owner)
{
    spawn_common(lists, obj, owner, WM_SP_KIND_BAM_FIREBALL, 0, 0,
                 86, 6, 97, 0, 0, WM_SP_ANIM_FIREBALL);
}

void wm_arcade_spawn_taker_spirit(wm_arcade_special_lists_t *lists,
                                  wm_arcade_special_obj_t *obj,
                                  wm_arcade_actor_t *owner)
{
    spawn_common(lists, obj, owner, WM_SP_KIND_TAKER_SPIRIT, 1, WM_SP_ID_SPIRIT,
                 32, 7, 0x36, 0, 0, WM_SP_ANIM_SPIRIT);
}

void wm_arcade_spawn_taker_reaper(wm_arcade_special_lists_t *lists,
                                  wm_arcade_special_obj_t *obj,
                                  wm_arcade_actor_t *owner)
{
    spawn_common(lists, obj, owner, WM_SP_KIND_TAKER_REAPER, 1, WM_SP_ID_REAPER,
                 2, 4, 0x2e, 0, 0, WM_SP_ANIM_REAPER_GROW);
}

void wm_arcade_spawn_yoko_salt(wm_arcade_special_lists_t *lists,
                               wm_arcade_special_obj_t *obj,
                               wm_arcade_actor_t *owner)
{
    spawn_common(lists, obj, owner, WM_SP_KIND_YOKO_SALT, 1, WM_SP_ID_SALT,
                 0x36, 7, 0x5b, 0x4800, 0x30000, WM_SP_ANIM_SALT_GROW);
}

void wm_arcade_special_reaper_finish_grow(wm_arcade_special_obj_t *obj)
{
    if (!obj) return;
    /* reaper_grow: ASP_CODE set_xv, then fall through to reaper_anim. */
    obj->x_vel = (obj->obj_control & WM_OBJ_FLIPH) ? -FX16(7) : FX16(7);
    obj->anim = WM_SP_ANIM_REAPER;
    /* reaper_anim begins with two WL 3 frames BEFORE its COLLBOX opcode. */
    set_collbox(obj, 0, 10, -8, 16, -1000, 20);
    obj->source_phase = WM_SP_PHASE_REAPER_PRECOLL;
    obj->source_phase_ticks = 6;
    wm_arcade_special_set_boxes(obj);
}

void wm_arcade_special_reaper_enable_collision(wm_arcade_special_obj_t *obj)
{
    if (!obj) return;
    set_collbox(obj, 0, 10, -8, 16, -10, 20);
    obj->source_phase = WM_SP_PHASE_NONE;
    obj->source_phase_ticks = 0;
    wm_arcade_special_set_boxes(obj);
}

void wm_arcade_special_salt_become_live(wm_arcade_special_obj_t *obj)
{
    if (!obj) return;
    /* salt_grow: ASP_CODE set_xv, then salt_anim COLLBOX executes immediately. */
    obj->x_vel = (obj->obj_control & WM_OBJ_FLIPH) ? -FX16(7) : FX16(7);
    obj->anim = WM_SP_ANIM_SALT;
    set_collbox(obj, 0, 10, -8, 16, -40, 80);
    obj->source_phase = WM_SP_PHASE_SALT_LIVE;
    obj->source_phase_ticks = 20;
    wm_arcade_special_set_boxes(obj);
}

void wm_arcade_special_salt_disable_collision(wm_arcade_special_obj_t *obj)
{
    if (!obj) return;
    /* salt_anim: ASP_ZEROVELS, ASP_SET_GRAV 0, then collision-off box. */
    obj->x_vel = obj->y_vel = obj->z_vel = 0;
    obj->gravity = 0;
    set_collbox(obj, 0, 10, -8, 16, -1000, 20);
    obj->source_phase = WM_SP_PHASE_NONE;
    obj->source_phase_ticks = 0;
    wm_arcade_special_set_boxes(obj);
}

void wm_arcade_special_tick_source_state(wm_arcade_special_obj_t *obj)
{
    if (!obj || obj->source_phase_ticks == 0) return;

    obj->source_phase_ticks--;
    if (obj->source_phase_ticks != 0) return;

    switch (obj->source_phase) {
    case WM_SP_PHASE_REAPER_GROW:
        wm_arcade_special_reaper_finish_grow(obj);
        break;
    case WM_SP_PHASE_REAPER_PRECOLL:
        wm_arcade_special_reaper_enable_collision(obj);
        break;
    case WM_SP_PHASE_SALT_GROW:
        wm_arcade_special_salt_become_live(obj);
        break;
    case WM_SP_PHASE_SALT_LIVE:
        wm_arcade_special_salt_disable_collision(obj);
        break;
    default:
        obj->source_phase = WM_SP_PHASE_NONE;
        break;
    }
}

static void r1_anim(wm_arcade_actor_t *victim,
                    wm_arcade_react1_anim_group_t group,
                    const wm_arcade_special_callbacks_t *callbacks)
{
    const wm_arcade_react1_callbacks_t *cb;
    if (!callbacks || !callbacks->react1) return;
    cb = callbacks->react1->callbacks;
    if (cb && cb->change_anim) cb->change_anim(victim, group, cb->user);
}

static void r1_sound(wm_arcade_actor_t *victim,
                     wm_arcade_react1_sound_t sound,
                     const wm_arcade_special_callbacks_t *callbacks)
{
    const wm_arcade_react1_callbacks_t *cb;
    if (!callbacks || !callbacks->react1) return;
    cb = callbacks->react1->callbacks;
    if (cb && cb->play_sound) cb->play_sound(victim, sound, cb->user);
}

static void adjust_health(wm_arcade_actor_t *victim, int16_t delta,
                          wm_arcade_actor_t *owner,
                          const wm_arcade_special_callbacks_t *callbacks)
{
    if (callbacks && callbacks->react && callbacks->react->adjust_health)
        callbacks->react->adjust_health(victim, delta, owner,
                                        callbacks->react->user);
}

static void setmode_normal(wm_arcade_actor_t *victim)
{
    if (victim->player_mode != WM_PMODE_DEAD)
        victim->player_mode = WM_PMODE_NORMAL;
}

static wm_arcade_special_anim_t react_splat_for_id(int id)
{
    if (id == WM_SP_ID_REAPER) return WM_SP_ANIM_REAPER_SPLAT;
    if (id == WM_SP_ID_SALT) return WM_SP_ANIM_SALT_SPLAT;
    /* REACT1 wrestler_hit_special explicitly range-checks and falls back. */
    return WM_SP_ANIM_SPIRIT_SPLAT;
}

static int special_hit_splat_for_valid_id(int id, wm_arcade_special_anim_t *out)
{
    if (!out) return 0;
    switch (id) {
    case WM_SP_ID_SPIRIT: *out = WM_SP_ANIM_SPIRIT_SPLAT; return 1;
    case WM_SP_ID_REAPER: *out = WM_SP_ANIM_REAPER_SPLAT; return 1;
    case WM_SP_ID_SALT: *out = WM_SP_ANIM_SALT_SPLAT; return 1;
    default: return 0;
    }
}

int wm_arcade_wrestler_hit_special(wm_arcade_special_lists_t *lists,
                                   wm_arcade_special_obj_t *obj,
                                   wm_arcade_actor_t *victim,
                                   wm_arcade_combat_runtime_t *runtime,
                                   const wm_arcade_special_callbacks_t *callbacks)
{
    wm_arcade_actor_t *owner;
    int32_t xv;
    int32_t distance;
    if (!lists || !obj || !victim || !runtime || !obj->owner) return 0;
    owner = obj->owner;

    owner->last_hit_time = runtime->pcnt;

    /* REACT1 calls the shared hit_stuff with the projectile-process identity. */
    wm_arcade_hit_stuff_identity(obj, 0, victim,
                                 callbacks ? callbacks->react : NULL);

    if (obj->id == WM_SP_ID_SALT) {
        if (victim->player_mode == WM_PMODE_BLOCK) {
            /*
             * Preserve the executable source, not its comment: REACT1 loads 1
             * into a14 but stores a4.  In COLLIS.ASM's call path a4 still
             * holds SP_COLLZ1, so the direct port writes that value.
             */
            owner->usr_var2 = obj->collision_box.z1;
        } else {
            r1_sound(victim, WM_R1_SND_SCREAM, callbacks);
            adjust_health(victim, -WM_D_SALT, owner, callbacks);
        }

        wm_arcade_special_change_anim(obj, WM_SP_ANIM_SALT_SPLAT);
        wm_arcade_special_set_boxes(obj);

        if (victim->player_mode == WM_PMODE_BLOCK) {
            victim->x_vel = fixed_int(obj->x_fixed) < victim->x_int ? FX16(3) : -FX16(3);
            r1_sound(victim, WM_R1_SND_BLOCK, callbacks);
            r1_anim(victim, WM_R1_ANIM_HITBLOCK, callbacks);
            return 1;
        }

        if (victim->life == 0) return 1;
        setmode_normal(victim);
        victim->z_fixed = owner->z_fixed - FX16(1);
        victim->z_int = fixed_int(victim->z_fixed);
        r1_anim(victim, WM_R1_ANIM_SPECIAL_HEAD_HIT2_SAND, callbacks);
        victim->usr_var1 = 0;
        victim->delay_meter = 8 * 60;
        victim->x_vel = (victim->facing_dir & WM_MOVE_RIGHT) ? -FX16(1) : FX16(1);
        victim->y_vel = FX16(3);
        victim->z_vel = 0x7000;
        obj->z_vel = 0x7000;
        obj->x_vel >>= 1;
        return 1;
    }

    if (victim->player_mode != WM_PMODE_BLOCK) {
        if (obj->id == WM_SP_ID_SPIRIT) {
            victim->immobilize_time = 60;
        } else {
            /* Source comment says 2; executable code loads -3. */
            adjust_health(victim, -3, owner, callbacks);
        }
    }

    wm_arcade_special_delete(lists, obj);
    wm_arcade_special_change_anim(obj, react_splat_for_id(obj->id));
    wm_arcade_special_set_boxes(obj);

    if (victim->player_mode == WM_PMODE_BLOCK) {
        victim->x_vel = fixed_int(obj->x_fixed) < victim->x_int ? FX16(2) : -FX16(2);
        r1_sound(victim, WM_R1_SND_BLOCK, callbacks);
        r1_anim(victim, WM_R1_ANIM_HITBLOCK, callbacks);
        return 1;
    }

    if (victim->player_mode == WM_PMODE_INAIR || victim->life == 0)
        return 1;

    setmode_normal(victim);
    victim->z_fixed = owner->z_fixed - FX16(1);
    victim->z_int = fixed_int(victim->z_fixed);
    r1_anim(victim, WM_R1_ANIM_SPECIAL_BODY_HIT2, callbacks);
    victim->usr_var1 = 0;

    xv = -FX16(3);
    if (obj->id == WM_SP_ID_SPIRIT) {
        victim->usr_var1 = 1;
        victim->delay_meter = 10 * 60;
        distance = owner->x_int - victim->x_int;
        if (distance < 0) distance = -distance;
        xv = 0;
        if (distance >= 0x5c) xv = FX16(4);
    }
    if (!(victim->facing_dir & WM_MOVE_RIGHT)) xv = -xv;
    victim->x_vel = xv;
    victim->y_vel = FX16(3);
    victim->z_vel = 0;
    r1_sound(victim, WM_R1_SND_SCREAM, callbacks);
    return 1;
}

int wm_arcade_special_hit(wm_arcade_special_lists_t *lists,
                          wm_arcade_special_obj_t *a,
                          wm_arcade_special_obj_t *b)
{
    wm_arcade_special_anim_t anim;
    if (!lists || !a || !b) return 0;

    /* SPECIAL.ASM deletes the first object before its unchecked table read. */
    wm_arcade_special_delete(lists, a);
    if (!special_hit_splat_for_valid_id(a->id, &anim)) {
        a->source_unchecked_splat_id = 1;
        return 0;
    }
    wm_arcade_special_change_anim(a, anim);
    wm_arcade_special_set_boxes(a);

    /* Then it repeats the same unchecked lookup for the second object. */
    wm_arcade_special_delete(lists, b);
    if (!special_hit_splat_for_valid_id(b->id, &anim)) {
        b->source_unchecked_splat_id = 1;
        return 0;
    }
    wm_arcade_special_change_anim(b, anim);
    wm_arcade_special_set_boxes(b);
    return 1;
}

static int boxes_overlap(const wm_arcade_box3_t *a, const wm_arcade_box3_t *b)
{
    return b->x2 >= a->x1 && b->x1 <= a->x2 &&
           b->y2 >= a->y1 && b->y1 <= a->y2 &&
           b->z2 >= a->z1 && b->z1 <= a->z2;
}

static int object_lists_collide(wm_arcade_special_lists_t *lists,
                                wm_arcade_special_obj_t *a,
                                wm_arcade_special_obj_t *b,
                                wm_arcade_special_collision_result_t *out)
{
    wm_arcade_special_obj_t *aa, *bb;
    for (aa = a; aa; aa = aa->next) {
        for (bb = b; bb; bb = bb->next) {
            if (boxes_overlap(&aa->collision_box, &bb->collision_box)) {
                int ok;
                out->object_object_hit = 1;
                out->last_object = aa;
                ok = wm_arcade_special_hit(lists, aa, bb);
                if (!ok) out->unresolved_unchecked_splat_id = 1;
                return 1;
            }
        }
    }
    return 0;
}

static int object_list_hits_players(wm_arcade_special_lists_t *lists,
                                    wm_arcade_special_obj_t *head,
                                    wm_arcade_actor_t **actors,
                                    size_t actor_count,
                                    wm_arcade_combat_runtime_t *runtime,
                                    const wm_arcade_special_callbacks_t *callbacks,
                                    wm_arcade_special_collision_result_t *out)
{
    wm_arcade_special_obj_t *obj = head;
    while (obj) {
        wm_arcade_special_obj_t *next = obj->next;
        size_t i;
        for (i = 0; i < actor_count; ++i) {
            wm_arcade_actor_t *victim = actors ? actors[i] : NULL;
            if (!victim || !victim->active) continue;
            if (victim->anim_mode & WM_MODE_NOCOLLIS) continue;
            if (boxes_overlap(&obj->collision_box, &victim->hurt_box)) {
                if (wm_arcade_wrestler_hit_special(lists, obj, victim,
                                                   runtime, callbacks)) {
                    out->wrestler_hits++;
                    out->last_object = obj;
                    out->last_victim = victim;
                    return 1; /* source exits this entire object list after first hit */
                }
            }
        }
        obj = next;
    }
    return 0;
}

wm_arcade_special_collision_result_t wm_arcade_object_collisions(
    wm_arcade_special_lists_t *lists,
    wm_arcade_actor_t **actors,
    size_t actor_count,
    wm_arcade_combat_runtime_t *runtime,
    const wm_arcade_special_callbacks_t *callbacks)
{
    wm_arcade_special_collision_result_t out;
    memset(&out, 0, sizeof(out));
    if (!lists || !runtime) return out;

    wm_arcade_special_set_all_boxes(lists);
    (void)object_lists_collide(lists, lists->p1, lists->p2, &out);

    /* Reload list heads after possible deletions, exactly like object_collisions. */
    (void)object_list_hits_players(lists, lists->p1, actors, actor_count,
                                   runtime, callbacks, &out);
    (void)object_list_hits_players(lists, lists->p2, actors, actor_count,
                                   runtime, callbacks, &out);
    (void)object_list_hits_players(lists, lists->neutral, actors, actor_count,
                                   runtime, callbacks, &out);
    return out;
}
