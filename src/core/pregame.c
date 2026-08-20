#include "wm/pregame.h"

#include <string.h>

#include "wm/source_clock.h"

/* DISPLAY.EQU: TSEC = 53. */
#define SOURCE_TSEC WM_SOURCE_TICKS_PER_SEC

/* PROGRESS.ASM constants. */
#define BELT_SETUP_SLEEP_TICKS 2u
#define BELT_SLIDE_STEP 0x18
#define BELT_SLIDE_CLAMP 255
#define BELT_SELECT_TICKS (SOURCE_TSEC * 4u)
#define BELT_FLASH_TICKS (3u * 6u)
#define BELT_POST_FLASH_TICKS 15u
#define PROGRESS_SCROLL_COUNTER_START 100u
#define PROGRESS_HOLD_COUNTER_START 90u
#define PROGRESS_RUN_SPEED_FP (4 << 16)
#define PROGRESS_CLOSE_P_DELAY 20u
#define PROGRESS_CLOSE_P_SPEED (200u / PROGRESS_CLOSE_P_DELAY)
#define PROGRESS_CLOSE_EFFECT_SLEEP (35u + 24u)
#define PROGRESS_CLOSE_FINAL_SLEEP 10u
#define PROGRESS_BIT_ACCEL_FP 0x5000
#define PROGRESS_BIT_KILL_Y_FP (255 << 16)
#define PROGRESS_BIT_INITIAL_DELAY 2u
#define PROGRESS_BIT_SPARK_DELAY 100u
#define PROGRESS_SHAKE_STEP_TICKS 2u
#define PROGRESS_SHAKE_STEPS 12u

static const uint8_t which_music_source[9] = {5,2,1,7,6,4,8,0,3};

/* PROGRESS.ASM tables are run-length pairs: number of matches, opponents. */
static const uint8_t ladder_intercontinental[][2] = {
    {4,1}, {2,2}, {1,3}
};
static const uint8_t ladder_wwf[][2] = {
    {4,2}, {2,3}, {1,3}
};

static bool any_source_button(const wm_input_state *in) {
    if (!in) return false;
    return in->start || in->run || in->light_punch || in->power_punch ||
           in->light_kick || in->power_kick || in->block;
}

/* RNDRNG0 is external/shared Wolf Unit code and is not present in this game's
 * source drop.  Keep the bridge isolated: the ladder *algorithm* below is a
 * direct port; only the entropy primitive is platform-local until RNDRNG0 is
 * recovered from shipped program ROM. */
static uint32_t rndrng0_bridge(wm_pregame_state *s, uint32_t maximum) {
    uint32_t x = s->rng_state ? s->rng_state : 0x57574650u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s->rng_state = x;
    return (uint32_t)(((uint64_t)x * ((uint64_t)maximum + 1u)) >> 32);
}

static void init_temp_table(uint8_t temp[8]) {
    /* PROGRESS.ASM::INIT_TEMP_TABLE stores 7,6,...,0. */
    for (unsigned i = 0; i < 8u; ++i)
        temp[i] = (uint8_t)(7u - i);
}

static void randomize_order(wm_pregame_state *s, uint8_t temp[8]) {
    /* PROGRESS.ASM::RANDOMIZE_ORDER: A2 advances while A10 counts 7 -> 0;
       RNDRNG0(A10) chooses an inclusive offset in the remaining suffix. */
    for (unsigned current = 0; current < 8u; ++current) {
        unsigned maximum = 7u - current;
        unsigned offset = rndrng0_bridge(s, maximum);
        unsigned other = current + offset;
        uint8_t t = temp[current];
        temp[current] = temp[other];
        temp[other] = t;
    }
}

static uint8_t fetch_next_opponent(wm_pregame_state *s,
                                   uint8_t temp[8], unsigned *temp_index) {
    if (*temp_index >= 8u) {
        randomize_order(s, temp);
        *temp_index = 0u;
    }
    return temp[(*temp_index)++];
}

static uint32_t pack_ladder_entry(uint8_t count,
                                  uint8_t op1, uint8_t op2, uint8_t op3) {
    return ((uint32_t)count << 24) |
           ((uint32_t)op3 << 16) |
           ((uint32_t)op2 << 8) |
           (uint32_t)op1;
}

