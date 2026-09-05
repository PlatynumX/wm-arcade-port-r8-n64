#include "wm/arcade/wm_arcade_bret_drone.h"
#include "wm/arcade/wm_arcade_roster.h"
#include "wm/arcade/wmania_ring_geometry.h"
#include <string.h>

/*
 * DRONE.ASM's own U_M/D_M/L_M/R_M equ to MOVE_x<<5 (RING.EQU/DRONE.ASM:98-
 * 101) -- exactly wm_arcade_drone_script_op_t's packed input_word layout
 * (button bits 0-4, direction bits 5+, see apply_packed_input in
 * wm_arcade_drone.c) -- and P_M/SP_M/K_M/SK_M/B_M to the plain button bits.
 */
#define UM ((uint16_t)(WM_MOVE_UP << 5))
#define DM ((uint16_t)(WM_MOVE_DOWN << 5))
#define LM ((uint16_t)(WM_MOVE_LEFT << 5))
#define RM ((uint16_t)(WM_MOVE_RIGHT << 5))
#define PM ((uint16_t)WM_BTN_PUNCH)
#define SPM ((uint16_t)WM_BTN_SPUNCH)
#define KM ((uint16_t)WM_BTN_KICK)
#define SKM ((uint16_t)WM_BTN_SKICK)
#define BM ((uint16_t)WM_BTN_BLOCK)

#define IN(w, dl) {WM_DRONE_SC_INPUT, (uint16_t)(w), (dl), 0, 0, NULL, NULL}
#define SEEKOP(lbl) {WM_DRONE_SC_SEEK, 0, 0, 0, 0, (lbl), NULL}
#define SKILLOP(lbl) {WM_DRONE_SC_SKILL_ABORT, 0, 0, 0, 0, (lbl), NULL}
#define CALLOP(lbl) {WM_DRONE_SC_CALL_CODE, 0, 0, 0, 0, (lbl), NULL}
#define FUNCOP(lbl) {WM_DRONE_SC_CALL_FUNCTION, 0, 0, 0, 0, (lbl), NULL}
#define RJMP(pct, idx) {WM_DRONE_SC_RANDOM_JUMP, 0, 0, (pct), (idx), NULL, NULL}
#define JMPI(idx) {WM_DRONE_SC_JUMP, 0, 0, 0, (idx), NULL, NULL}
#define JMPX(scr) {WM_DRONE_SC_JUMP, 0, 0, 0, 0, NULL, (scr)}

static int32_t iabs32(int32_t v) { return v < 0 ? -v : v; }

static uint32_t rr(WmRng *rng, uint32_t max_inclusive) {
    return wm_rng_rndrng0(rng, max_inclusive);
}

/*
 * DRONE.ASM's own +-2/3 reroll (drone_seekdirdist's "Get rnd +-2/3"),
 * transcribed as the literal source arithmetic rather than a guessed
 * closed form -- see wm_arcade_bret_drone.c's derivation notes.
 */
static int32_t rnd_pm23(WmRng *rng) {
    int32_t a0 = (int32_t)rr(rng, 3);
    a0 -= 1;
    if (a0 == 0) a0 -= 2;
    a0 += 1;
    if (a0 <= 0) a0 -= 2;
    return a0;
}

/*
 * drone_seekxz: computes the joy bits to push the mover toward (tx,tz),
 * stopping within `range` on each axis independently. DRONE.ASM's own
 * subk4/addk8 (X) and subk1/addk2 (Z) bit trick nets out to exactly the
 * WM_MOVE_LEFT(4)/RIGHT(8) and WM_MOVE_UP(1)/DOWN(2) bit values already
 * used throughout this port -- see the derivation in this file's history.
 * Returns 0 once both axes are within range (the source's own "arrived").
 */
static uint16_t seekxz_joy(int32_t self_x, int32_t self_z,
                           int32_t tx, int32_t tz, int32_t range) {
    uint16_t joy = 0;
    int32_t dx = self_x - tx;
    int32_t dz = self_z - tz;
    if (iabs32(dx) > range) joy = (uint16_t)(joy | ((dx < 0) ? WM_MOVE_RIGHT : WM_MOVE_LEFT));
    if (iabs32(dz) > range) joy = (uint16_t)(joy | ((dz < 0) ? WM_MOVE_DOWN : WM_MOVE_UP));
    return joy;
}

/* Facing-relative flip for a direction nibble, matching apply_packed_input
   and the real interpreter's own #setbx facing check -- needed here only
   for the handful of DS_CODE-block outputs that reach the real interpreter
   as *bytecode* words after their own DS_CODEEND (drn_run's #brkrun/#nchrg,
   drn_oprun/drn_opinair's terminal presses); seekxz/seekdirdist-sourced
   writes are already absolute and must NOT be flipped again. */
static uint16_t flip_lr(uint16_t dir) {
    uint16_t v = (uint16_t)(dir & (WM_MOVE_UP | WM_MOVE_DOWN));
    if (dir & WM_MOVE_LEFT) v = (uint16_t)(v | WM_MOVE_RIGHT);
    if (dir & WM_MOVE_RIGHT) v = (uint16_t)(v | WM_MOVE_LEFT);
    return v;
}

static void apply_bytecode_input(wm_arcade_actor_t *self, wm_arcade_drone_state_t *d,
                                 uint16_t buttons, uint16_t dir, int32_t delay) {
    if ((self->facing_dir & WM_MOVE_RIGHT) == 0) dir = flip_lr(dir);
    d->but = buttons;
    d->joy = dir;
    d->delay = delay;
}

