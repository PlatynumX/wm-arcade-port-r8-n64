#include "wm_arcade_drone.h"
#include "wm_arcade_drone_source_tables.h"
#include <string.h>

#define DRN_PUNCH WM_BTN_PUNCH
#define DRN_BLOCK WM_BTN_BLOCK

static const uint8_t s_getup_pct[30] = {
    10,12,14,16,18, 20,22,24,26,28, 30,32,34,36,38,
    40,42,44,46,48, 50,52,54,56,58, 60,70,80,90,100
};

static uint32_t rnd_plain(const wm_arcade_drone_callbacks_t *cb, uint32_t maxv) {
    /* DRONE.ASM's rnd and rndrng0 are distinct source services.  Never
       substitute one for the other merely because both return bounded values. */
    if (!cb || !cb->rnd_upto) return 0;
    return cb->rnd_upto(maxv, cb->user);
}

static uint32_t rnd_range(const wm_arcade_drone_callbacks_t *cb, uint32_t maxv) {
    if (!cb || !cb->rndrng0_upto) return 0;
    return cb->rndrng0_upto(maxv, cb->user);
}

void wm_arcade_drone_init(wm_arcade_drone_state_t *d, int skill) {
    if (!d) return;
    memset(d, 0, sizeof(*d));
    d->skill = skill;
}

int wm_arcade_drone_getup_pct(int skill) {
    if (skill < 0) skill = 0;
    if (skill > 29) skill = 29;
    /* V13e-c1 source generation validates getup_t against this already-audited
       direct port.  Use the fetched DRONE.ASM table when present. */
    if (wm_arcade_drone_source_tables_ready())
        return (int)wm_arcade_drone_source_getup_pct(skill);
    return s_getup_pct[skill];
}

void wm_arcade_drone_commit_inputs(wm_arcade_actor_t *a,
                                   wm_arcade_drone_state_t *d,
                                   uint16_t old_but,
                                   uint16_t old_joy) {
    uint16_t now_but, x, now_joy;
    if (!a || !d) return;
    now_but = (uint16_t)(d->but | d->but_charge);
    d->but = now_but;
    x = (uint16_t)(now_but ^ old_but);
    d->but_down = (uint16_t)(x & now_but);
    d->but_up = (uint16_t)(x & old_but);
    now_joy = d->joy;
    x = (uint16_t)(now_joy ^ old_joy);
    d->joy_down = (uint16_t)(x & now_joy);
    d->joy_up = (uint16_t)(x & old_joy);

    a->but_val_cur = d->but;
    a->but_val_down = d->but_down;
    a->but_val_up = d->but_up;
    a->stick_val_cur = d->joy;
    a->stick_val_down = d->joy_down;
    a->stick_val_up = d->joy_up;
}

static int same_target_rank(wm_arcade_actor_t *self,
                            wm_arcade_actor_t *opp,
                            const wm_arcade_drone_world_t *w,
                            const wm_arcade_drone_callbacks_t *cb,
                            int *alive_team) {
    int rank = 0, alive = 0;
    size_t i;
    if (!w || !w->actors) {
        if (alive_team) *alive_team = 1;
        return 0;
    }
    for (i = 0; i < w->actor_count; ++i) {
        wm_arcade_actor_t *p = w->actors[i];
        wm_arcade_actor_t *po;
        int32_t pd;
        if (!p || !p->active || p->player_side != self->player_side ||
            p->player_mode == WM_PMODE_DEAD) continue;
        ++alive;
        po = cb && cb->closest_actor_for ? cb->closest_actor_for(p, cb->user) : p->smart_target;
        if (po != opp) continue;
        pd = cb && cb->closest_dist_for ? cb->closest_dist_for(p, cb->user) : p->closest_dist;
        if (self->closest_dist > pd) ++rank;
    }
    if (alive_team) *alive_team = alive;
    return rank;
}

static void select_script(wm_arcade_actor_t *self,
                          wm_arcade_drone_state_t *d,
                          const char *label,
                          const wm_arcade_drone_callbacks_t *cb) {
    d->script = label;
    d->script_pc = 0;
    d->script_mode = self->player_mode;
    if (cb && cb->script_selected && label) cb->script_selected(self, label, cb->user);
}