static void init_ladder_table(wm_pregame_state *s) {
    uint8_t temp[8];
    unsigned temp_index = 0u;
    unsigned out = 0u;
    memset(s->ladder, 0, sizeof(s->ladder));

    init_temp_table(temp);
    randomize_order(s, temp);

    const uint8_t (*runs)[2] =
        s->belt_type == WM_PREGAME_BELT_WWF ? ladder_wwf : ladder_intercontinental;
    const unsigned run_count = 3u;

    for (unsigned r = 0; r < run_count; ++r) {
        unsigned matches = runs[r][0];
        uint8_t count = runs[r][1];
        for (unsigned m = 0; m < matches && out < WM_PREGAME_LADDER_ENTRIES; ++m) {
            uint8_t op1 = fetch_next_opponent(s, temp, &temp_index);
            uint8_t op2 = count >= 2u ? fetch_next_opponent(s, temp, &temp_index) : 0u;
            uint8_t op3 = count >= 3u ? fetch_next_opponent(s, temp, &temp_index) : 0u;
            s->ladder[out++].packed = pack_ladder_entry(count, op1, op2, op3);
        }
    }
    s->current_ladder_index = -1; /* source CURRENT_LADDER = LADDER-20h */
}

static uint8_t ladder_slot_for_source_wrestler(uint8_t source_wrestler) {
    /* TEMP_LADDER is eight packed slots (0..7); SORT_OUT_WRESTLER_NUM maps
       packed slot 7 to live wrestler 8 (Lex).  Use the packed-domain value
       when applying scramble_table_entry's exclusion bitmask. */
    return source_wrestler == 8u ? 7u : (source_wrestler & 7u);
}

static unsigned count_bits8(uint8_t bits) {
    unsigned n = 0u;
    for (unsigned i = 0; i < 8u; ++i)
        n += (bits >> i) & 1u;
    return n;
}

static uint8_t get_rnd_wrestler(wm_pregame_state *s, uint8_t excluded) {
    /* PROGRESS.ASM::get_rnd_wrestler chooses the Nth non-excluded packed
       wrestler. RNDRNG0's inclusive-range contract is preserved by the
       isolated bridge above. */
    unsigned excluded_count = count_bits8(excluded);
    unsigned maximum = 7u - excluded_count;
    unsigned nth = rndrng0_bridge(s, maximum) + 1u;
    for (uint8_t w = 0; w < 8u; ++w) {
        if (excluded & (uint8_t)(1u << w))
            continue;
        if (--nth == 0u)
            return w;
    }
    return 0u;
}

static uint32_t scramble_table_entry(wm_pregame_state *s,
                                     int ladder_index,
                                     uint32_t packed) {
    uint8_t count = (uint8_t)(packed >> 24);
    if (count <= 1u)
        return packed;

    /* PROGRESS.ASM::scramble_table_entry excludes the human and, except for
       the first entry, every drone from the previous fight.  Final-battle
       replacement and the PCNT 1-in-32 triple-Doink easter egg depend on
       source-global state that is not reached by the current first-pregame
       port, so keep those branches at the later-ladder boundary. */
    uint8_t excluded = (uint8_t)(1u << ladder_slot_for_source_wrestler(
        s->player_source_wrestler));
    if (ladder_index > 0) {
        uint32_t prev = s->ladder[ladder_index - 1].packed;
        unsigned prev_count = (unsigned)(prev >> 24);
        if (prev_count > 3u) prev_count = 3u;
        for (unsigned i = 0; i < prev_count; ++i) {
            uint8_t w = (uint8_t)((prev >> (i * 8u)) & 0xffu);
            if (w < 8u) excluded |= (uint8_t)(1u << w);
        }
    }

    /* Source makes one retry for duplicate picks rather than looping until
       unique; preserve that exact slightly-odd behavior. */
    uint8_t a3 = get_rnd_wrestler(s, excluded);
    uint8_t a4 = get_rnd_wrestler(s, excluded);
    if (a4 == a3)
        a4 = get_rnd_wrestler(s, excluded);
    uint8_t a5 = get_rnd_wrestler(s, excluded);
    if (a5 == a3 || a5 == a4)
        a5 = get_rnd_wrestler(s, excluded);

    /* The assembly packs its first/second/third picks high-to-low, leaving the
       third pick in OP1.  NEXT_IN_LADDER then moves Shawn (4), if present,
       into the final occupied opponent slot. */
    uint8_t op1 = a5;
    uint8_t op2 = a4;
    uint8_t op3 = a3;
    if (op1 == 4u) {
        uint8_t t = op1; op1 = op2; op2 = t;
    }
    if (count >= 3u && op2 == 4u) {
        uint8_t t = op2; op2 = op3; op3 = t;
    }
    return pack_ladder_entry(count, op1, op2, op3);
}

