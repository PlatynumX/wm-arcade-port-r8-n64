/*
 * Every animation program in the game, played.
 *
 * The port emits 1,530 animation programs out of the wrestlers' sequence
 * files, and until now no test touched more than the dozen or so it
 * thought to name by hand. That is how three real defects shipped:
 *
 *   ANI_REPEAT was executed as ANI_END, so 169 programs died after one
 *   pass; ANI_IFROPE and ANI_IFNOTROPE were emitted with no branch
 *   destination, so 75 branches walked off the end of their program and
 *   killed the animation before its first frame; 1,794 label entries
 *   named an op index one past their program's last op; and three
 *   animations were refused outright because they branch into the head
 *   of a later routine, which the emitter could not see as a label.
 *
 * None of them needed a subtle test to catch. Each one shows up the
 * moment you play the whole corpus and ask the obvious question: did a
 * frame ever appear? So this plays all of them, with the physics running,
 * and asserts the shape of the answer.
 *
 * PHYSICS MATTERS HERE, and getting that wrong is how this test nearly
 * cried wolf. Played with no integrator, nineteen programs never showed a
 * frame -- every one of them a knockdown parked on ANI_WAITHITGND, which
 * holds until the wrestler is on the ground. Nothing was moving him to
 * the ground, so nothing ever landed. WRESTLE.ASM:2457's main loop runs
 *
 *     wrestler_veladd -> wrestler_friction -> animate_wrestler
 *
 * and wm_match_tick (src/core/match.c:584) keeps that order, so the tick
 * loop below does too. With it, the nineteen become four.
 *
 * AND THE HAND-OFFS ARE FOLLOWED. ANI_CHANGEANIM does not stop an
 * animation, it replaces it: the interpreter sets exec->become and ends,
 * and wrestler_backend.c:196 drives that straight back through
 * change_anim_label so the target gets its own header. A test that
 * stopped at the first `ended` would call every routine that opens with
 * a hand-off frameless, which is what the remaining four were. Followed,
 * the four become one -- and that one is a data table, not an animation.
 */
#include "wm/anim_program.h"
#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wm_arcade_veladd.h"
#include "wm/arcade/wmania_ring_geometry.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* The corpus as it stands. Emitting more is always fine; emitting fewer
   means the emitter started refusing something it used to accept. */
#define CORPUS_FLOOR 1532u

/* How long to play each program. Long enough that everything with an end
   reaches it -- the longest terminating program in the corpus finishes
   well inside this -- and short enough that all of them stay quick. */
#define TICKS 400

/* How many ANI_CHANGEANIM hand-offs to follow before giving up. Two
   animations that become each other would otherwise never come to rest. */
#define MAX_HANDOFFS 8

/* Distinct frame positions one playthrough may visit before repeating.
   Comfortably above the longest program in the corpus times the hand-off
   limit; the test asserts it was enough rather than assuming it. */
#define SEEN_MAX 8192

/* Chains that come to rest rather than loop, as measured -- a program
   plus every ANI_CHANGEANIM it hands off to. One that used to come to
   rest and now runs forever is a hang in the making, so this may grow
   but must not shrink. The ones that do NOT come to rest are not left
   as an unexplained remainder: see test_nothing_hangs below. */
#define ENDING_FLOOR 1240

/*
 * The programs that are still running after TICKS ticks and have shown
 * no frame twice -- because they have shown none at all.
 *
 * Both end on ANI_ROT, whose own comment in ANIM.ASM:4441 is "just sit
 * and do nothing". ANI_ROT holds the frame that is showing and never
 * advances, which is exactly how a dead wrestler stays down -- and
 * started cold, with nothing showing yet, there is no frame to hold.
 * A wrestler reaching either in a match already has one.
 *
 * WRESTLE2.ASM:3992 xxx_dead_anim sets MODE_DEAD and runs two ANI_CODE
 * routines first. FINISEQ.ASM:972 disappear_wrestler is the end of the
 * Undertaker's coffin finish: MODE_INVISIBLE plus MODE_DEAD, and then
 * nothing, because the man is inside the box and gone.
 */
static const char *const PARKED[] = {
    "xxx_dead_anim",
    "disappear_wrestler",
};