static void abort_script(wm_arcade_drone_state_t *d) {
    d->script = NULL;
    d->script_pc = 0;
}

static int attack_locked_mode(int mode) {
    return mode == WM_PMODE_PUPPET2 || mode == WM_PMODE_PUPPET ||
           mode == WM_PMODE_HEADHELD || mode == WM_PMODE_HEADHOLD ||
           mode == WM_PMODE_ATTACHED;
}

static uint16_t flip_lr_dir(uint16_t dir) {
    const uint16_t vertical = (uint16_t)(dir & (WM_MOVE_UP | WM_MOVE_DOWN));
    const uint16_t horiz = (uint16_t)(dir & (WM_MOVE_LEFT | WM_MOVE_RIGHT));
    uint16_t out = vertical;
    if (horiz & WM_MOVE_LEFT) out = (uint16_t)(out | WM_MOVE_RIGHT);
    if (horiz & WM_MOVE_RIGHT) out = (uint16_t)(out | WM_MOVE_LEFT);
    return out;
}

static void apply_packed_input(wm_arcade_actor_t *self,
                               wm_arcade_drone_state_t *d,
                               uint16_t packed,
                               int32_t delay) {
    uint16_t dir;
    d->but = (uint16_t)(packed & WM_BTN_ATTACK_MASK);
    dir = (uint16_t)(packed >> 5);
    if ((self->facing_dir & WM_MOVE_RIGHT) == 0) dir = flip_lr_dir(dir);
    d->joy = dir;
    d->delay = delay;
}

wm_arcade_drone_step_result_t wm_arcade_drone_script_step(
    wm_arcade_actor_t *self,
    wm_arcade_actor_t *opp,
    wm_arcade_drone_state_t *d,
    const wm_arcade_drone_script_t *script,
    const wm_arcade_drone_callbacks_t *cb) {
    size_t guard = 0;
    if (!self || !opp || !d || !script || !script->ops) return WM_DRONE_STEP_IDLE;

    /* MODE_NORMAL and MODE_BLOCK are intentionally treated as the same script mode. */
    if (self->player_mode != WM_PMODE_NORMAL && self->player_mode != WM_PMODE_BLOCK &&
        self->player_mode != d->script_mode) {
        abort_script(d);
        return WM_DRONE_STEP_ABORT_SCRIPT;
    }

    while (d->script_pc < script->op_count && guard++ < script->op_count + 32u) {
        const wm_arcade_drone_script_op_t *op = &script->ops[d->script_pc];
        switch (op->opcode) {
            case WM_DRONE_SC_DONE:
                /* DRONE.ASM #dsdone stores the already-advanced a9 back to
                 * DRN_ACT_p and returns for this tick. It is a yield, not an
                 * abort. Next tick resumes at the following script word. */
                ++d->script_pc;
                return WM_DRONE_STEP_SCRIPT;

            case WM_DRONE_SC_SEEK:
                if (cb && cb->script_seek && cb->script_seek(self, d, cb->user) == 0) {
                    ++d->script_pc;
                    continue;
                }
                return WM_DRONE_STEP_SCRIPT;

            case WM_DRONE_SC_SKILL_ABORT: {
                int pct = cb && cb->script_skill_pct
                    ? cb->script_skill_pct(op->source_label, d->skill, cb->user) : 0;
                if (opp->player_mode == WM_PMODE_BLOCK) {
                    abort_script(d);
                    return WM_DRONE_STEP_ABORT_SCRIPT;
                }
                if ((int)rnd_range(cb, 99) < pct) {
                    ++d->script_pc;
                    continue;
                }
                d->delay = 10;
                abort_script(d);
                return WM_DRONE_STEP_ABORT_SCRIPT;
            }

            case WM_DRONE_SC_WAIT_INTERRUPTIBLE:
                if ((self->anim_mode & WM_ARCADE_MODE_UNINT) == 0) {
                    ++d->script_pc;
                    continue;
                }
                return WM_DRONE_STEP_SCRIPT;

            case WM_DRONE_SC_ABORT_IF_BLOCKING:
                if (opp->player_mode == WM_PMODE_BLOCK) {
                    abort_script(d);
                    return WM_DRONE_STEP_ABORT_SCRIPT;
                }
                ++d->script_pc;
                continue;

            case WM_DRONE_SC_CALL_CODE:
            case WM_DRONE_SC_CALL_FUNCTION:
                /* C3 exposes source code-call seams but C4 owns their direct
                 * service ports. Never silently skip a source call. */
                if (!cb || !cb->script_call) return WM_DRONE_STEP_SCRIPT;
                /* Do not advance a9/PC unless the exact translated target ran. */
                if (!cb->script_call(self, op->source_label, cb->user))
                    return WM_DRONE_STEP_SCRIPT;
                ++d->script_pc;
                continue;

            case WM_DRONE_SC_RANDOM_JUMP:
                if ((int)rnd_range(cb, 99) < op->percent) d->script_pc = op->target_pc;
                else ++d->script_pc;
                continue;

            case WM_DRONE_SC_JUMP:
                d->script_pc = op->target_pc;
                continue;

            case WM_DRONE_SC_INPUT:
                apply_packed_input(self, d, op->input_word, op->delay);
                ++d->script_pc;
                if (op->delay > 0) return WM_DRONE_STEP_INPUT;
                /* Source falls through to #dsabt for zero/nonpositive delays. */
                abort_script(d);
                return WM_DRONE_STEP_INPUT;

            default:
                abort_script(d);
                return WM_DRONE_STEP_ABORT_SCRIPT;
        }
    }

    abort_script(d);
    return WM_DRONE_STEP_ABORT_SCRIPT;
}