static void next_in_ladder(wm_pregame_state *s) {
    if (s->current_ladder_index + 1 < (int)WM_PREGAME_PLAYABLE_LADDER_ENTRIES)
        ++s->current_ladder_index;
    if (s->current_ladder_index < 0)
        s->current_ladder_index = 0;

    uint32_t p = s->ladder[s->current_ladder_index].packed;
    if ((uint8_t)(p >> 24) > 1u) {
        p = scramble_table_entry(s, s->current_ladder_index, p);
        s->ladder[s->current_ladder_index].packed = p;
    }
    s->opponent_count = (uint8_t)(p >> 24);
    if (s->opponent_count > WM_PREGAME_MAX_OPPONENTS)
        s->opponent_count = WM_PREGAME_MAX_OPPONENTS;
    s->opponents[0] = (uint8_t)(p & 0xffu);
    s->opponents[1] = (uint8_t)((p >> 8) & 0xffu);
    s->opponents[2] = (uint8_t)((p >> 16) & 0xffu);
}

static void enter_progress(wm_pregame_state *s, wm_audio_state *audio) {
    init_ladder_table(s);          /* SELECT.ASM pregame_show */
    next_in_ladder(s);             /* PUT_UP_PROGRESS */
    s->progress_world_x_fp = 0;
    /* CREATE_TEMP_WRESTLER::RUNNING_MAN starts at [140,0], Y=100 and
       standing_addr. Opponents are created on waiting_addr. */
    s->progress_player_x_fp = (140 << 16);
    s->progress_temp_speed_fp = 0;
    s->progress_player_action = WM_PROGRESS_ACT_STAND;
    s->progress_opponent_action = WM_PROGRESS_ACT_WAIT;
    s->progress_player_anim_ticks = 0;
    s->progress_opponent_anim_ticks = 0;
    s->phase_ticks = 0;
    s->progress_counter = PROGRESS_SCROLL_COUNTER_START;
    s->flash_frame = 0;
    s->phase = WM_PREGAME_PROGRESS_SETUP;

    /* PUT_UP_PROGRESS does SNDSND 2056, then WHICH_MUSIC for the human.
       The current N64 DCS bank does not yet map those source commands, so do
       not substitute a guessed track. Preserve the exact IDs in state/code. */
    (void)audio;
    (void)which_music_source[s->player_source_wrestler < 9u ?
                             s->player_source_wrestler : 0u];
}

static int32_t progress_speed_fp(unsigned remaining) {
    /* PROGRESS.ASM::SET_SCROLL_SPEED. RUN_SPEED is [4,0]. During the last
       sixteen counts it subtracts ((16-remaining)>>2)<<15. */
    int32_t speed = PROGRESS_RUN_SPEED_FP;
    if (remaining <= 16u) {
        unsigned delta = 16u - remaining;
        speed -= (int32_t)((delta >> 2) << 15);
    }
    return speed;
}


/* PROGRESS.ASM::SHAKE_TABLE, packed [Y,X] plus PRCSLP 2 per entry. */
static const int8_t progress_shake_table[PROGRESS_SHAKE_STEPS][2] = {
    {-1,-1}, { 2, 1}, {-2, 0}, {-3, 1}, { 2, 3}, { 0,-2},
    {-1,-1}, { 2, 1}, {-2, 0}, {-3, 1}, { 2, 3}, { 0,-2},
};

static void progress_set_shake(wm_pregame_state *s, unsigned elapsed) {
    if (!s) return;
    unsigned i = elapsed / PROGRESS_SHAKE_STEP_TICKS;
    if (i >= PROGRESS_SHAKE_STEPS) {
        s->progress_shake_x = 0;
        s->progress_shake_y = 0;
        return;
    }
    s->progress_shake_y = progress_shake_table[i][0];
    s->progress_shake_x = progress_shake_table[i][1];
}

/* CLOSE_PROGRESS_SCREEN: P_DELAY=20, speed=200/P_DELAY=10.
   get_all_buttons_cur2 swaps A8/A9 when any current button is active. */