/*
 * Programs that contain a frame op yet never show one, following the
 * ANI_CHANGEANIM hand-offs the way the backend does.
 *
 * Exactly one, and it is not an animation. BAMSEQ2.ASM keeps a
 * roll-frame data table beside Bam Bam's animations, and the emitter
 * picks it up because it has the shape of one; nothing ever starts it.
 *
 * The list is asserted EXACTLY: a second is a regression, and this one
 * starting to work is a fix that should come with the line deleted
 * rather than pass silently.
 */
static const char *const FRAMELESS[] = {
    "bam_roll_frames",
};

/*
 * Every ANI_CHANGEANIM operand names a program that exists. No
 * exceptions, so there is no list -- the assertion below is simply that
 * the count is zero.
 *
 * It was 54 operands over five labels. Three were animations the emitter
 * refused because they branch into the head of a later routine
 * (ANIM.ASM:29 makes ANI_IFSTATUS a raw write to the animation PC), and
 * two were animations written as bare column-0 labels rather than SUBRs:
 * DNKSEQ2.ASM's dnk_combo_hammer_anim (:3031) and re_enter_combo_hiptoss
 * (:4429). The roster pass walks SUBR lines, so those two were emitted
 * by nothing at all, and an ANI_CHANGEANIM further down the same file
 * named both. tools/wlpuppet.py's changeanim_op_targets now reads the op
 * itself, the same way its siblings read ANI_SLAVEANIM's and
 * ANI_CHANGEANIM_TBL's tables.
 *
 * This is worth an assertion rather than a count, because an unresolved
 * label is not inert: backend_change_anim_label sets prog = NULL and
 * ended = true, so a wrestler who reaches one stops where he stands.
 */

static int in_list(const char *const *list, size_t n, const char *s)
{
    size_t i;
    for (i = 0; i < n; ++i)
        if (strcmp(list[i], s) == 0) return 1;
    return 0;
}

static int is_branch(uint8_t op)
{
    switch (op) {
        case WM_AOP_GOTO:
        case WM_AOP_IFSTATUS:
        case WM_AOP_IFNOTSTATUS:
        case WM_AOP_IFBLOCKED:
        case WM_AOP_IF_RPTCOUNT:
        case WM_AOP_IFNOT_RPTCOUNT:
        case WM_AOP_IF_RPTCOUNT_GE:
        case WM_AOP_SLIDE_BACK:
        case WM_AOP_IF_BUTCOUNT_GE:
        case WM_AOP_IF_BUTCOUNT_LT:
        case WM_AOP_IFOPPMODE:
        case WM_AOP_IFROPE:
        case WM_AOP_IFNOTROPE:
            return 1;
        default:
            return 0;
    }
}

static void test_the_corpus_is_all_there(void)
{
    size_t n = wm_anim_program_count(), i;

    assert(n >= CORPUS_FLOOR);
    for (i = 0; i < n; ++i) {
        const wm_anim_program *p = wm_anim_program_at(i);
        assert(p != NULL);
        assert(p->source_label != NULL && p->source_label[0] != '\0');
        assert(p->source_file != NULL);
        assert(p->op_count > 0);
        /* A program starting past its own last op would show nothing. */
        assert(p->entry < p->op_count);
        /* Lookup by label has to find THIS program: two programs sharing
           a label would make every ANI_CHANGEANIM to it a coin toss. */
        assert(wm_anim_program_find(p->source_label) == p);
    }
    assert(wm_anim_program_at(n) == NULL);
}

/*
 * The ANI_IFROPE defect, as a standing check.
 *
 * A branch emitted with target -1 reaches the dispatcher as (size_t)-1,
 * fails `pc < p->op_count`, and falls straight out of the loop into
 * `ended = true`. It cost seven animations, every break_neck among them,
 * and it was invisible because the program still loaded, still had ops,
 * and still looked complete. Every destination in the corpus, in range.
 */
static void test_no_branch_walks_off_its_program(void)
{
    size_t n = wm_anim_program_count(), i, j;

    for (i = 0; i < n; ++i) {
        const wm_anim_program *p = wm_anim_program_at(i);
        for (j = 0; j < p->op_count; ++j) {
            const wm_anim_op *o = &p->ops[j];
            if (!is_branch(o->op)) continue;
            assert(o->target >= 0);
            assert((size_t)o->target < p->op_count);
        }
        for (j = 0; j < p->label_count; ++j) {
            assert(p->labels[j].name != NULL);
            assert(p->labels[j].at < p->op_count);
        }
    }
}