/* ------------------------------------------------------------------ */
/* #sine_t (DRONE.ASM, drone_seekdirdist's #drn_getxz): 6 radius bands,
   50/100/150/200/250/300, each a 16-entry (dir 0-15) table; the source's
   own 20-word-per-band layout pads the last 4 entries with a duplicate of
   the first 4 (period-16 wraparound) purely so a caller can read dir+4
   without a separate bounds check -- (dir+4)&15 here does the same thing
   with no padding needed. The X read is a 4-position (quarter-cycle)
   phase-shifted read of the SAME table as the Z read (cos(x)=sin(x+90)). */
static const int16_t s_sine_band[6][16] = {
    {-50,-46,-35,-19,0,19,35,46,50,46,35,19,0,-19,-35,-46},
    {-100,-92,-71,-38,0,38,71,92,100,92,71,38,0,-38,-71,-92},
    {-150,-139,-106,-57,0,57,106,139,150,139,106,57,0,-57,-106,-139},
    {-200,-185,-141,-76,0,76,141,185,200,185,141,76,0,-76,-141,-185},
    {-250,-231,-177,-95,0,95,177,231,250,231,177,95,0,-95,-177,-231},
    {-300,-277,-212,-115,0,114,212,277,300,277,212,114,0,-114,-212,-277}
};

/* #drn_getxz: returns the sine-table Z-offset used (0 is treated as
   "failed" by the caller too, matching the source's own `jrnz` test on
   this same value), or 0 if the resulting XZ falls outside the ring
   visinity box. On success, out_tx/out_tz hold the real target position
   (opp position plus offset) -- the source leaves these live in a0/a1 for
   drone_seekxz's own immediately-following call. */
static int32_t drn_getxz(int band, int dir, const wm_arcade_actor_t *opp,
                         int32_t *out_tx, int32_t *out_tz) {
    int d = dir & 15;
    int32_t z_off = s_sine_band[band][d];
    int32_t x_off = s_sine_band[band][(d + 4) & 15];
    int32_t tx = opp->x_int + x_off;
    int32_t tz;
    if (tx < WM_RING_X_CENTER - 220 || tx > WM_RING_X_CENTER + 220) return 0;
    tz = opp->z_int + z_off;
    if (tz < WM_RING_TOP || tz > WM_RING_BOT) return 0;
    *out_tx = tx;
    *out_tz = tz;
    return z_off;
}

/* drone_seekdirdist: pushes self toward the seek_dir/seek_dist circling
   position around opp, retrying nearby directions if the exact one is out
   of the ring, and (only when DRN_MODE was -1, i.e. mode+1>=0 is false)
   rerolling seek_dir by +-2/3 once arrived "so we don't walk into ropes". */
static void run_seekdirdist(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                            wm_arcade_drone_state_t *d, WmRng *rng) {
    int band = d->seek_dist;
    int dir = d->seek_dir;
    int32_t tx = 0, tz = 0;
    uint16_t old_joy = d->joy;

    if (band < 0) band = 0;
    if (band > 5) band = 5;

    if (drn_getxz(band, dir, opp, &tx, &tz) == 0) {
        int b0, a5 = dir, found = 0;
        for (b0 = 7; b0 > 0; --b0) {
            dir = (dir + 1) & 15;
            if (drn_getxz(band, dir, opp, &tx, &tz) != 0) { found = 1; break; }
            a5 = (a5 - 1) & 15;
            if (drn_getxz(band, a5, opp, &tx, &tz) != 0) { dir = a5; found = 1; break; }
        }
        if (!found) { d->joy = 0; return; }
        d->seek_dir = dir;
    }

    {
        uint16_t joy = seekxz_joy(self->x_int, self->z_int, tx, tz, 30);
        d->joy = joy;
        if (joy != 0) return; /* still moving: keep this seek_dir */
    }

    if (d->mode + 1 >= 0) return; /* DRN_MODE wasn't -1: skip the reroll */
    d->joy = old_joy;            /* restore "to lessen glitch" */
    dir = (dir + rnd_pm23(rng)) & 15;
    d->seek_dir = dir;
}

/* ------------------------------------------------------------------ */
/* Primitive action scripts. Bret's own (brM_n/brMm_n/brM_hh/brM_hhr) plus
   the shared generic ones (M_og/Mm_og/M_opptbkl/M_shrtblkr[dl]) reference
   these by DRONE.ASM's own label text; every value here is transcribed
   directly from DRONE.ASM's `.word`/DS_ tables (see this file's own
   derivation notes / commit history for the exact source line ranges). */