static uint8_t progress_bit_anim_len(uint8_t kind) {
    static const uint8_t lens[10] = {10,10,5,5,10,10,5,5,10,10};
    return kind < 10u ? lens[kind] : 1u;
}

/* Direct PROGRESS.ASM::CREATE_BITS call ordering/ranges. */
static void progress_create_bits(wm_pregame_state *s) {
    if (!s || s->progress_bits_created) return;

    memset(s->progress_bits, 0, sizeof(s->progress_bits));
    uint32_t ymax = 40u;
    int32_t shared_y_fp = 0;

    for (unsigned i = 0; i < WM_PREGAME_PROGRESS_BITS; ++i) {
        const unsigned remaining = WM_PREGAME_PROGRESS_BITS - i;
        wm_progress_bit *b = &s->progress_bits[i];

        if ((remaining & 3u) == 0u) {
            shared_y_fp = (int32_t)(rndrng0_bridge(s, ymax) << 16);
            ymax += 20u;
        }

        b->active = true;
        b->x_fp = (200 << 16);
        b->y_fp = shared_y_fp;
        b->yvel_fp = -(int32_t)rndrng0_bridge(s, 0x58000u);
        b->xvel_fp = (int32_t)rndrng0_bridge(s, 0x80000u) - 0x40000;
        b->kind = (uint8_t)rndrng0_bridge(s, 13u);
        b->delay = PROGRESS_BIT_INITIAL_DELAY;
        b->anim_index = 0u;
        b->anim_started = false;
    }

    s->progress_bits_created = true;
}

/* Direct PROGRESS.ASM::MOVE_BITS fixed-point physics/animation timing. */
static void progress_tick_bits(wm_pregame_state *s) {
    if (!s || !s->progress_bits_created) return;

    for (unsigned i = 0; i < WM_PREGAME_PROGRESS_BITS; ++i) {
        wm_progress_bit *b = &s->progress_bits[i];
        if (!b->active) continue;

        b->yvel_fp += PROGRESS_BIT_ACCEL_FP;
        b->y_fp += b->yvel_fp;
        if (b->y_fp >= PROGRESS_BIT_KILL_Y_FP) {
            b->active = false;
            continue;
        }
        b->x_fp += b->xvel_fp;

        if (b->delay > 0u) --b->delay;
        if (b->delay != 0u) continue;

        const uint8_t len = progress_bit_anim_len(b->kind);
        if (!b->anim_started) {
            b->anim_started = true;
            b->anim_index = 0u;
        } else {
            b->anim_index = (uint8_t)((b->anim_index + 1u) % len);
        }
        b->delay = b->kind < 10u ? 1u : PROGRESS_BIT_SPARK_DELAY;
    }
}

static void begin_progress_close(wm_pregame_state *s, bool fast) {
    if (!s) return;
    s->phase = WM_PREGAME_PROGRESS_CLOSE;
    s->phase_ticks = 0;
    s->progress_close_delay =
        fast ? PROGRESS_CLOSE_P_SPEED : PROGRESS_CLOSE_P_DELAY;
    s->progress_close_speed =
        fast ? PROGRESS_CLOSE_P_DELAY : PROGRESS_CLOSE_P_SPEED;
    s->progress_close_move_ticks = 0;
    s->progress_close_post_ticks = 0;
    s->progress_shake_x = 0;
    s->progress_shake_y = 0;
    memset(s->progress_bits, 0, sizeof(s->progress_bits));
    s->progress_bits_created = false;
}

/* CLOSE_PROGRESS_SCREEN process timing. Renderer owns INIT_BLOC,
   SETUP_LOGOS and MOVE_BLOC geometry from the same source routine. */