/* A frame op with no name draws nothing, and does it quietly. */
static void test_every_frame_op_names_a_frame(void)
{
    size_t n = wm_anim_program_count(), i, j;

    for (i = 0; i < n; ++i) {
        const wm_anim_program *p = wm_anim_program_at(i);
        for (j = 0; j < p->op_count; ++j)
            if (p->ops[j].op == WM_AOP_FRAME)
                assert(p->ops[j].text != NULL && p->ops[j].text[0] != '\0');
    }
}

static void test_every_changeanim_target_resolves(void)
{
    size_t n = wm_anim_program_count(), i, j;
    int dangling = 0;

    for (i = 0; i < n; ++i) {
        const wm_anim_program *p = wm_anim_program_at(i);
        for (j = 0; j < p->op_count; ++j) {
            const wm_anim_op *o = &p->ops[j];
            if (o->op != WM_AOP_CHANGEANIM || !o->text) continue;
            if (wm_anim_program_find(o->text) != NULL) continue;
            printf("%s: ANI_CHANGEANIM to %s, which nothing emits\n",
                   p->source_label, o->text);
            ++dangling;
        }
    }
    assert(dangling == 0);
}

/* One wrestler, standing in the ring on the mat, facing right: the state
   a knockdown animation needs to be able to land. */
static void stage(wm_arcade_actor_t *a)
{
    memset(a, 0, sizeof(*a));
    a->life = 163;
    a->in_ring = 1;
    a->facing_dir = 8;
    a->x_int = WM_RING_X_CENTER - 85;
    a->z_int = 1127 + 93;
    a->x_fixed = (int32_t)a->x_int << 16;
    a->z_fixed = (int32_t)a->z_int << 16;
    a->ground_y = WM_MAT_Y;
    a->y_int = WM_MAT_Y;
}

static void test_playing_the_whole_corpus(void)
{
    size_t n = wm_anim_program_count(), i, k;
    const size_t nframeless = sizeof(FRAMELESS) / sizeof(FRAMELESS[0]);
    int frameless_hit[sizeof(FRAMELESS) / sizeof(FRAMELESS[0])] = { 0 };
    int ending = 0, looping = 0;

    for (i = 0; i < n; ++i) {
        const wm_anim_program *p = wm_anim_program_at(i);
        wm_anim_exec exec;
        wm_arcade_actor_t a;
        int t, has_frame_op = 0, showed = 0, hops = 0;
        size_t j;

        for (j = 0; j < p->op_count; ++j)
            if (p->ops[j].op == WM_AOP_FRAME) { has_frame_op = 1; break; }

        memset(&exec, 0, sizeof(exec));
        stage(&a);
        wm_anim_exec_start(&exec, p, &a, 0, NULL);
        if (wm_anim_exec_frame(&exec)) showed = 1;

        for (t = 0; t < TICKS; ++t) {
            if (exec.ended) {
                /* wrestler_backend.c:196 -- an animation that ends by
                   BECOMING another does not stop. Bounded, because a
                   pair that hand off to each other would not. */
                const wm_anim_program *next;
                if (!exec.become || hops >= MAX_HANDOFFS) break;
                next = wm_anim_program_find(exec.become);
                if (!next) break;      /* the dangling two, asserted above */
                ++hops;
                wm_anim_exec_start(&exec, next, &a, (uint16_t)t, NULL);
                if (wm_anim_exec_frame(&exec)) showed = 1;
                continue;
            }
            wm_wrestler_veladd(&a, &exec, 0);
            wm_wrestler_friction(&a);
            wm_anim_exec_tick(&exec, &a, (uint16_t)t);
            if (wm_anim_exec_frame(&exec)) showed = 1;
        }

        if (exec.ended) ++ending; else ++looping;

        if (!has_frame_op || showed) {
            /* If it showed a frame it is not frameless, and the list had
               better not claim it is. */
            if (has_frame_op && in_list(FRAMELESS, nframeless,
                                        p->source_label)) {
                printf("%s shows a frame now -- drop it from FRAMELESS\n",
                       p->source_label);
                assert(!"FRAMELESS entry is fixed");
            }
            continue;
        }

        if (!in_list(FRAMELESS, nframeless, p->source_label)) {
            printf("%s has frame ops but never showed one (ended=%d)\n",
                   p->source_label, (int)exec.ended);
            assert(!"animation plays no frame");
        }
        for (k = 0; k < nframeless; ++k)
            if (strcmp(FRAMELESS[k], p->source_label) == 0)
                frameless_hit[k] = 1;
    }

    for (k = 0; k < nframeless; ++k) {
        if (!frameless_hit[k])
            printf("%s is no longer in the corpus\n", FRAMELESS[k]);
        assert(frameless_hit[k]);
    }

    printf("  played %zu programs: %d come to rest, %d still running "
           "after %d ticks\n", n, ending, looping, TICKS);
    assert(ending >= ENDING_FLOOR);
}

