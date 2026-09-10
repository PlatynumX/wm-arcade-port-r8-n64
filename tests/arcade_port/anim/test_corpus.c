/*
 * Every animation program in the game, played.
 *
 * The port emits 1,525 animation programs out of the wrestlers' sequence
 * files, and until now no test touched more than the dozen or so it
 * thought to name by hand. That is how three real defects shipped:
 *
 *   ANI_REPEAT was executed as ANI_END, so 169 programs died after one
 *   pass; ANI_IFROPE and ANI_IFNOTROPE were emitted with no branch
 *   destination at all, so 75 branches walked off the end of their
 *   program and killed the animation before its first frame; and
 *   ANI_CHANGEANIM still names five labels nothing emits.
 *
 * None of the three needed a subtle test to catch. Each one shows up the
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
#define CORPUS_FLOOR 1525u

/* How long to play each program. Long enough that everything with an end
   reaches it -- the longest terminating program in the corpus finishes
   well inside this -- and short enough that 1,525 of them stay quick. */
#define TICKS 400

/* Programs that terminate rather than loop, as measured. A program that
   used to end and now runs forever is a hang in the making, so this may
   grow but must not shrink. */
#define ENDING_FLOOR 1278

/*
 * Programs that contain a frame op yet never show one.
 *
 * All four are explained, and the list is asserted EXACTLY: a fifth is a
 * regression, and one of these four starting to work is a fix that should
 * come with the line below deleted rather than pass silently.
 *
 *   bam_roll_frames         is not an animation. It is the roll-frame
 *                           data table BAMSEQ2.ASM keeps beside them, and
 *                           the emitter picks it up because it looks like
 *                           one. Harmless; it is never started.
 *
 *   hrt_neckbroken_anim     hands off with ANI_CHANGEANIM before reaching
 *   xxx_get_face_rake_anim  a frame of its own -- the frames further down
 *   xxx_get_face_rake2_anim their bodies belong to the label they jump
 *                           to, and get played there.
 */
static const char *const FRAMELESS[] = {
    "bam_roll_frames",
    "hrt_neckbroken_anim",
    "xxx_get_face_rake_anim",
    "xxx_get_face_rake2_anim",
};

/*
 * ANI_CHANGEANIM targets that no emitted program answers to, and why.
 *
 * This is a live defect, not a curiosity: backend_change_anim_label sets
 * prog = NULL and ended = true when the label does not resolve, so a
 * wrestler who reaches one of these simply stops. Two of the five are
 * dangling in the original as well; three exist in the source and are
 * refused by the emitter:
 *
 *   bam_faceup_getup_anim   branches to bam_4_faceup_getup_anim
 *   dnk_faceup_getup_anim   branches to dnk_2_faceup_getup_anim
 *   yok_combo_scissor_anim  branches to yok_overhd_slam_anim
 *
 * all three via ANI_IFSTATUS, which ANIM.ASM:29 implements as a raw write
 * to the animation PC (`move a0,a4`) -- a jump into another routine's
 * instruction stream, keeping ANIBASE where it was. The emitter resolves
 * branch targets to op indices inside one program and has no way to spell
 * that, so it refuses the whole routine rather than emit a branch it
 * cannot honour. The refusal is the honest answer; the gap is real.
 *
 *   dnk_combo_hammer_anim   named by ANI_CHANGEANIM, defined nowhere in
 *   re_enter_combo_hiptoss  the linked source. Dangling in the arcade
 *                           build too.
 *
 * The first two matter most: they are how a wrestler knocked onto his
 * back gets up.
 */
static const char *const UNRESOLVED_CHANGEANIM[] = {
    "bam_faceup_getup_anim",
    "dnk_combo_hammer_anim",
    "dnk_faceup_getup_anim",
    "re_enter_combo_hiptoss",
    "yok_combo_scissor_anim",
};

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
    size_t n = wm_anim_program_count(), i, j, k;
    const size_t expected = sizeof(UNRESOLVED_CHANGEANIM)
                          / sizeof(UNRESOLVED_CHANGEANIM[0]);
    int hit[sizeof(UNRESOLVED_CHANGEANIM)
            / sizeof(UNRESOLVED_CHANGEANIM[0])] = { 0 };

    for (i = 0; i < n; ++i) {
        const wm_anim_program *p = wm_anim_program_at(i);
        for (j = 0; j < p->op_count; ++j) {
            const wm_anim_op *o = &p->ops[j];
            if (o->op != WM_AOP_CHANGEANIM || !o->text) continue;
            if (wm_anim_program_find(o->text) != NULL) continue;
            /* Unresolved. It had better be one of the five known ones. */
            if (!in_list(UNRESOLVED_CHANGEANIM, expected, o->text)) {
                printf("%s: ANI_CHANGEANIM to %s, which nothing emits\n",
                       p->source_label, o->text);
                assert(!"new dangling ANI_CHANGEANIM target");
            }
            for (k = 0; k < expected; ++k)
                if (strcmp(UNRESOLVED_CHANGEANIM[k], o->text) == 0) hit[k] = 1;
        }
    }
    /* And each of the five is still dangling. When one is fixed, this
       fires and the entry comes out of the list above. */
    for (k = 0; k < expected; ++k) {
        if (!hit[k])
            printf("%s resolves now -- drop it from UNRESOLVED_CHANGEANIM\n",
                   UNRESOLVED_CHANGEANIM[k]);
        assert(hit[k]);
    }
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
        int t, has_frame_op = 0, showed = 0;
        size_t j;

        for (j = 0; j < p->op_count; ++j)
            if (p->ops[j].op == WM_AOP_FRAME) { has_frame_op = 1; break; }

        memset(&exec, 0, sizeof(exec));
        stage(&a);
        wm_anim_exec_start(&exec, p, &a, 0, NULL);
        if (wm_anim_exec_frame(&exec)) showed = 1;

        for (t = 0; t < TICKS && !exec.ended; ++t) {
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

    printf("  played %zu programs: %d end, %d still running after %d ticks\n",
           n, ending, looping, TICKS);
    assert(ending >= ENDING_FLOOR);
}

int main(void)
{
    test_the_corpus_is_all_there();
    test_no_branch_walks_off_its_program();
    test_every_frame_op_names_a_frame();
    test_every_changeanim_target_resolves();
    test_playing_the_whole_corpus();
    printf("the animation corpus: all checks passed\n");
    return 0;
}