static wm_arcade_drone_step_result_t run_selected_script(
    wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
    wm_arcade_drone_state_t *d, const wm_arcade_drone_callbacks_t *cb) {
    const wm_arcade_drone_script_t *sc;
    if (!d->script) return WM_DRONE_STEP_IDLE;
    if (!cb || !cb->resolve_script) return WM_DRONE_STEP_SCRIPT;
    sc = cb->resolve_script(d->script, cb->user);
    if (!sc) return WM_DRONE_STEP_SCRIPT;
    return wm_arcade_drone_script_step(self, opp, d, sc, cb);
}

static const char *pick_range_script(const wm_arcade_actor_t *self,
                                     const wm_arcade_actor_t *opp,
                                     int band, int mymode, int opmode,
                                     int skill,
                                     const wm_arcade_drone_callbacks_t *cb) {
    const wm_arcade_drone_script_list_t *list;
    int32_t maxidx;
    uint32_t idx;
    if (!cb || !cb->range_script_list) return NULL;
    list = cb->range_script_list(self, opp, band, mymode, opmode, cb->user);
    if (!list || !list->scripts || list->script_count == 0) return NULL;
    maxidx = list->source_max_index;
    if (maxidx < 0) {
        maxidx = -maxidx;
        if (skill > 26) maxidx = 1; /* source cmpi 26*16 after X16 skill */
    }
    if (maxidx < 0) return NULL;
    if ((size_t)maxidx >= list->script_count) maxidx = (int32_t)list->script_count - 1;
    idx = rnd_range(cb, (uint32_t)maxidx);
    if (idx >= list->script_count) return NULL;
    return list->scripts[idx];
}

