#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "wm_arcade_wrestler_process.h"

int main(void)
{
    wm_arcade_actor_t a, b;
    wm_arcade_wrestler_process_t pa, pb;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    a.active = b.active = 1;

    wm_arcade_wrestler_process_init(&pa, &a);
    wm_arcade_wrestler_process_init(&pb, &b);
    assert(a.ptime == 1);
    assert(wm_arcade_wrestler_process_resume(&pa) ==
           WM_WRESTLER_RESUME_CALC_CLOSEST);
    assert(wm_arcade_wrestler_process_dispatch_ready(&a));
    assert((uint16_t)a.ptime == 0u);

    wm_arcade_wrestler_process_sleep(&pa, &a);
    assert(a.ptime == 1);
    assert(wm_arcade_wrestler_process_resume(&pa) ==
           WM_WRESTLER_RESUME_POST_SLEEP);

    assert(wm_arcade_wrestler_process_dispatch_ready(&a));
    wm_arcade_wrestler_process_sleep(&pa, &a);

    a.ptime = 0;
    assert(wm_arcade_wrestler_process_dispatch_ready(&a));
    assert((uint16_t)a.ptime == 0xffffu);

    a.status_flags = WM_STATUS_KOD;
    wm_arcade_wrestler_process_sleep(&pa, &a);
    assert(a.ptime == 0x7fff);
    assert(!wm_arcade_wrestler_process_dispatch_ready(&a));
    wm_arcade_wrestler_process_wake(&a);
    assert(a.ptime == 1);
    assert(wm_arcade_wrestler_process_dispatch_ready(&a));

    b.status_flags = WM_STATUS_KOD;
    wm_arcade_wrestler_process_sleep(&pb, &b);
    assert(!wm_arcade_wrestler_process_dispatch_ready(&b));
    wm_arcade_wrestler_process_wake(&b);
    assert(wm_arcade_wrestler_process_dispatch_ready(&b));
    return 0;
}
