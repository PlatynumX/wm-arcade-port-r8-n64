/*
 * AWARD.ASM:3056 decompress_string and the text it unpacks: the two
 * Mortal Kombat 3 tips and the eight wrestlers' ending stories.
 */
#include <assert.h>
#include <string.h>

#include "wm_arcade_story.h"

/*
 * The routine itself, against a blob copied out of BRETST.H with the
 * answer the source wrote beside it.
 */
static void test_decompress(void) {
    /* `; Compressed string "THROUGH THE EXCELLENCE OF"` */
    static const uint8_t packed[] = {
        0x75, 0x3a, 0xc3, 0x36, 0x9a, 0x06, 0x75, 0x6a,
        0x06, 0x66, 0x4e, 0x9a, 0x6d, 0x6b, 0xbe, 0xa4,
        0x19, 0xc0, 0x27, 0x00
    };
    char out[64];
    size_t n = wm_story_decompress(packed, sizeof packed, out, sizeof out);
    assert(n == strlen("THROUGH THE EXCELLENCE OF"));
    assert(strcmp(out, "THROUGH THE EXCELLENCE OF") == 0);

    /* Four characters to every three bytes -- 25 characters out of
       twenty bytes, terminator included. */
    assert(n * 6u <= sizeof(packed) * 8u);

    /* A zero field ends it even mid-byte. */
    {
        static const uint8_t only_a[] = { 0x22, 0x00 };
        char buf[8];
        assert(wm_story_decompress(only_a, sizeof only_a, buf, sizeof buf)
               == 1);
        assert(buf[0] == 'A');   /* 0x22 + 0x1F */
    }

    /* The alphabet is 0x20-0x5F and nothing else: 6 bits + 0x1F. */
    {
        static const uint8_t space[] = { 0x01, 0x00 };
        char buf[8];
        assert(wm_story_decompress(space, sizeof space, buf, sizeof buf) == 1);
        assert(buf[0] == ' ');
    }

    /* A cap it cannot fit in is reported, not truncated into a lie.
       The source writes into a fixed message_buffer and does not
       bound itself at all. */
    {
        char tiny[4];
        assert(wm_story_decompress(packed, sizeof packed, tiny, sizeof tiny)
               == (size_t)-1);
    }
    assert(wm_story_decompress(packed, sizeof packed, out, 0) == (size_t)-1);
}

/* STORIES.ASM:73 #wrestler_stories_table. */
static void test_stories(void) {
    size_t slot, total = 0;

    for (slot = 0; slot < WM_STORY_SLOTS; ++slot) {
        const wm_story_set_t *set = &wm_wrestler_stories[slot];
        size_t i;
        assert(set->name);
        assert(set->story_count > 0);
        for (i = 0; i < set->story_count; ++i) {
            const wm_story_t *st = &set->stories[i];
            size_t k;
            assert(st->line_count > 0);
            for (k = 0; k < st->line_count; ++k) {
                const char *line = st->lines[k];
                size_t c;
                assert(line);
                /* Every character came out of a six-bit field plus
                   0x1F, so the whole corpus is inside 0x20-0x5F. */
                for (c = 0; line[c]; ++c)
                    assert(line[c] >= 0x20 && line[c] <= 0x5f);
            }
            total += st->line_count;
        }
    }
    assert(total > 200);

    /* Slot 7 is Adam Bomb, and the source hands him Doink's stories
       -- the same hand-me-down the animation dispatch tables make.
       The two rows name the same table and share its lines. */
    assert(strcmp(wm_wrestler_stories[6].name,
                  wm_wrestler_stories[7].name) == 0);
    assert(wm_wrestler_stories[6].stories == wm_wrestler_stories[7].stories);

    /* And nobody else does. */
    {
        size_t a, b, shared = 0;
        for (a = 0; a < WM_STORY_SLOTS; ++a)
            for (b = a + 1; b < WM_STORY_SLOTS; ++b)
                if (wm_wrestler_stories[a].stories ==
                    wm_wrestler_stories[b].stories)
                    ++shared;
        assert(shared == 1);
    }
}

/* A line of the Undertaker's, read against the source by hand. */
static void test_a_story_reads(void) {
    const wm_story_set_t *und = &wm_wrestler_stories[2];
    assert(strcmp(und->name, "taker_stories") == 0);
    assert(und->story_count == 1);
    assert(strcmp(und->stories[0].lines[0],
                  "AFTER THE DUST SETTLED AND THE") == 0);
    /* Death Valley, where he may rest in peace. */
    {
        const wm_story_t *st = &und->stories[0];
        const char *last = st->lines[st->line_count - 1];
        assert(strstr(last, "PEACE") != NULL);
    }
}

/* The two MK3 tips and the codes they tell you to enter. */
static void test_mk3_tips(void) {
    assert(strcmp(wm_mk3_tip_lines[0][0],
                  "FOR TWO PLAYER GAMES, USE THE") == 0);
    assert(strcmp(wm_mk3_tip_lines[0][2], "SPECIAL FEATURE.") == 0);
    assert(strcmp(wm_mk3_tip_lines[1][0],
                  "IN TWO PLAYER GAMES, ENTER") == 0);
    assert(strcmp(wm_mk3_tip_lines[1][2], "HIDDEN FEATURE.") == 0);

    /* Six icons each, in the order the object records lay them out
       across the screen twenty-four pixels apart. */
    assert(strcmp(wm_mk3_code_icons[0][0], "I_GORO") == 0);
    assert(strcmp(wm_mk3_code_icons[0][1], "I_SHOKAHN") == 0);
    assert(strcmp(wm_mk3_code_icons[0][5], "I_YINYANG") == 0);
    assert(strcmp(wm_mk3_code_icons[1][0], "I_QUESTION") == 0);
    assert(strcmp(wm_mk3_code_icons[1][5], "I_GORO") == 0);
    /* The two codes really are different. */
    assert(strcmp(wm_mk3_code_icons[0][0], wm_mk3_code_icons[1][0]) != 0);
}

int main(void) {
    test_decompress();
    test_stories();
    test_a_story_reads();
    test_mk3_tips();
    return 0;
}