/*
 * Nothing hangs: every program that is still running has been PROVEN to
 * loop, rather than assumed to.
 *
 * 292 of the 1,532 are still going after 400 ticks. That number on its
 * own says nothing -- a genuine idle stance and a program wedged on a
 * condition that will never come true look identical from outside. So
 * this records every frame each one shows, and requires it to show one
 * twice: a program that revisits a frame it already played is cycling by
 * observation, and will keep cycling forever.
 *
 * 291 of the 292 do. The one that does not is xxx_dead_anim, parked on
 * ANI_ROT by design, named and explained above.
 */
static void test_nothing_hangs(void)
{
    size_t n = wm_anim_program_count(), i;
    const size_t nparked = sizeof(PARKED) / sizeof(PARKED[0]);
    int parked_hit[sizeof(PARKED) / sizeof(PARKED[0])] = { 0 };
    int running = 0, cyclic = 0;
    /* Where a frame has been shown: (program, op index). Bounded by the
       largest program's op count times the hand-off limit. */
    static const wm_anim_program *seen_prog[SEEN_MAX];
    static size_t seen_pc[SEEN_MAX];

    for (i = 0; i < n; ++i) {
        const wm_anim_program *p = wm_anim_program_at(i);
        wm_anim_exec exec;
        wm_arcade_actor_t a;
        int t, hops = 0, cycled = 0;
        size_t nseen = 0;

        memset(&exec, 0, sizeof(exec));
        stage(&a);
        wm_anim_exec_start(&exec, p, &a, 0, NULL);

        for (t = 0; t < TICKS; ++t) {
            if (exec.ended) {
                const wm_anim_program *next;
                if (!exec.become || hops >= MAX_HANDOFFS) break;
                next = wm_anim_program_find(exec.become);
                if (!next) break;
                ++hops;
                wm_anim_exec_start(&exec, next, &a, (uint16_t)t, NULL);
                continue;
            }
            if (!cycled && exec.program && wm_anim_exec_frame(&exec)) {
                size_t k;
                for (k = 0; k < nseen; ++k) {
                    if (seen_prog[k] == exec.program && seen_pc[k] == exec.pc) {
                        cycled = 1;
                        break;
                    }
                }
                if (!cycled && nseen < SEEN_MAX) {
                    seen_prog[nseen] = exec.program;
                    seen_pc[nseen] = exec.pc;
                    ++nseen;
                }
                /* Running out of room would turn a real cycle into a
                   false "hang", so refuse to guess. */
                assert(cycled || nseen < SEEN_MAX);
            }
            wm_wrestler_veladd(&a, &exec, 0);
            wm_wrestler_friction(&a);
            wm_anim_exec_tick(&exec, &a, (uint16_t)t);
        }

        if (exec.ended) continue;
        ++running;
        if (cycled) {
            ++cyclic;
            if (in_list(PARKED, nparked, p->source_label)) {
                printf("%s cycles now -- drop it from PARKED\n",
                       p->source_label);
                assert(!"PARKED entry is fixed");
            }
            continue;
        }
        if (!in_list(PARKED, nparked, p->source_label)) {
            printf("%s never ends and never repeats a frame\n",
                   p->source_label);
            assert(!"animation neither ends nor loops");
        }
        for (size_t k = 0; k < nparked; ++k)
            if (strcmp(PARKED[k], p->source_label) == 0) parked_hit[k] = 1;
    }

    for (size_t k = 0; k < nparked; ++k) {
        if (!parked_hit[k])
            printf("%s no longer parks\n", PARKED[k]);
        assert(parked_hit[k]);
    }

    printf("  %d still running: %d proven to cycle, %d parked by design\n",
           running, cyclic, running - cyclic);
}


int main(void)
{
    test_the_corpus_is_all_there();
    test_no_branch_walks_off_its_program();
    test_every_frame_op_names_a_frame();
    test_every_changeanim_target_resolves();
    test_playing_the_whole_corpus();
    test_nothing_hangs();
    printf("the animation corpus: all checks passed\n");
    return 0;
}