static const wm_arcade_drone_script_op_t ops_p[] = {IN(PM, 0)};
static const wm_arcade_drone_script_op_t ops_sp[] = {IN(SPM, 0)};
static const wm_arcade_drone_script_op_t ops_k[] = {IN(KM, 0)};
static const wm_arcade_drone_script_op_t ops_sk[] = {IN(SKM, 0)};
static const wm_arcade_drone_script_op_t ops_spx[] = {
    IN(SPM,2), IN(0,2), IN(SPM,2), IN(0,2), IN(SPM,2),
    IN(SPM,2), IN(0,2), IN(SPM,2), IN(0,2), IN(SPM,0)
};
static const wm_arcade_drone_script_op_t ops_hgrab[] = {
    IN(RM,2), IN(0,2), IN(RM,2), IN(SPM,0)
};
static const wm_arcade_drone_script_op_t ops_flng[] = {
    IN(LM,2), IN(0,2), IN((uint16_t)(LM|SPM),0)
};
static const wm_arcade_drone_script_op_t ops_htoss[] = {
    IN(LM,2), IN(0,2), IN((uint16_t)(LM|PM),0)
};
static const wm_arcade_drone_script_op_t ops_slhtoss[] = {
    IN(LM,2), IN(0,2), IN(LM,2), IN(PM,0)
};
static const wm_arcade_drone_script_op_t ops_ucut[] = {IN((uint16_t)(DM|SPM),0)};
static const wm_arcade_drone_script_op_t ops_ddp[] = {
    IN(DM,2), IN(0,2), IN(DM,2), IN(PM,0)
};
static const wm_arcade_drone_script_op_t ops_jp[] = {
    IN(DM,2), IN((uint16_t)(DM|RM),2), IN(RM,2), IN(PM,0)
};
static const wm_arcade_drone_script_op_t ops_j2sp[] = {IN((uint16_t)(LM|DM),2)};
static const wm_arcade_drone_script_op_t ops_llsk[] = {
    IN(LM,2), IN(0,2), IN(LM,2), IN(SKM,0)
};
static const wm_arcade_drone_script_op_t ops_fast[] = {
    IN(LM,2), IN((uint16_t)(LM|DM),2), IN(DM,2), IN((uint16_t)(DM|RM),2),
    IN(RM,2), IN((uint16_t)(RM|UM),2), IN(UM,2), IN((uint16_t)(UM|LM),0)
};
static const wm_arcade_drone_script_op_t ops_chrg[] = {
    CALLOP("drone_chrg"), IN(0,0)
};
static const wm_arcade_drone_script_op_t ops_uddsk[] = {
    IN(UM,2), IN(DM,2), IN(0,2), IN(DM,2), IN(SKM,0)
};
static const wm_arcade_drone_script_op_t ops_lrrsp[] = {
    IN(LM,2), IN(RM,2), IN(0,2), IN(RM,2), IN(SPM,0)
};
static const wm_arcade_drone_script_op_t ops_lrrp[] = {
    IN(LM,2), IN(RM,2), IN(0,2), IN(RM,2), IN(PM,0)
};
static const wm_arcade_drone_script_op_t ops_jpx[] = {
    IN(DM,2), IN((uint16_t)(DM|RM),2), IN(RM,2), IN(PM,0),
    SKILLOP("sklrep_t"),
    IN(PM,4), IN(0,4), IN(PM,4), IN(0,4), IN(PM,0)
};
static const wm_arcade_drone_script_op_t ops_rp[] = {IN((uint16_t)(RM|PM),0)};
static const wm_arcade_drone_script_op_t ops_rsp[] = {IN((uint16_t)(RM|SPM),0)};
static const wm_arcade_drone_script_op_t ops_oghg[] = {IN(DM,2), IN(SPM,0)};
static const wm_arcade_drone_script_op_t ops_seeksp[] = {
    SEEKOP("plain70"), IN(SPM,0)
};
static const wm_arcade_drone_script_op_t ops_seeksk[] = {
    SEEKOP("plain70"), IN(SKM,0)
};
static const wm_arcade_drone_script_op_t ops_run[] = {
    CALLOP("drone_chkrun"), IN((uint16_t)(PM|KM),0), IN(0,0)
};

/* One-op scripts whose entire behavior is a real, wrestler-agnostic DS_CODE
   block (or a persistent SEEK loop) -- see the per-routine callbacks below
   for the transcribed logic. */
static const wm_arcade_drone_script_op_t ops_call_drn_run[] = {FUNCOP("drn_run")};
static const wm_arcade_drone_script_op_t ops_call_drn_oprun[] = {FUNCOP("drn_oprun")};
static const wm_arcade_drone_script_op_t ops_roll[] = {
    FUNCOP("drn_roll_init"), SEEKOP("drn_roll")
};
static const wm_arcade_drone_script_op_t ops_climbtb[] = {FUNCOP("drn_climbtb")};
static const wm_arcade_drone_script_op_t ops_ontb[] = {SEEKOP("drn_ontb")};
static const wm_arcade_drone_script_op_t ops_inair[] = {SEEKOP("drn_inair")};
static const wm_arcade_drone_script_op_t ops_opinair[] = {
    FUNCOP("drn_opinair_init"), SEEKOP("drn_opinair")
};
static const wm_arcade_drone_script_op_t ops_oppdead[] = {SEEKOP("drn_oppdead")};
static const wm_arcade_drone_script_op_t ops_seek[] = {SEEKOP("drn_seek")};
static const wm_arcade_drone_script_op_t ops_retreat[] = {
    FUNCOP("drn_retreat_init"), SEEKOP("drn_retreat")
};
static const wm_arcade_drone_script_op_t ops_seekclose[] = {SEEKOP("drn_seekclose")};
static const wm_arcade_drone_script_op_t ops_charge_run[] = {
    CALLOP("drone_chkrun"), IN((uint16_t)(PM|KM),0), FUNCOP("charge_run_fire"), IN(0,0)
};

/* drn_combo's shared #cstrt/#csp/#csk/#cp/#ck repeat-punch chain (used by
   every wrestler's own drn_combo branch via `DS_JMP #cstrt`); combined
   into one array since every jump between these pieces is same-script.
   Indices: 0=#cstrt, 4=#csp, 14=#csk, 24=#cp, 34=#ck. Reachable only if
   CHECK_COMBO_GO ever reports "lit", which this port's own already-
   established finding (wm_arcade_mode_dead.h) proves it never does -- see
   check_combo_go_cb below -- kept here for completeness/fidelity only. */
