#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "wm/character_assets.h"
#include "wm_arcade_wimp_frame.h"

static void check_roster(uint8_t id)
{
    const wm_source_sprite *sprite = wm_character_base_sprite(id);
    int16_t tail[WM_WIMP_TAIL_WORDS];
    wm_arcade_frame_box_t box;

    assert(sprite != NULL);
    assert(sprite->source_frame != NULL);

    /*
     * R37N3 collision contract:
     * COLLIS.ASM reads CUR_FRAME's IANI3 words directly.  The metadata-only
     * source lookup must therefore reproduce the physical WIMP directory tail
     * independently of renderer pixel/DragonFS loading.
     */
    assert(wm_character_wimp_tail_find(id, sprite->source_frame, tail));
    assert(memcmp(tail, sprite->wimp_tail, sizeof(tail)) == 0);

    /*
     * This is the collision-facing adapter.  It must copy the source words
     * exactly and must NOT apply the renderer-facing plausibility filter.
     */
    assert(wm_arcade_wimp_frame_box_from_tail(tail, &box));
    assert(box.iani3x == tail[WM_WIMP_IANI3_X_SLOT]);
    assert(box.iani3y == tail[WM_WIMP_IANI3_Y_SLOT]);
    assert(box.iani3z == tail[WM_WIMP_IANI3_Z_SLOT]);
    assert(box.iani3id == tail[WM_WIMP_IANI3_ID_SLOT]);

    /*
     * Deliberately do not call wm_arcade_wimp_frame_box_from_sprite() here.
     * That adapter belongs to renderer/asset safety and is covered by the R36
     * renderer-equivalence regression, where sentinel tails must fail closed.
     */
}

int main(void)
{
    static const uint8_t ids[] = {0,1,2,3,4,5,6,8};
    int16_t exact[WM_WIMP_TAIL_WORDS] = {0};
    wm_arcade_frame_box_t f;
    size_t i;

    for (i = 0u; i < sizeof(ids)/sizeof(ids[0]); ++i)
        check_roster(ids[i]);

    /*
     * COLLIS.ASM imposes no port-side plausibility window on IANI3.  Prove
     * that the collision adapter preserves signed source words exactly,
     * including values the renderer safety adapter is allowed to reject.
     */
    exact[WM_WIMP_IANI3_X_SLOT] = -600;
    exact[WM_WIMP_IANI3_Y_SLOT] = 513;
    exact[WM_WIMP_IANI3_Z_SLOT] = 0;
    exact[WM_WIMP_IANI3_ID_SLOT] = -1;

    assert(wm_arcade_wimp_frame_box_from_tail(exact, &f));
    assert(f.iani3x == -600);
    assert(f.iani3y == 513);
    assert(f.iani3z == 0);
    assert(f.iani3id == -1);

    assert(!wm_arcade_wimp_frame_box_from_tail(NULL, &f));
    assert(!wm_arcade_wimp_frame_box_from_tail(exact, NULL));

    puts("R37N3 source CUR_FRAME/WIMP metadata regression: PASS");
    return 0;
}
