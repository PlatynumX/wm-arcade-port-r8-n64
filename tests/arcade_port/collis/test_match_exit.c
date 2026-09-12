/*
 * WRESTLE.ASM:1116, the code after `JSRP start_match` -- "The only
 * time we return from start_match is when the match is over and the
 * game must goto: 1. Buy-in screen ... 2. Ladder screen for the next
 * matchup".
 *
 * app.c had no such return at all: WM_APP_MODE_MATCH was terminal, so
 * a match that finished left the game ticking a decided match for
 * ever. The same dead end a decided round and a finished match each
 * used to be, one and two levels down.
 */
#include <assert.h>
#include <string.h>

#include "wm/app.h"
#include "wm/match.h"
#include "wm_arcade_roster.h"

static wm_app A;

/* Drop the app straight into a match, past select and pregame, so
   these tests are about the EXIT rather than the entry. */
static void into_match(void) {
    memset(&A, 0, sizeof A);
    wm_app_init(&A);
    A.mode = WM_APP_MODE_SELECT;
    wm_pregame_init(&A.pregame, (uint8_t)WM_ROSTER_TAKER,
                    WM_WRESTLER_UNDERTAKER, &A.rng);
    A.mode = WM_APP_MODE_MATCH_INIT;
    wm_app_tick(&A, NULL);
    assert(A.mode == WM_APP_MODE_MATCH);
    assert(A.match.match_over == 0);
}

/* Force the match to its end without fighting 4000 ticks of it. */
static void finish_match(int winner_side) {
    wm_input_state in;
    int i;
    memset(&in, 0, sizeof in);
    A.match.score.p1rounds = winner_side == 0 ? 2 : 0;
    A.match.score.p2rounds = winner_side == 1 ? 2 : 0;
    A.match.score.match_winner = winner_side == 0 ? 1 : 2;
    wm_match_end_start(&A.match.match_end);
    for (i = 0; i < 4000 && A.mode == WM_APP_MODE_MATCH; ++i)
        wm_app_tick(&A, &in);
    assert(A.mode == WM_APP_MODE_MATCH_OVER);
    /* WM_APP_MODE_MATCH_OVER is one tick: it decides and moves on. */
    wm_app_tick(&A, &in);
    assert(A.mode != WM_APP_MODE_MATCH && A.mode != WM_APP_MODE_MATCH_OVER);
}

/*
 * `move @match_over,a0 / jrz #not_over ... RETP`. The match now says
 * when it is over, which is the whole hinge.
 */
static void test_match_says_it_is_over(void) {
    into_match();
    finish_match(0);
    /* LIFEBAR.ASM:2985 `MOVK 2,A0 / move a0,@match_over`. */
    assert(A.match.match_over == 2);
}

/*
 * "This player will keep on playing. Display ladder of progression
 * which shows his next opponent." `jruc do_pregame` -- and NOT
 * through the select screen.
 */
static void test_a_win_goes_back_to_the_pregame(void) {
    uint32_t matches;
    int rung;

    into_match();
    matches = A.pregame.match_count;
    rung = A.pregame.current_ladder_index;

    finish_match(0);
    assert(A.mode == WM_APP_MODE_PREGAME);
    /* `move @match_cnt,a0 / inc a0`. */
    assert(A.pregame.match_count == matches + 1u);
    /* The ladder is not rebuilt and not rewound: PUT_UP_PROGRESS will
       advance it one rung from here. */
    assert(A.pregame.current_ladder_index == rung);
    /* `CLR A0 / MOVE A0,@DONE_HOWARD`. */
    assert(!A.done_howard);
    /* `movi 60,a0 / move a0,@are_we_waiting_f`. */
    assert(A.are_we_waiting_f == 60u);
}

/*
 * The bug this uncovered, and it only bites once a second match is
 * reachable: enter_progress used to call INIT_LADDER_TABLE, which the
 * source calls once per GAME (ATTRACT.ASM:595, SELECT.ASM's
 * GAME_BEATEN) and never from the pregame. Rebuilding it here put the
 * player back on rung zero, so he would fight the first opponent for
 * ever.
 */
static void test_the_ladder_advances_between_matches(void) {
    wm_input_state in;
    int i;
    int first, second;

    memset(&in, 0, sizeof in);
    into_match();
    /* Run the pregame through once to land on a rung. */
    A.mode = WM_APP_MODE_PREGAME;
    for (i = 0; i < 4000 && A.mode == WM_APP_MODE_PREGAME; ++i)
        wm_app_tick(&A, &in);
    first = A.pregame.current_ladder_index;
    assert(first >= 0);

    /* Win, come back, run the pregame again. */
    while (A.mode != WM_APP_MODE_MATCH && i < 8000) {
        wm_app_tick(&A, &in);
        ++i;
    }
    assert(A.mode == WM_APP_MODE_MATCH);
    finish_match(0);
    assert(A.mode == WM_APP_MODE_PREGAME);
    for (i = 0; i < 4000 && A.mode == WM_APP_MODE_PREGAME; ++i)
        wm_app_tick(&A, &in);
    second = A.pregame.current_ladder_index;
    assert(second == first + 1);
}

/*
 * `#go_buyin`: a human who lost gets SELECT.ASM's continue offer --
 * translated for a long time in wm/select_continue.h, initialised by
 * app.c, and never once reached.
 */