static const wm_arcade_drone_script_op_t ops_combo_cstrt[] = {
    /* 0 #cstrt */ IN(0,2), RJMP(25,14), RJMP(25,24), RJMP(25,34),
    /* 4 #csp   */ IN(SPM,6), IN(0,6), IN(SPM,6), IN(0,6),
                   IN(SPM,6), IN(0,6), IN(SPM,6), IN(0,6),
                   SKILLOP("sklrep_t"), JMPI(0),
    /* 14 #csk  */ IN(SKM,6), IN(0,6), IN(SKM,6), IN(0,6),
                   IN(SKM,6), IN(0,6), IN(SKM,6), IN(0,6),
                   SKILLOP("sklrep_t"), JMPI(0),
    /* 24 #cp   */ IN(PM,6), IN(0,6), IN(PM,6), IN(0,6),
                   IN(PM,6), IN(0,6), IN(PM,6), IN(0,6),
                   SKILLOP("sklrep_t"), JMPI(0),
    /* 34 #ck   */ IN(KM,6), IN(0,6), IN(KM,6), IN(0,6),
                   IN(KM,6), IN(0,6), IN(KM,6), IN(0,6),
                   SKILLOP("sklrep_t"), JMPI(0)
};
static const wm_arcade_drone_script_op_t ops_combo_brt[] = {
    IN(RM,2), IN(0,2), IN(RM,2), IN(0,2), RJMP(50,7), IN(PM,2), JMPX("combo_cstrt"),
    /* 7 #brt2 */ IN(SKM,2), JMPX("combo_cstrt")
};

/* ------------------------------------------------------------------ */
/* Range lists (wm_arcade_drone_script_list_t) -- header word is the
   source's own max random index (negative marks a headhold list). */

static const char *const s_brM_n[] = {
    "#run", "#p", "#sp", "#k", "#sk", "#spx",
    "#hgrab", "#hgrab", "#hgrab", "#flng", "#htoss", "#htoss",
    "#ucut", "#ddp", "#jp", "#j2sp", "#llsk"
};
static const wm_arcade_drone_script_list_t s_brM_n_list = {16, s_brM_n, 17};

static const char *const s_brMm_n[] = {
    "drn_seek", "drn_retreat", "#run", "#sp", "#sk", "#flng",
    "#ddp", "#jp", "#j2sp", "#llsk", "#fast", "#chrg"
};
static const wm_arcade_drone_script_list_t s_brMm_n_list = {11, s_brMm_n, 12};

static const char *const s_brM_hh[] = {
    "#uddsk", "#lrrsp", "#ucut", "#jpx", "#rp", "#rsp", "#lrrp"
};
static const wm_arcade_drone_script_list_t s_brM_hh_list = {-6, s_brM_hh, 7};

static const char *const s_brM_hhr[] = {"#k", "#uddsk", "#lrrsp", "#lrrp"};
static const wm_arcade_drone_script_list_t s_brM_hhr_list = {3, s_brM_hhr, 4};

static const char *const s_M_og[] = {"#p", "#sp", "#k", "#sk", "#oghg", "#oghg"};
static const wm_arcade_drone_script_list_t s_M_og_list = {5, s_M_og, 6};

static const char *const s_Mm_og[] = {"drn_seek", "#seeksp", "#seeksk"};
static const wm_arcade_drone_script_list_t s_Mm_og_list = {2, s_Mm_og, 3};

static const char *const s_M_opptbkl[] = {"#run"};
static const wm_arcade_drone_script_list_t s_M_opptbkl_list = {0, s_M_opptbkl, 1};

static const char *const s_mdn[] = {"drn_seek", "#run", "drn_climbtb", "drn_taunt"};
static const wm_arcade_drone_script_list_t s_mdn_list = {3, s_mdn, 4};

static const char *const s_mdog[] = {"drn_seek", "drn_seek", "#run"};
static const wm_arcade_drone_script_list_t s_mdog_list = {2, s_mdog, 3};

/* M_shrtblkr/M_shrtblkrdl are selected *directly* by wm_arcade_drone_main
   (their own name becomes d->script) rather than via range_script_list, so
   resolve_script_cb below performs their own random pick internally. */
static const char *const s_M_shrtblkr[] = {"#hgrab", "#htoss"};
static const char *const s_M_shrtblkrdl[] = {"#hgrab", "#spx"};

/* ------------------------------------------------------------------ */
/* DS_CODE-block callbacks (script_call / script_seek). "self"/"opp" match
   DRONE.ASM's own a13/a8; b6/b7 (own/opp mode) are self->player_mode /
   opp->player_mode directly. */

static int call_drone_chkrun(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                             wm_arcade_drone_state_t *d, const char *label, void *user) {
    (void)d; (void)label; (void)user;
    /* #out (raw INRING!=0, i.e. genuinely out of the ring) is unreachable
       in this port -- in_ring is always true/"in ring", same reasoning as
       confine_wrestler's own #outring boundary. */
    if (opp->player_mode != WM_PMODE_ONGROUND) {
        if (self->closest_zdist < 70 && self->closest_zdist > 30 &&
            self->closest_xdist < 150)
            return WM_DRONE_CALL_SKIP_NEXT;
    }
    return WM_DRONE_CALL_CONTINUE;
}

static int call_drone_chrg(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                           wm_arcade_drone_state_t *d, const char *label, void *user) {
    WmRng *rng = (WmRng *)user;
    (void)opp; (void)label;
    if (d->but_charge != 0) return WM_DRONE_CALL_CONTINUE; /* already charging */
    /* Source: movk 20h,a0; callr rnd -- an AND-mask roll (bit5 only) that
       picks between two #wres_t entries per wrestler; consumed here only
       for RNG-stream parity -- Bret's own entry (`.long #brt,#brt`) is the
       same script either way, and no other wrestler's charge data is
       transcribed in this port (only Bret has a real move backend). */
    (void)rr(rng, 32);
    if (self->wrestler_num == WM_ROSTER_BRET) {
        d->but_charge = SKM;
        d->but_charge_delay = 53 * 2; /* TSEC*2 */
        d->charge_script = "charge_run";
    }
    return WM_DRONE_CALL_CONTINUE;
}

static int call_charge_run_fire(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                                wm_arcade_drone_state_t *d, const char *label, void *user) {
    (void)label; (void)user;
    if (iabs32(self->x_int - opp->x_int) < 150 && iabs32(self->z_int - opp->z_int) < 40)
        d->but_charge = 0; /* fire the charged move */
    return WM_DRONE_CALL_CONTINUE;
}