static void tick_progress_close(wm_pregame_state *s) {
    if (!s) return;
    ++s->phase_ticks;

    if (s->progress_close_move_ticks < s->progress_close_delay) {
        ++s->progress_close_move_ticks;
        if (s->progress_close_move_ticks == s->progress_close_delay &&
            s->progress_close_delay != PROGRESS_CLOSE_P_SPEED)
            progress_set_shake(s, 0);
            progress_create_bits(s);
        return;
    }

    progress_tick_bits(s);

    const bool fast = s->progress_close_delay == PROGRESS_CLOSE_P_SPEED;
    const unsigned effects = fast ? 0u : PROGRESS_CLOSE_EFFECT_SLEEP;
    const unsigned total = effects + PROGRESS_CLOSE_FINAL_SLEEP;

    if (s->progress_close_post_ticks < total) {
        ++s->progress_close_post_ticks;
        if (!fast) {
            if (s->progress_close_post_ticks <= effects)
                progress_set_shake(s, s->progress_close_post_ticks);
            else
                progress_set_shake(s, PROGRESS_SHAKE_STEPS *
                                      PROGRESS_SHAKE_STEP_TICKS);
        }
        if (s->progress_close_post_ticks < total)
            return;
    }

    s->progress_shake_x = 0;
    s->progress_shake_y = 0;
    s->ready_for_match = true;
    s->finished = true;
    s->phase = WM_PREGAME_READY_FOR_MATCH;
}

void wm_pregame_init(wm_pregame_state *s,
                     uint8_t selected_source_wrestler,
                     wm_wrestler_id selected_roster_wrestler) {
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->phase = WM_PREGAME_BELT_SETUP;
    s->belt_type = WM_PREGAME_BELT_INTERCONTINENTAL; /* INTER_DEFAULT .set 1 */
    s->player_source_wrestler = selected_source_wrestler < 9u ? selected_source_wrestler : 0u;
    s->player_roster_wrestler = selected_roster_wrestler;
    s->current_ladder_index = -1;
    s->belt_world_y = 0;
    s->match_count = 1u; /* WRESTLE.ASM increments match_cnt before pregame_show. */
    s->win_streak = 0u; /* Fresh one-player run: both source win-streak counters are clear. */
    s->rng_state = 0x50524731u ^ ((uint32_t)s->player_source_wrestler * 0x9E3779B9u);
}