static void test_a_loss_offers_a_continue(void) {
    int rung;

    into_match();
    /* As if the pregame had already put him on the first rung. */
    A.pregame.current_ladder_index = 0;
    rung = A.pregame.current_ladder_index;

    finish_match(1);
    assert(A.mode == WM_APP_MODE_CONTINUE);
    /* "The cpu won": `clr a0 / move a0,@match_winner`. */
    assert(A.last_match_winner == 0);
    /*
     * `move @CURRENT_LADDER,A0,L / subi 20h,a0` -- "decrement
     * CURRENT_LADDER, because NEXT_IN_LADDER automatically increments
     * it". Continue and you meet the SAME opponent, not the next.
     */
    assert(A.pregame.current_ladder_index == rung - 1);
}

/* Accepting it puts the player back in the pregame. */
static void test_continuing_resumes_the_game(void) {
    wm_input_state in;
    uint32_t matches;
    int i;

    into_match();
    A.awards.win_streak[0] = 4;
    finish_match(1);
    assert(A.mode == WM_APP_MODE_CONTINUE);
    matches = A.pregame.match_count;

    /* A Start still held from the match must not buy in by itself. */
    memset(&in, 0, sizeof in);
    in.start = true;
    for (i = 0; i < 5; ++i) wm_app_tick(&A, &in);
    assert(A.mode == WM_APP_MODE_CONTINUE);

    /* A fresh press does. */
    in.start = false;
    wm_app_tick(&A, &in);
    in.start = true;
    wm_app_tick(&A, &in);
    assert(A.mode == WM_APP_MODE_PREGAME);
    assert(A.pregame.match_count == matches + 1u);
    /* "Clear the loser's wincount." */
    assert(A.awards.win_streak[0] == 0);
}

/*
 * SELECT.ASM:1190 do_game_over, which a declined continue used to
 * skip straight past. Four seconds of GAME OVER with the master
 * volume fading under it, then attract.
 */
static void test_declining_goes_through_game_over(void) {
    wm_input_state in;
    int i;
    uint8_t vol_at_entry;

    memset(&in, 0, sizeof in);
    into_match();
    A.awards.icon_total[0] = 7;
    A.awards.icon_total[1] = 3;
    finish_match(1);
    assert(A.mode == WM_APP_MODE_CONTINUE);

    for (i = 0; i < 4000 && A.mode == WM_APP_MODE_CONTINUE; ++i)
        wm_app_tick(&A, &in);
    assert(A.mode == WM_APP_MODE_GAME_OVER);

    /* `MOVI 100,A8 / CREATE FADE_PID,FADE_MASTER_VOL` -- started on
       entry and running under the wait. */
    vol_at_entry = A.sound.master_volume;
    assert(vol_at_entry == WM_SOUND_ADJVOLUME_DEFAULT);
    assert(A.volume_fade.active);

    /* The fade is 100 ticks and the hold is 4*53, so it reaches
       silence well before the screen goes. */
    for (i = 0; i < 100; ++i) wm_app_tick(&A, &in);
    assert(A.mode == WM_APP_MODE_GAME_OVER);
    assert(!A.volume_fade.active);
    assert(A.sound.master_volume == 0);

    /* `SLEEP TSEC*4` in total. */
    for (i = 0; i < 4000 && A.mode == WM_APP_MODE_GAME_OVER; ++i)
        wm_app_tick(&A, &in);
    assert(A.mode == WM_APP_MODE_ATTRACT);
    assert(i < 4 * 53);

    /*
     * ADJVOLUME is read back and set_volume restores it, so attract
     * does not start silent -- the fade would otherwise be the last
     * word on the volume for the rest of the machine's life.
     */
    assert(A.sound.master_volume == WM_SOUND_ADJVOLUME_DEFAULT);
    /* `clear_icon_total` for both players. */
    assert(A.awards.icon_total[0] == 0);
    assert(A.awards.icon_total[1] == 0);
}

/* Letting it run out ends the game and the attract loop takes over. */
static void test_declining_goes_back_to_attract(void) {
    wm_input_state in;
    int i;

    memset(&in, 0, sizeof in);
    into_match();
    finish_match(1);
    assert(A.mode == WM_APP_MODE_CONTINUE);

    /*
     * Ten digits at two seconds each, and nothing is pressed, so it
     * times out on its own. WM_SELECT_CONTINUE_INITIAL_DIGIT is 9 and
     * a digit is 53*2 ticks.
     */
    for (i = 0; i < 4000 && A.mode == WM_APP_MODE_CONTINUE; ++i)
        wm_app_tick(&A, &in);
    assert(A.mode == WM_APP_MODE_GAME_OVER);
    assert(i > 10 * (int)WM_SELECT_CONTINUE_TICKS_PER_DIGIT / 2);
    for (i = 0; i < 4000 && A.mode != WM_APP_MODE_ATTRACT; ++i)
        wm_app_tick(&A, &in);
    assert(A.mode == WM_APP_MODE_ATTRACT);
}

int main(void) {
    test_match_says_it_is_over();
    test_a_win_goes_back_to_the_pregame();
    test_the_ladder_advances_between_matches();
    test_a_loss_offers_a_continue();
    test_continuing_resumes_the_game();
    test_declining_goes_back_to_attract();
    test_declining_goes_through_game_over();
    return 0;
}