static int call_drn_run(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                        wm_arcade_drone_state_t *d, const char *label, void *user) {
    WmRng *rng = (WmRng *)user;
    int32_t a4, a1, a0, a3, a2;
    (void)label;

    if (self->player_mode != WM_PMODE_RUNNING && self->player_mode != WM_PMODE_BOUNCING) {
        return WM_DRONE_CALL_ABORT; /* #abrt: mode changed under us */
    }
    if (rr(rng, 0x1ff) == 0) {
        apply_bytecode_input(self, d, 0, WM_MOVE_LEFT, 0); /* #brkrun */
        return WM_DRONE_CALL_ABORT;
    }

    a4 = (int32_t)(self->x_vel >> 16) << 3;
    a1 = (int32_t)(opp->x_vel >> 16) << 5;
    a0 = (opp->x_int + a1) - (self->x_int + a4);
    a3 = self->x_int + a4;
    a1 = a4 ^ a0;
    a0 = iabs32(a0);
    a2 = self->closest_zdist;

    /* #inr: raw INRING==0 branch, always taken here (see call_drone_chkrun). */
    {
        int rope_ok;
        if (a4 < 0) rope_ok = a3 > WM_RING_X_CENTER - 210; /* #lrp */
        else rope_ok = a3 < WM_RING_X_CENTER + 210;
        if (!rope_ok) {
            if (opp->getup_time <= 0 && opp->player_mode != WM_PMODE_ONGROUND && a0 <= 300) {
                if (opp->player_mode == WM_PMODE_RUNNING) {
                    if (a2 < 90) {
                        apply_bytecode_input(self, d, 0, WM_MOVE_LEFT, 0); /* #brkrun */
                        return WM_DRONE_CALL_ABORT;
                    }
                } else if (a0 <= 180) {
                    rope_ok = 0; /* fall through to #rpok, not "ok" */
                } else {
                    rope_ok = 1;
                }
            } else {
                rope_ok = 1;
            }
        }
        if (!rope_ok) {
            /* #rpok */
            if (a1 < 0) goto l_rsk; /* Running away? */
            /* #cont */
            if (opp->player_mode == WM_PMODE_INAIR2) {
                apply_bytecode_input(self, d, 0, WM_MOVE_LEFT, 0); /* #brkrun */
                return WM_DRONE_CALL_ABORT;
            }
            a2 -= 30;
            if (a2 > 0) goto l_rsk;
            if (a0 > 250) goto l_rsk;
            {
                int32_t roll = 130 + (int32_t)rr(rng, 120);
                if (a0 > roll) goto l_rsk;
            }
            if (opp->player_mode == WM_PMODE_PUPPET2 || opp->player_mode == WM_PMODE_PUPPET ||
                opp->player_mode == WM_PMODE_HEADHELD || opp->player_mode == WM_PMODE_HEADHOLD ||
                opp->player_mode == WM_PMODE_ATTACHED) {
                apply_bytecode_input(self, d, 0, WM_MOVE_LEFT, 0); /* #brkrun */
                return WM_DRONE_CALL_ABORT;
            }
            if (d->but_charge != 0 && d->but_charge_delay <= 0) {
                d->but_charge = 0; /* fire it */
                return WM_DRONE_CALL_ABORT; /* #abrt (bare) */
            }
            /* #nchrg: two sequential 33%-style rolls (K_M/SK_M/SP_M). */
            {
                uint16_t but;
                if (rr(rng, 99) < 33) but = KM;
                else if (rr(rng, 99) < 33) but = SKM;
                else but = SPM;
                apply_bytecode_input(self, d, but, 0, 0);
                return WM_DRONE_CALL_ABORT;
            }
        }
    }
l_rsk:
    {
        uint16_t joy = seekxz_joy(self->x_int, self->z_int, opp->x_int, opp->z_int, 10);
        d->joy = (uint16_t)(joy & (WM_MOVE_UP | WM_MOVE_DOWN)); /* direct write, no flip */
        d->but = 0;
        d->delay = 0;
    }
    return WM_DRONE_CALL_ABORT;
}

static int call_drn_oprun(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                          wm_arcade_drone_state_t *d, const char *label, void *user) {
    WmRng *rng = (WmRng *)user;
    int32_t gap, sign_combine;
    (void)d; (void)label;
    if (rr(rng, 7) != 0) return WM_DRONE_CALL_ABORT;
    gap = opp->x_int - self->x_int;
    sign_combine = (int32_t)(opp->x_vel >> 16) ^ gap;
    gap = iabs32(gap);
    if (self->closest_zdist > gap) return WM_DRONE_CALL_ABORT;
    if (sign_combine < 0 || opp->getup_time > 0)
        apply_bytecode_input(self, d, (uint16_t)(PM | KM), 0, 0);
    return WM_DRONE_CALL_ABORT;
}

static int call_drn_roll_init(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                              wm_arcade_drone_state_t *d, const char *label, void *user) {
    (void)self; (void)opp; (void)label;
    return call_drone_chrg(self, opp, d, "drone_chrg", user);
}

static int seek_drn_roll(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                         wm_arcade_drone_state_t *d, void *user) {
    WmRng *rng = (WmRng *)user;
    if (self->player_mode != WM_PMODE_ONGROUND) return 0;
    if (self->closest_xdist > 150) return 0;
    if (self->closest_zdist > 70) return 0;
    {
        uint16_t joy = seekxz_joy(self->x_int, self->z_int, opp->x_int, opp->z_int, 0);
        if (joy == 0) return 0;
        d->joy = (uint16_t)(joy ^ (WM_MOVE_UP | WM_MOVE_DOWN)); /* flip up&down */
    }
    return rr(rng, 0x7f) != 0;
}