wm_arcade_drone_step_result_t wm_arcade_drone_main(
    wm_arcade_actor_t *self, wm_arcade_drone_state_t *d,
    const wm_arcade_drone_world_t *w, const wm_arcade_drone_callbacks_t *cb) {
    wm_arcade_actor_t *opp;
    uint16_t old_but, old_joy;
    int mymode, opmode, rank, alive_team, result = WM_DRONE_STEP_IDLE;
    int skill;
    int do_action = 0;
    if (!self || !d || !w) return WM_DRONE_STEP_IDLE;
    old_but = d->but;
    old_joy = d->joy;
    opp = cb && cb->closest_actor ? cb->closest_actor(self, cb->user) : self->smart_target;
    if (!opp) {
        wm_arcade_drone_commit_inputs(self, d, old_but, old_joy);
        return WM_DRONE_STEP_IDLE;
    }
    skill = d->skill;
    if (skill < 0) skill = 0;
    if (skill > 29) skill = 29;
    mymode = self->player_mode;
    opmode = opp->player_mode;

    rank = same_target_rank(self, opp, w, cb, &alive_team);
    if (rank != 0) {
        if (!d->script) {
            int oldmode = d->mode;
            d->mode = -3;
            if (oldmode != -3) {
                d->seek_dir = (self->x_int - opp->x_int >= 0) ? 4 : 12;
                {
                    int sd = (int)rnd_range(cb, 4);
                    sd = 2 + (sd & 1) + rank;
                    d->seek_dist = sd;
                }
            }
        }
    } else if (d->mode == -3) {
        uint32_t range = w->first_ladder ? 1u : (skill > 13 ? 4u : 3u);
        d->mode = (int32_t)rnd_range(cb, range) - 2;
        if (!d->script) d->joy = 0;
        d->seek_dir = (self->x_int - opp->x_int >= 0) ? 4 : 12;
        d->seek_dist = (int32_t)rnd_range(cb, 4);
    }

    /* Source checks PCNT low 5 bits and uses rnd(3) as a 1-in-4 gate. */
    if ((w->pcnt & 31u) == 0u && rnd_plain(cb, 3) == 0u && d->mode > -3) {
        uint32_t range = w->first_ladder ? 1u : (skill > 13 ? 4u : 3u);
        d->mode = (int32_t)rnd_range(cb, range) - 2;
        if (!d->script) d->joy = 0;
        d->seek_dir = (self->x_int - opp->x_int >= 0) ? 4 : 12;
        {
            int sd = (int)rnd_range(cb, 4);
            if (d->mode == -3) sd = 2 + (sd & 1) + rank;
            d->seek_dist = sd;
        }
    }

    if (d->mode < 0 && mymode == WM_PMODE_NORMAL && !d->script && cb && cb->seek_dir_dist)
        cb->seek_dir_dist(self, d, cb->user);

    /* Source decrements whenever charged, including 0 -> -1. */
    if (d->but_charge) --d->but_charge_delay;

    /* Any positive GETUP_TIME aborts the current script. The roll only controls
     * whether five ticks are removed before that abort. */
    if (self->getup_time > 0) {
        if ((int)rnd_range(cb, 99) < wm_arcade_drone_getup_pct(skill)) {
            self->getup_time -= 5;
            if (self->getup_time < 0) self->getup_time = 0;
        }
        abort_script(d);
        result = WM_DRONE_STEP_ABORT_SCRIPT;
        goto done;
    }

    /* Source subtracts first and only stores the decremented value when >0. */
    if (d->delay > 1) {
        --d->delay;
        goto done;
    }

    /* Block detection / attack-specific learning. */
    {
        int32_t till = (int16_t)(opp->attack_time - w->round_tickcount);
        int within = 0;
        if (till >= 0 && opp->attack_type != WM_AT_PUSH &&
            opp->getup_time <= 0 && opmode != WM_PMODE_INAIR2 && !(old_but & DRN_BLOCK)) {
            if (opp->attack_type == WM_AT_MSL) {
                within = self->closest_zdist <= 50; /* source bypasses X check */
            } else {
                within = self->closest_xdist <= 180 && self->closest_zdist <= 100;
            }
            if (within) {
                int at = opp->attack_type;
                int misses = (at >= 0 && at < WM_AT_NUM) ? d->missed_blocks[at] : 0;
                int pct = 0;
                if (misses > 9) misses = 9;
                if (cb && cb->block_base_pct) pct += cb->block_base_pct(skill, cb->user);
                if (cb && cb->block_attack_pct) pct += cb->block_attack_pct(misses, cb->user);
                /* Literal source: (alive_team-1), then X32 (shift left 5), subtract. */
                if (alive_team > 1) pct -= (alive_team - 1) << 5;
                if ((int)rnd_range(cb, 99) < pct) {
                    if (till < 15) till = 15;
                    if (opp->attack_type == WM_AT_LEAPING &&
                        rnd_range(cb, (uint32_t)(skill >> 2)) != 0u) {
                        opp->attack_time = w->round_tickcount;
                        select_script(self, d, "slhtoss", cb);
                        result = run_selected_script(self, opp, d, cb);
                        if (result == WM_DRONE_STEP_IDLE) result = WM_DRONE_STEP_SCRIPT;
                        goto done;
                    }
                    d->delay = till;
                    d->but = DRN_BLOCK;
                    if (opp->attack_type == WM_AT_PUPPET) {
                        d->joy = WM_MOVE_DOWN_LEFT; /* B_M+L_M+D_M through #setbx */
                    }
                    abort_script(d);
                    result = WM_DRONE_STEP_BLOCK;
                    goto done;
                }
                opp->attack_time = w->round_tickcount;
                if (at >= 0 && at < WM_AT_NUM && d->missed_blocks[at] != UINT16_MAX)
                    ++d->missed_blocks[at];
            }
        }
    }

    /* Existing source script continues at #cact; do not select a new action. */
    if (d->script) {
        result = run_selected_script(self, opp, d, cb);
        goto done;
    }

    if (mymode == WM_PMODE_BLOCK && !(old_but & DRN_PUNCH) &&
        rnd_range(cb, 7) == 0u && self->closest_xdist <= 110 && self->closest_zdist <= 40) {
        d->but = (uint16_t)(DRN_PUNCH | DRN_BLOCK);
        result = WM_DRONE_STEP_INPUT;
        goto done;
    }
    d->but = 0;

    /* MODE_NORMAL has its own passive/aggressive scheduling before #doact. */
    if (mymode == WM_PMODE_NORMAL) {
        int dmode;
        if (self->in_ring != 0 && opp->in_ring == 0) {
            select_script(self, d, "drn_enterring", cb);
            result = run_selected_script(self, opp, d, cb);
            if (result == WM_DRONE_STEP_IDLE) result = WM_DRONE_STEP_SCRIPT;
            goto done;
        }
        dmode = d->mode;
        if (dmode > -3) {
            if (opmode == WM_PMODE_ONGROUND) {
                do_action = 1;
            } else if (opmode == WM_PMODE_INAIR2) {
                select_script(self, d, "drn_opinair", cb);
                result = run_selected_script(self, opp, d, cb);
                if (result == WM_DRONE_STEP_IDLE) result = WM_DRONE_STEP_SCRIPT;
                goto done;
            } else if (opmode == WM_PMODE_RUNNING && self->closest_xdist > 80) {
                select_script(self, d, "drn_oprun", cb);
                result = run_selected_script(self, opp, d, cb);
                if (result == WM_DRONE_STEP_IDLE) result = WM_DRONE_STEP_SCRIPT;
                goto done;
            } else {
                int32_t after_attack = (int16_t)(opp->attack_time - w->round_tickcount) + 15;
                if (after_attack >= 0) {
                    do_action = 1;
                } else if (dmode >= 0) {
                    uint32_t attack_roll_max = skill > 22 ? 7u : 31u;
                    if (rnd_plain(cb, attack_roll_max) == 0u) do_action = 1;
                }
            }
        }

        if (!do_action) {
            int big;
            /* Source: (PCNT+1) must have low four bits clear. */
            if (((w->pcnt + 1u) & 15u) != 0u) goto done;
            big = self->closest_xdist > self->closest_zdist
                ? self->closest_xdist : self->closest_zdist;
            if (big < 70) {
                do_action = 1;
            } else {
                if (d->mode <= -3) goto done;
                if (!w->first_ladder && rnd_plain(cb, 15) == 0u) do_action = 1;
                if (!do_action && opmode == WM_PMODE_NORMAL) goto done;
            }
        }

        if (do_action && attack_locked_mode(opmode)) goto done;
    }

    /* #doact: non-normal self modes jump here directly and do not use the
     * opponent puppet/headhold exclusion above. */
    if (mymode == WM_PMODE_ONGROUND) {
        select_script(self, d, "drn_roll", cb);
    } else if (mymode == WM_PMODE_INAIR2) {
        select_script(self, d, "drn_inair", cb);
    } else if (mymode == WM_PMODE_ONTURNBKL) {
        select_script(self, d, "drn_ontb", cb);
    } else if (mymode == WM_PMODE_DEAD) {
        d->but = (uint16_t)rnd_plain(cb, 7);
        result = WM_DRONE_STEP_INPUT;
        goto done;
    } else if ((self->anim_mode & WM_ARCADE_MODE_UNINT) != 0) {
        goto done;
    } else if (mymode == WM_PMODE_RUNNING) {
        select_script(self, d, "drn_run", cb);
    } else if (mymode == WM_PMODE_HEADHOLD) {
        d->but_charge = 0;
        if (!cb || !cb->check_combo_go || cb->check_combo_go(self, cb->user) >= 0)
            select_script(self, d, "drn_combo", cb);
    }

    if (d->script) goto selected;

    if (d->but_charge && d->but_charge_delay <= 0 &&
        opmode != WM_PMODE_BLOCK && d->charge_script) {
        select_script(self, d, d->charge_script, cb);
        goto selected;
    }

    if (self->closest_dist <= 200 && opmode == WM_PMODE_BLOCK) {
        if (self->closest_zdist <= 40 && self->closest_xdist <= 60) {
            select_script(self, d,
                opp->stick_val_cur == WM_MOVE_DOWN_LEFT ? "M_shrtblkrdl" : "M_shrtblkr", cb);
        } else {
            select_script(self, d, "drn_seekclose", cb);
        }
        goto selected;
    }
    if (opmode == WM_PMODE_DEAD) {
        select_script(self, d, "drn_oppdead", cb);
        goto selected;
    }

    {
        int big = self->closest_xdist;
        int z2 = self->closest_zdist * 2;
        int band;
        const char *label;
        if (z2 > big) big = z2;
        band = big < 100 ? 0 : (big < 180 ? 1 : 2);
        label = pick_range_script(self, opp, band, mymode, opmode, skill, cb);
        if (label) select_script(self, d, label, cb);
    }

selected:
    if (d->script && mymode == WM_PMODE_HEADHOLD) {
        int base = alive_team >= 2 ? 22 : 1;
        int mx = cb && cb->headhold_delay_max ? cb->headhold_delay_max(skill, cb->user) : 0;
        int dd = base + (int)rnd_range(cb, mx > 0 ? (uint32_t)mx : 0u);
        if ((w->pcnt & 1u) && dd > 65) dd = 65;
        d->delay = dd;
        d->but = 0;
        d->joy = 0; /* #setbx with a0=0 */
        result = WM_DRONE_STEP_SCRIPT;
        goto done;
    }
    if (d->script && mymode == WM_PMODE_HEADHELD) {
        int team_delay = alive_team;
        int mx = cb && cb->headheld_delay_max ? cb->headheld_delay_max(skill, cb->user) : 0;
        d->but_charge = 0;
        if (rnd_plain(cb, 7) != 0u) {
            /* Literal source: subk 1; X16 => (alive_team-1)<<4. */
            team_delay = (alive_team - 1) << 4;
        }
        d->delay = team_delay + 1 + (int)rnd_range(cb, mx > 0 ? (uint32_t)mx : 0u);
        if ((w->pcnt & 1u) && d->delay > 70) d->delay = 70;
        d->but = 0;
        d->joy = 0;
        result = WM_DRONE_STEP_SCRIPT;
        goto done;
    }

    if (d->script) {
        result = run_selected_script(self, opp, d, cb);
        if (result == WM_DRONE_STEP_IDLE) result = WM_DRONE_STEP_SCRIPT;
    }

done:
    wm_arcade_drone_commit_inputs(self, d, old_but, old_joy);
    return (wm_arcade_drone_step_result_t)result;
}
