/*
 * AWARD.ASM:3056 decompress_string -- see wm/arcade/wm_arcade_story.h.
 */
#include "wm/arcade/wm_arcade_story.h"

size_t wm_story_decompress(const uint8_t *packed, size_t packed_bytes,
                           char *out, size_t cap) {
    uint32_t bits = 0;
    unsigned have = 0;
    size_t i, n = 0;

    if (!out || cap == 0) return (size_t)-1;
    if (!packed) { out[0] = '\0'; return 0; }

    for (i = 0; i < packed_bytes; ++i) {
        bits |= (uint32_t)packed[i] << have;
        have += 8;
        /*
         * `setf 6,0,0 / move *a0+,a2` -- six bits at a time out of a
         * little-endian stream, least significant first.
         */
        while (have >= 6) {
            unsigned v = bits & 0x3fu;
            bits >>= 6;
            have -= 6;
            /* `jrz #decompress_done`, then `clr a0 / movb a0,*a1`. */
            if (v == 0) { out[n] = '\0'; return n; }
            if (n + 1 >= cap) return (size_t)-1;
            /* `addi 01fh,a2 / movb a2,*a1`. */
            out[n++] = (char)(v + 0x1f);
        }
    }
    /* The blob ran out with no zero field. The source would keep
       reading whatever followed it in memory; this stops. */
    out[n] = '\0';
    return n;
}