static int call_drn_climbtb(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                            wm_arcade_drone_state_t *d, const char *label, void *user) {
    (void)self; (void)opp; (void)label; (void)user;
    /* #lp: this port never has team battles (one wrestler per side), so
       "# on team <= 1" is always true -- the 50% skip roll below it in
       the real source is unreachable here. */
    d->script = "drn_ontb";
    d->script_pc = 0;
    return WM_DRONE_CALL_REDIRECTED;
}

static int seek_drn_ontb(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                         wm_arcade_drone_state_t *d, void *user) {
    (void)user;
    if (self->player_mode == WM_PMODE_ONTURNBKL) {
        if (opp->player_mode == WM_PMODE_ONTURNBKL)
            apply_bytecode_input(self, d, 0, WM_MOVE_DOWN, 0); /* #dn */
        else
            apply_bytecode_input(self, d, KM, 0, 0); /* #jmp */
        return 0;
    }
    /* #not: in-ring is always true (raw INRING==0 branch) and Bret's
       wrestler_num isn't Yoko's (3), so #ering is unreachable here, same
       reasoning as drone_chkrun's own #out. */
    if (opp->player_mode == WM_PMODE_ONTURNBKL || opp->player_mode == WM_PMODE_INAIR2)
        return 0; /* #x */
    {
        int32_t target_x = (self->x_int <= WM_RING_X_CENTER)
            ? WM_RING_X_CENTER - 225 : WM_RING_X_CENTER + 225;
        uint16_t joy = seekxz_joy(self->x_int, self->z_int, target_x, WM_RING_TOP, 32);
        d->joy = joy;
        if (joy == 0) {
            /* Arrived at visinity: push directly into the corner (result discarded). */
            (void)seekxz_joy(self->x_int, self->z_int, target_x, WM_RING_TOP - 10, 0);
        }
    }
    if (self->closest_xdist > 120) return 1;
    if (self->closest_zdist > 70) return 1;
    return 0;
}

static int seek_drn_inair(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                          wm_arcade_drone_state_t *d, void *user) {
    (void)user;
    d->joy = seekxz_joy(self->x_int, self->z_int, opp->x_int, opp->z_int, 0);
    return self->player_mode == WM_PMODE_INAIR2;
}

static int call_drn_opinair_init(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                                 wm_arcade_drone_state_t *d, const char *label, void *user) {
    WmRng *rng = (WmRng *)user;
    (void)opp; (void)label;
    if (rr(rng, 1) != 0) {
        apply_bytecode_input(self, d, (uint16_t)(PM | KM), WM_MOVE_LEFT, 2); /* #run */
        return WM_DRONE_CALL_ABORT;
    }
    return WM_DRONE_CALL_CONTINUE;
}

static int seek_drn_opinair(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                            wm_arcade_drone_state_t *d, void *user) {
    (void)user;
    if (opp->player_mode != WM_PMODE_INAIR2) {
        apply_bytecode_input(self, d, KM, 0, 0);
        return 0;
    }
    {
        int32_t small = self->closest_xdist < self->closest_zdist
            ? self->closest_xdist : self->closest_zdist;
        if (small > 150) return 1;
    }
    apply_bytecode_input(self, d, KM, 0, 0);
    return 0;
}

static int seek_drn_oppdead(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                            wm_arcade_drone_state_t *d, void *user) {
    WmRng *rng = (WmRng *)user;
    int32_t threshold = (self->wrestler_num == 2) ? 32 : 90; /* Undertaker vs everyone */
    d->seek_dir = 0;
    d->seek_dist = 0;
    run_seekdirdist(self, opp, d, rng);
    if (self->closest_dist <= threshold) {
        apply_bytecode_input(self, d, PM, 0, 0);
        return 0;
    }
    opp->anim_mode = (uint16_t)(opp->anim_mode | WM_MODE_OVERLAP);
    if (d->joy != 0) return 1;
    apply_bytecode_input(self, d, PM, 0, 0);
    return 0;
}

static int seek_plain70(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                        wm_arcade_drone_state_t *d, void *user) {
    (void)user;
    d->joy = seekxz_joy(self->x_int, self->z_int, opp->x_int, opp->z_int, 70);
    return d->joy != 0;
}

static int seek_drn_seek(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                         wm_arcade_drone_state_t *d, void *user) {
    WmRng *rng = (WmRng *)user;
    if (self->player_mode != WM_PMODE_NORMAL && self->player_mode != WM_PMODE_BLOCK) return 0;
    if (rr(rng, 0x3f) == 0) return 0;
    d->joy = seekxz_joy(self->x_int, self->z_int, opp->x_int, opp->z_int, 70);
    return d->joy != 0;
}

static int call_drn_retreat_init(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                                 wm_arcade_drone_state_t *d, const char *label, void *user) {
    (void)self; (void)opp; (void)label; (void)user;
    d->seek_dist = 4;
    return WM_DRONE_CALL_CONTINUE;
}

static int seek_drn_retreat(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                            wm_arcade_drone_state_t *d, void *user) {
    WmRng *rng = (WmRng *)user;
    run_seekdirdist(self, opp, d, rng);
    return rr(rng, 0x1f) != 0;
}

static int seek_drn_seekclose(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                              wm_arcade_drone_state_t *d, void *user) {
    WmRng *rng = (WmRng *)user;
    int32_t target_x, target_z;
    if (self->player_mode != WM_PMODE_NORMAL && self->player_mode != WM_PMODE_BLOCK) return 0;
    if (rr(rng, 0x3f) == 0) return 0;
    target_x = opp->x_int + ((self->x_int >= opp->x_int) ? 32 : -32);
    target_z = opp->z_int;
    d->joy = seekxz_joy(self->x_int, self->z_int, target_x, target_z, 23);
    return d->joy != 0;
}