void wm_pregame_tick(wm_pregame_state *s,
                     const wm_input_state *input,
                     wm_audio_state *audio) {
    if (!s || s->finished) return;

    /* PROGRESS.ASM starts both palette_cycle processes before the two-tick
       setup sleep and kills them only after flash_it + the 15-tick hold. */
    if (s->phase >= WM_PREGAME_BELT_SETUP && s->phase <= WM_PREGAME_BELT_FLASH)
        ++s->belt_anim_ticks;

    switch (s->phase) {
        case WM_PREGAME_BELT_SETUP:
            if (++s->phase_ticks >= BELT_SETUP_SLEEP_TICKS) {
                s->phase_ticks = 0;
                s->phase = WM_PREGAME_BELT_SLIDE;
            }
            break;

        case WM_PREGAME_BELT_SLIDE:
            s->belt_world_y += BELT_SLIDE_STEP;
            if (s->belt_world_y >= 252) {
                s->belt_world_y = BELT_SLIDE_CLAMP;
                s->phase_ticks = 0;
                s->belt_wait_ticks = BELT_SELECT_TICKS;
                s->phase = WM_PREGAME_BELT_SELECT;
            }
            break;

        case WM_PREGAME_BELT_SELECT:
            if (input) {
                if (input->stick_y > 0)
                    s->belt_type = WM_PREGAME_BELT_INTERCONTINENTAL;
                else if (input->stick_y < 0)
                    s->belt_type = WM_PREGAME_BELT_WWF;
            }
            if (any_source_button(input) || s->belt_wait_ticks == 0u) {
                s->flash_ticks = BELT_FLASH_TICKS; /* flash_it: 3 loops, two SLEEPK 3 halves */
                s->phase_ticks = 0;
                s->phase = WM_PREGAME_BELT_FLASH;
                break;
            }
            --s->belt_wait_ticks;
            break;

        case WM_PREGAME_BELT_FLASH:
            ++s->phase_ticks;
            if (s->flash_ticks > 0u) --s->flash_ticks;
            if (s->flash_ticks == 0u &&
                s->phase_ticks >= (BELT_FLASH_TICKS + BELT_POST_FLASH_TICKS)) {
                enter_progress(s, audio);
            }
            break;

        case WM_PREGAME_PROGRESS_SETUP:
            /* After OPEN_SCREEN_LINE, source sets TEMP_SPEED=RUN_SPEED and
               changes player channel 1 to running_addr. */
            s->progress_temp_speed_fp = PROGRESS_RUN_SPEED_FP;
            s->progress_player_action = WM_PROGRESS_ACT_RUN;
            s->progress_player_anim_ticks = 0;
            s->progress_opponent_action = WM_PROGRESS_ACT_WAIT;
            s->progress_opponent_anim_ticks = 0;
            s->phase_ticks = 0;
            s->phase = WM_PREGAME_PROGRESS_SCROLL;
            break;

        case WM_PREGAME_PROGRESS_SCROLL: {
            s->flash_frame = (s->flash_frame + 1u) & 15u;
            /* MOVE_PROGRESS exits directly on current buttons. It does not
               enter STILL_PROGRESS first. */
            if (any_source_button(input)) {
                begin_progress_close(s, true);
                tick_progress_close(s);
                break;
            }

            /* The temporary wrestler is a separate process: its XVEL is the
               shared TEMP_SPEED while the background gets SET_SCROLL_SPEED. */
            s->progress_player_x_fp += s->progress_temp_speed_fp;

            if (s->progress_counter <= 16u) {
                /* SET_SCROLL_SPEED clears TEMP_SPEED and reissues
                   standing_addr during the deceleration tail. */
                s->progress_temp_speed_fp = 0;
                s->progress_player_action = WM_PROGRESS_ACT_STAND;
                s->progress_player_anim_ticks = 0;
            } else {
                ++s->progress_player_anim_ticks;
            }
            ++s->progress_opponent_anim_ticks;
            s->progress_world_x_fp += progress_speed_fp(s->progress_counter);

            if (s->progress_counter == 0u) {
                /* After MOVE_PROGRESS, all live opponents switch to
                   clever_addr before STILL_PROGRESS begins. */
                s->progress_counter = PROGRESS_HOLD_COUNTER_START;
                s->progress_opponent_action = WM_PROGRESS_ACT_CLEVER;
                s->progress_opponent_anim_ticks = 0;
                s->phase = WM_PREGAME_PROGRESS_HOLD;
            } else {
                --s->progress_counter;
            }
            break;
        }

        case WM_PREGAME_PROGRESS_HOLD:
            s->flash_frame = (s->flash_frame + 1u) & 15u;
            ++s->progress_player_anim_ticks;
            ++s->progress_opponent_anim_ticks;
            /* STILL_PROGRESS also exits directly on current buttons. */
            if (any_source_button(input)) {
                begin_progress_close(s, true);
                tick_progress_close(s);
                break;
            }
            if (s->progress_counter == 80u) {
                s->progress_player_action = WM_PROGRESS_ACT_TAUNT;
                s->progress_player_anim_ticks = 0;
            }
            if (s->progress_counter == 0u) {
                begin_progress_close(s, false);
                tick_progress_close(s);
            } else {
                --s->progress_counter;
            }
            break;

        case WM_PREGAME_PROGRESS_CLOSE:
            tick_progress_close(s);
            break;

        case WM_PREGAME_READY_FOR_MATCH:
        default:
            s->ready_for_match = true;
            s->finished = true;
            break;
    }
}

uint8_t wm_pregame_opponent_at(const wm_pregame_state *s, unsigned index) {
    if (!s || index >= s->opponent_count || index >= WM_PREGAME_MAX_OPPONENTS)
        return 0xffu;
    uint8_t w = s->opponents[index];
    /* PROGRESS.ASM::SORT_OUT_WRESTLER_NUM promotes packed ladder slot 7
       (the unused source slot) to live wrestler 8, Lex Luger. */
    return w == 7u ? 8u : w;
}

const char *wm_pregame_phase_name(wm_pregame_phase phase) {
    switch (phase) {
        case WM_PREGAME_BELT_SETUP: return "BELT_SETUP";
        case WM_PREGAME_BELT_SLIDE: return "BELT_SLIDE";
        case WM_PREGAME_BELT_SELECT: return "BELT_SELECT";
        case WM_PREGAME_BELT_FLASH: return "BELT_FLASH";
        case WM_PREGAME_PROGRESS_SETUP: return "PROGRESS_SETUP";
        case WM_PREGAME_PROGRESS_SCROLL: return "PROGRESS_SCROLL";
        case WM_PREGAME_PROGRESS_HOLD: return "PROGRESS_HOLD";
        case WM_PREGAME_PROGRESS_CLOSE: return "PROGRESS_CLOSE";
        case WM_PREGAME_READY_FOR_MATCH: return "READY_FOR_MATCH";
        default: return "UNKNOWN";
    }
}
