/*
 * DISPLAY.ASM's object allocator and display lists.
 * See wm/arcade/wm_arcade_objlist.h for what is carried and why.
 */
#include "wm/arcade/wm_arcade_objlist.h"

#include <string.h>

static bool valid(const wm_obj_pool *p, int h) {
    return p && h >= 0 && h < WM_OBJ_COUNT;
}

void wm_obj_pool_init(wm_obj_pool *p) {
    int i;
    if (!p) return;
    memset(p, 0, sizeof *p);
    /*
     * DISPLAY.ASM:1451. `movi OBJSTR,a1 / move a1,@OFREE,L / movi NOBJ,b0`
     * then a loop linking each block to the next. The last block's link is
     * whatever follows it in RAM; the list is bounded by the count, not by a
     * terminator, so here the tail terminates explicitly.
     */
    for (i = 0; i < WM_OBJ_COUNT; ++i) {
        p->next[i] = (int16_t)(i + 1 < WM_OBJ_COUNT ? i + 1 : WM_OBJ_NONE);
    }
    p->free_head = 0;
    p->head[WM_OBJ_LIST_FOREGROUND] = WM_OBJ_NONE;
    p->head[WM_OBJ_LIST_BACKGROUND] = WM_OBJ_NONE;
}

int wm_obj_get(wm_obj_pool *p) {
    int h;
    if (!p || p->free_head == WM_OBJ_NONE) return WM_OBJ_NONE;

    h = p->free_head;
    p->free_head = p->next[h];
    /* `clr a1 / move a1,*a0(OPLINK),L / move a1,*a0(ODATA_p),L` -- the block
       comes out blank apart from its link, which the caller overwrites. */
    memset(&p->obj[h], 0, sizeof p->obj[h]);
    p->next[h] = WM_OBJ_NONE;
    p->in_use[h] = true;
    return h;
}

void wm_obj_free(wm_obj_pool *p, int handle) {
    if (!valid(p, handle)) return;
    p->next[handle] = p->free_head;
    p->free_head = (int16_t)handle;
    p->in_use[handle] = false;
}

void wm_obj_insert(wm_obj_pool *p, int handle, wm_obj_list_id list) {
    int prev = WM_OBJ_NONE, cur;
    int32_t z, y;

    if (!valid(p, handle)) return;
    if (list != WM_OBJ_LIST_FOREGROUND && list != WM_OBJ_LIST_BACKGROUND) return;

    z = p->obj[handle].z;
    y = p->obj[handle].y;

    /*
     * The source's walk, with its comparisons the way the TMS34010 means
     * them: `cmp a3,a1` computes a1 - a3, so `jrgt #lp` continues while the
     * NEW object's key is greater than the current one's.
     */
    cur = p->head[list];
    while (cur != WM_OBJ_NONE) {
        if (z > p->obj[cur].z) {
            prev = cur;
            cur = p->next[cur];
            continue;
        }
        if (z < p->obj[cur].z) break;
        if (y > p->obj[cur].y) {
            prev = cur;
            cur = p->next[cur];
            continue;
        }
        break;  /* equal keys insert BEFORE, as `jrgt` failing does */
    }

    p->next[handle] = (int16_t)cur;
    if (prev == WM_OBJ_NONE) {
        p->head[list] = (int16_t)handle;
    } else {
        p->next[prev] = (int16_t)handle;
    }
}

bool wm_obj_delete(wm_obj_pool *p, int handle) {
    int prev = WM_OBJ_NONE, cur;

    if (!valid(p, handle)) return false;

    cur = p->head[WM_OBJ_LIST_FOREGROUND];
    while (cur != WM_OBJ_NONE && cur != handle) {
        prev = cur;
        cur = p->next[cur];
    }
    if (cur != handle) return false;   /* `delerr` */

    if (prev == WM_OBJ_NONE) {
        p->head[WM_OBJ_LIST_FOREGROUND] = p->next[cur];
    } else {
        p->next[prev] = p->next[cur];
    }
    /* `clr a1 / move a1,*a0(OXPOS)  ;Indicates not in use for collisions` */
    p->obj[handle].x = 0;
    wm_obj_free(p, handle);
    return true;
}

int wm_obj_delete_class(wm_obj_pool *p, int32_t id, int32_t mask) {
    int prev = WM_OBJ_NONE, cur, n = 0;
    int32_t want;

    if (!p) return 0;

    /* `sext a0 / andn a1,a0  ;Form match` -- andn is "and not", so the mask
       names the bits to IGNORE. */
    want = id & ~mask;

    cur = p->head[WM_OBJ_LIST_FOREGROUND];
    while (cur != WM_OBJ_NONE) {
        if ((p->obj[cur].id & ~mask) == want) {
            int dead = cur;
            cur = p->next[cur];
            if (prev == WM_OBJ_NONE) {
                p->head[WM_OBJ_LIST_FOREGROUND] = (int16_t)cur;
            } else {
                p->next[prev] = (int16_t)cur;
            }
            /*
             * The source loops back to #cmp rather than #lp here, so prev is
             * NOT advanced past a deleted block -- consecutive matches are all
             * removed. Leaving prev alone does the same.
             */
            wm_obj_free(p, dead);
            ++n;
            continue;
        }
        prev = cur;
        cur = p->next[cur];
    }
    return n;
}

int wm_obj_first(const wm_obj_pool *p, wm_obj_list_id list) {
    if (!p) return WM_OBJ_NONE;
    if (list != WM_OBJ_LIST_FOREGROUND && list != WM_OBJ_LIST_BACKGROUND) {
        return WM_OBJ_NONE;
    }
    return p->head[list];
}

int wm_obj_next(const wm_obj_pool *p, int handle) {
    if (!valid(p, handle)) return WM_OBJ_NONE;
    return p->next[handle];
}

int wm_obj_list_count(const wm_obj_pool *p, wm_obj_list_id list) {
    int n = 0, cur = wm_obj_first(p, list);
    while (cur != WM_OBJ_NONE) {
        ++n;
        cur = p->next[cur];
    }
    return n;
}

int wm_obj_free_count(const wm_obj_pool *p) {
    int n = 0, cur;
    if (!p) return 0;
    for (cur = p->free_head; cur != WM_OBJ_NONE; cur = p->next[cur]) ++n;
    return n;
}