static int call_seek_dispatch(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                              wm_arcade_drone_state_t *d, const char *label, void *user) {
    if (!label) return 0;
    if (strcmp(label, "plain70") == 0) return seek_plain70(self, opp, d, user);
    if (strcmp(label, "drn_roll") == 0) return seek_drn_roll(self, opp, d, user);
    if (strcmp(label, "drn_ontb") == 0) return seek_drn_ontb(self, opp, d, user);
    if (strcmp(label, "drn_inair") == 0) return seek_drn_inair(self, opp, d, user);
    if (strcmp(label, "drn_opinair") == 0) return seek_drn_opinair(self, opp, d, user);
    if (strcmp(label, "drn_oppdead") == 0) return seek_drn_oppdead(self, opp, d, user);
    if (strcmp(label, "drn_seek") == 0) return seek_drn_seek(self, opp, d, user);
    if (strcmp(label, "drn_retreat") == 0) return seek_drn_retreat(self, opp, d, user);
    if (strcmp(label, "drn_seekclose") == 0) return seek_drn_seekclose(self, opp, d, user);
    return 0;
}

static int call_function_dispatch(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                                  wm_arcade_drone_state_t *d, const char *label, void *user) {
    if (!label) return WM_DRONE_CALL_ABORT;
    if (strcmp(label, "drn_run") == 0) return call_drn_run(self, opp, d, label, user);
    if (strcmp(label, "drn_oprun") == 0) return call_drn_oprun(self, opp, d, label, user);
    if (strcmp(label, "drn_roll_init") == 0) return call_drn_roll_init(self, opp, d, label, user);
    if (strcmp(label, "drn_climbtb") == 0) return call_drn_climbtb(self, opp, d, label, user);
    if (strcmp(label, "drn_opinair_init") == 0) return call_drn_opinair_init(self, opp, d, label, user);
    if (strcmp(label, "drn_retreat_init") == 0) return call_drn_retreat_init(self, opp, d, label, user);
    if (strcmp(label, "charge_run_fire") == 0) return call_charge_run_fire(self, opp, d, label, user);
    return WM_DRONE_CALL_ABORT;
}

static int script_call_cb(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                          wm_arcade_drone_state_t *d, const char *label, void *user) {
    if (!label) return WM_DRONE_CALL_ABORT;
    if (strcmp(label, "drone_chkrun") == 0) return call_drone_chkrun(self, opp, d, label, user);
    if (strcmp(label, "drone_chrg") == 0) return call_drone_chrg(self, opp, d, label, user);
    return call_function_dispatch(self, opp, d, label, user);
}

/* ------------------------------------------------------------------ */
/* Named-script resolver. */

#define ENTRY(name, ops) {(name), (ops), sizeof(ops) / sizeof((ops)[0])}

static const wm_arcade_drone_script_t s_scripts[] = {
    ENTRY("#p", ops_p), ENTRY("#sp", ops_sp), ENTRY("#k", ops_k), ENTRY("#sk", ops_sk),
    ENTRY("#spx", ops_spx), ENTRY("#hgrab", ops_hgrab), ENTRY("#flng", ops_flng),
    ENTRY("#htoss", ops_htoss), ENTRY("slhtoss", ops_slhtoss), ENTRY("#ucut", ops_ucut),
    ENTRY("#ddp", ops_ddp), ENTRY("#jp", ops_jp), ENTRY("#j2sp", ops_j2sp),
    ENTRY("#llsk", ops_llsk), ENTRY("#fast", ops_fast), ENTRY("#chrg", ops_chrg),
    ENTRY("#uddsk", ops_uddsk), ENTRY("#lrrsp", ops_lrrsp), ENTRY("#lrrp", ops_lrrp),
    ENTRY("#jpx", ops_jpx), ENTRY("#rp", ops_rp), ENTRY("#rsp", ops_rsp),
    ENTRY("#oghg", ops_oghg), ENTRY("#seeksp", ops_seeksp), ENTRY("#seeksk", ops_seeksk),
    ENTRY("#run", ops_run),
    ENTRY("drn_run", ops_call_drn_run), ENTRY("drn_oprun", ops_call_drn_oprun),
    ENTRY("drn_roll", ops_roll), ENTRY("drn_climbtb", ops_climbtb), ENTRY("drn_ontb", ops_ontb),
    ENTRY("drn_inair", ops_inair), ENTRY("drn_opinair", ops_opinair),
    ENTRY("drn_oppdead", ops_oppdead), ENTRY("drn_seek", ops_seek),
    ENTRY("drn_retreat", ops_retreat), ENTRY("drn_seekclose", ops_seekclose),
    ENTRY("charge_run", ops_charge_run),
    ENTRY("combo_cstrt", ops_combo_cstrt), ENTRY("drn_combo/brt", ops_combo_brt)
};

static const wm_arcade_drone_script_t *resolve_named(const char *label) {
    size_t i;
    for (i = 0; i < sizeof(s_scripts) / sizeof(s_scripts[0]); ++i)
        if (strcmp(s_scripts[i].source_label, label) == 0) return &s_scripts[i];
    return NULL;
}

static const wm_arcade_drone_script_t *resolve_script_cb(const char *label, void *user) {
    if (!label) return NULL;
    if (strcmp(label, "M_shrtblkr") == 0) {
        WmRng *rng = (WmRng *)user;
        return resolve_named(s_M_shrtblkr[rr(rng, 1)]);
    }
    if (strcmp(label, "M_shrtblkrdl") == 0) {
        WmRng *rng = (WmRng *)user;
        return resolve_named(s_M_shrtblkrdl[rr(rng, 1)]);
    }
    if (strcmp(label, "drn_combo") == 0) {
        /* Bret's own drn_combo per-wrestler branch. Never actually
           selected by wm_arcade_drone_main in this port -- see
           check_combo_go_cb below -- kept for completeness/fidelity. */
        return resolve_named("drn_combo/brt");
    }
    return resolve_named(label);
}

