#include <assert.h>
#include <string.h>
#include "wm_arcade_wrestler_process.h"

int main(void)
{
    wm_arcade_actor_t a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    a.active = b.active = 1;

    a.ptime = 1;
    assert(wm_arcade_wrestler_process_dispatch_ready(&a));
    assert(a.ptime == 0);

    a.ptime = 2;
    assert(!wm_arcade_wrestler_process_dispatch_ready(&a));
    assert(a.ptime == 1);
    assert(wm_arcade_wrestler_process_dispatch_ready(&a));
    assert(a.ptime == 0);

    /* MPROC stores PTIME as a word; zero decrements to ffff but still wakes
       because the GSP branch sees the signed -1 result. */
    a.ptime = 0;
    assert(wm_arcade_wrestler_process_dispatch_ready(&a));
    assert((uint16_t)a.ptime == 0xffffu);

    a.status_flags = 0;
    wm_arcade_wrestler_process_sleep_loop(&a);
    assert(a.ptime == 1);

    a.status_flags = WM_STATUS_KOD;
    wm_arcade_wrestler_process_sleep_loop(&a);
    assert(a.ptime == 0x7fff);
    assert(!wm_arcade_wrestler_process_dispatch_ready(&a));
    assert(a.ptime == 0x7ffe);

    /* Linked-list ordering: an earlier wrestler may wake a later wrestler and
       the later one is runnable when the dispatcher reaches it in THIS pass. */
    a.status_flags = 0;
    a.ptime = 1;
    b.ptime = 0x7fff;
    assert(wm_arcade_wrestler_process_dispatch_ready(&a));
    wm_arcade_wrestler_process_wake(&b);
    assert(b.ptime == 1);
    assert(wm_arcade_wrestler_process_dispatch_ready(&b));
    assert(b.ptime == 0);

    /* Reverse direction: later process wakes an already-visited process after
       that process has slept. The wake must survive until next dispatch. */
    wm_arcade_wrestler_process_sleep_loop(&a);
    assert(a.ptime == 1);
    wm_arcade_wrestler_process_wake(&a);
    assert(a.ptime == 1);

    return 0;
}
