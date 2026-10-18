/*
 * HSTD.ASM's two string routines: strip_white (:826) and
 * val_to_dec_tenths_asc (:1105).
 */
#include "wm/arcade/wm_arcade_string.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void check(const char *in, const char *want)
{
    char out[32];
    size_t n = wm_string_strip_white(in, strlen(in), out, sizeof(out));
    assert(n == strlen(want));
    assert(memcmp(out, want, n) == 0);
}

static void test_strip_white(void)
{
    check("ABC", "ABC");
    check("   ABC", "ABC");
    check("ABC   ", "ABC");
    check("  ABC  ", "ABC");
    check("  A B  ", "A B");      /* only the ends */

    /*
     * The empty case does NOT give an empty string. `cmp a0,a1 / jrgt
     * #copy_loop` fails when nothing is left, and both ends are reset
     * to the whole buffer -- so all-spaces in gives all-spaces out.
     */
    check("    ", "    ");

    /* One character, and one character that is a space. */
    check("A", "A");
    check(" ", " ");

    /* A NUL at the end is walked past like a space -- `move a2,a2 /
       jrz #get_prev` -- so a buffer padded with them trims. */
    {
        static const char padded[6] = { 'H', 'I', 0, 0, 0, 0 };
        char out[8];
        size_t n = wm_string_strip_white(padded, sizeof(padded),
                                         out, sizeof(out));
        assert(n == 2 && out[0] == 'H' && out[1] == 'I');
    }

    /* It writes no terminator and respects the caller's cap. */
    {
        char out[2];
        size_t n = wm_string_strip_white("ABCDE", 5, out, sizeof(out));
        assert(n == 2 && out[0] == 'A' && out[1] == 'B');
    }

    assert(wm_string_strip_white(NULL, 3, (char[4]){0}, 4) == 0);
    assert(wm_string_strip_white("AB", 2, NULL, 4) == 0);
    assert(wm_string_strip_white("AB", 0, (char[4]){0}, 4) == 0);
}

static void test_val_to_dec_tenths(void)
{
    wm_string_state st;

    /* The value is hundredths and prints as tenths. */
    wm_string_init(&st);
    wm_string_val_to_dec_tenths(&st, 1234u);
    assert(strcmp(st.buffer, "12.3") == 0);

    wm_string_init(&st);
    wm_string_val_to_dec_tenths(&st, 250u);
    assert(strcmp(st.buffer, "2.5") == 0);

    /*
     * A zero tenths digit takes the other branch entirely: dec_to_asc
     * suppresses leading zeros and would write nothing, so the source
     * concatenates the ROM string "0" by hand.
     */
    wm_string_init(&st);
    wm_string_val_to_dec_tenths(&st, 300u);
    assert(strcmp(st.buffer, "3.0") == 0);

    wm_string_init(&st);
    wm_string_val_to_dec_tenths(&st, 0u);
    assert(strcmp(st.buffer, "0.0") == 0);

    /* The hundredths digit is dropped rather than rounded. */
    wm_string_init(&st);
    wm_string_val_to_dec_tenths(&st, 1299u);
    assert(strcmp(st.buffer, "12.9") == 0);

    wm_string_val_to_dec_tenths(NULL, 100u);
}

int main(void)
{
    test_strip_white();
    test_val_to_dec_tenths();
    printf("HSTD strip_white and val_to_dec_tenths_asc: all checks passed\n");
    return 0;
}