static int32_t script_skill_pct_cb(const char *label, int skill, void *user) {
    (void)user;
    if (label && strcmp(label, "sklrep_t") == 0) return wm_arcade_drone_repeat_pct(skill);
    return 0;
}

static int32_t block_base_pct_cb(int skill, void *user) { (void)user; return wm_arcade_drone_block_base_pct(skill); }
static int32_t block_attack_pct_cb(int missed, void *user) { (void)user; return wm_arcade_drone_block_attack_pct(missed); }
static int32_t headhold_delay_max_cb(int skill, void *user) { (void)user; return wm_arcade_drone_headhold_delay_max(skill); }
static int32_t headheld_delay_max_cb(int skill, void *user) { (void)user; return wm_arcade_drone_headheld_delay_max(skill); }

/*
 * CHECK_COMBO_GO (LIFEBAR.ASM:718): this port's own already-established
 * finding (wm_arcade_mode_dead.h) is that it always reports "not lit" --
 * no per-player combo-meter fill is tracked here, only life itself. DRONE.
 * ASM's own drn_combo gate (`jrlt #ncmb ;Can't combo?`) uses the exact same
 * external routine, so it must report the same "can't combo" result here
 * too: returning a negative value (matching LIFEBAR.ASM's own convention)
 * keeps wm_arcade_drone_main's `>= 0` gate closed, exactly like the real
 * arcade's own combo meter -- always empty in this port -- would.
 */
static int check_combo_go_cb(wm_arcade_actor_t *actor, void *user) {
    (void)actor; (void)user;
    return -1;
}

static wm_arcade_actor_t *closest_actor_cb(wm_arcade_actor_t *actor, void *user) {
    (void)user;
    return actor->smart_target;
}

static wm_arcade_actor_t *closest_actor_for_cb(wm_arcade_actor_t *actor, void *user) {
    (void)user;
    return actor->smart_target;
}

static int32_t closest_dist_for_cb(const wm_arcade_actor_t *actor, void *user) {
    (void)user;
    return actor->closest_dist;
}

static void seek_dir_dist_cb(wm_arcade_actor_t *actor, wm_arcade_actor_t *opp,
                             wm_arcade_drone_state_t *drone, void *user) {
    run_seekdirdist(actor, opp, drone, (WmRng *)user);
}

static const wm_arcade_drone_script_list_t *range_script_list_cb(
    const wm_arcade_actor_t *self, const wm_arcade_actor_t *opp,
    int band, int mymode, int opmode, void *user) {
    (void)opp; (void)user;
    if (self->wrestler_num != WM_ROSTER_BRET) return NULL;

    if (band == 2) {
        /* bret_l_t: BBL -1,MODE_ONGROUND,#mdog / WL -1,#mdn (own mode
           wildcard, dispatched purely on the opponent's mode). */
        return (opmode == WM_PMODE_ONGROUND) ? &s_mdog_list : &s_mdn_list;
    }

    /* bret_s_t (band 0) / bret_m_t (band 1) share the same BBL shape;
       BBL's own two mode checks are (self, opponent), verified against
       each list's own comment ("Holding head" -> self==HEADHOLD, "Opp on
       gnd" -> opponent==ONGROUND). */
    if (mymode == WM_PMODE_HEADHOLD) return &s_brM_hh_list;
    if (mymode == WM_PMODE_HEADHELD) return &s_brM_hhr_list;
    if (opmode == WM_PMODE_ONGROUND) return (band == 0) ? &s_M_og_list : &s_Mm_og_list;
    if (opmode == WM_PMODE_ONTURNBKL || opmode == WM_PMODE_CLIMBTURNBKL)
        return &s_M_opptbkl_list;
    return (band == 0) ? &s_brM_n_list : &s_brMm_n_list;
}

/* Matches app.c's own drone_rndrng0_adapter precedent: both DRONE.ASM's
   `rnd` (AND-mask) and `rndrng0` (scaled) share the same @RAND state
   advance, and every real `rnd(mask)` call this port's translation
   actually performs uses a mask of the form 2^k-1, where a uniform 0..mask
   scale is numerically identical to the AND-mask distribution -- see this
   file's own callback derivations. */
static uint32_t rndrng0_adapter(uint32_t max_inclusive, void *user) {
    return wm_rng_rndrng0((WmRng *)user, max_inclusive);
}

wm_arcade_drone_callbacks_t wm_arcade_bret_drone_callbacks(WmRng *rng) {
    wm_arcade_drone_callbacks_t cb;
    memset(&cb, 0, sizeof(cb));
    cb.rnd_upto = rndrng0_adapter;
    cb.rndrng0_upto = rndrng0_adapter;
    cb.closest_actor = closest_actor_cb;
    cb.closest_actor_for = closest_actor_for_cb;
    cb.closest_dist_for = closest_dist_for_cb;
    cb.block_base_pct = block_base_pct_cb;
    cb.block_attack_pct = block_attack_pct_cb;
    cb.headhold_delay_max = headhold_delay_max_cb;
    cb.headheld_delay_max = headheld_delay_max_cb;
    cb.check_combo_go = check_combo_go_cb;
    cb.seek_dir_dist = seek_dir_dist_cb;
    cb.range_script_list = range_script_list_cb;
    cb.resolve_script = resolve_script_cb;
    cb.script_skill_pct = script_skill_pct_cb;
    cb.script_seek = call_seek_dispatch;
    cb.script_call = script_call_cb;
    cb.script_selected = NULL;
    cb.user = rng;
    return cb;
}
