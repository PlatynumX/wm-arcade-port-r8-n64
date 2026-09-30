#ifndef WM_ARCADE_OBJLIST_H
#define WM_ARCADE_OBJLIST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * DISPLAY.ASM's object list: the allocator and the two display lists every
 * drawn thing in the arcade game lives on.
 *
 * WHY THIS IS NOT wm_match_build_draw_list. The match rebuilds its draw list
 * every frame from the actors, which is right for wrestlers -- they exist
 * continuously and their positions are recomputed anyway. The attract screens
 * do not work that way: show_copyright CREATES nine text objects and a logo,
 * leaves them on screen through `wait_on_butn 3*TSEC`, then deletes a whole
 * ID class with obj_del1c and builds the second page. The objects' lifetime IS
 * the screen's behaviour, so it has to be modelled, not rebuilt.
 *
 * The pool is DISPLAY.ASM:1451's: `movi NOBJ,b0` over OBSIZ-sized blocks
 * linked head to tail, with OFREE pointing at the first. NOBJ is 350
 * (DISPLAY.EQU:96) and that number is the real budget -- GETOBJ's failure path
 * is CALLERR, not a resize.
 *
 * Handles are indices, not pointers. The source threads its free list through
 * each block's first long and its display lists through the same field; an
 * index does the same job without pretending a C array is TMS34010 RAM.
 */

/* DISPLAY.EQU:96 `NOBJ equ 350  ;Total # objects`. */
#define WM_OBJ_COUNT 350
/* GETOBJ returns zero on failure, and the source tests it with `move a0,a0`.
   Index 0 is a real slot here, so failure needs its own value. */
#define WM_OBJ_NONE (-1)

/*
 * Which list an object is on. The source keeps two, inserted by different
 * entry points into the same body: INSOBJ threads OBJLST and INSBOBJ threads
 * BAKLST (DISPLAY.ASM:1540 and :1549, `movi BAKLST,a14 / jruc #strt`).
 */
typedef enum {
    WM_OBJ_LIST_FOREGROUND = 0,  /* OBJLST */
    WM_OBJ_LIST_BACKGROUND = 1   /* BAKLST */
} wm_obj_list_id;

/*
 * The fields this translation carries.
 *
 * Not all of OBSIZ: the source's block holds velocities, scale, SAG pointers
 * and DMA control words that belong to hardware this port does not have. What
 * is here is what the list's own behaviour reads -- the sort keys, the
 * identity mask delete works on, and the palette word pal_clean sweeps -- plus
 * the position and image a renderer needs. A field is added when something
 * reads it, not in case.
 */
typedef struct {
    int32_t z;        /* OZPOS -- the primary sort key */
    int32_t y;        /* OYPOS -- the tiebreak within equal z */
    int32_t x;        /* OXPOS. DELOBJ clears it: "Indicates not in use for
                         collisions" (DISPLAY.ASM:1610). */
    int32_t id;       /* OID -- CLSNEUT|TYPTEXT|SUBTXT and friends */
    int32_t palette;  /* OPAL -- what pal_clean matches when sweeping */
    const void *image; /* OIMG */
} wm_obj;

typedef struct {
    wm_obj obj[WM_OBJ_COUNT];
    /* next[i] threads whichever list i is on, or the free list. */
    int16_t next[WM_OBJ_COUNT];
    int16_t free_head;
    int16_t head[2];
    bool in_use[WM_OBJ_COUNT];
} wm_obj_pool;

/* DISPLAY.ASM:1451's free-list build: every block linked head to tail. */
void wm_obj_pool_init(wm_obj_pool *p);

/*
 * GETOBJ (DISPLAY.ASM:1470): pop the free list.
 *
 * Returns a handle, or WM_OBJ_NONE when the pool is empty -- the source's
 * `nonelft` path, which raises CALLERR 3 and returns zero. The object is NOT
 * on a display list yet; the source's callers fill it and then call INSOBJ.
 */
int wm_obj_get(wm_obj_pool *p);

/* FREEOBJ (DISPLAY.ASM:1497): push back onto the free list, no unlinking. */
void wm_obj_free(wm_obj_pool *p, int handle);

/*
 * INSOBJ / INSBOBJ (DISPLAY.ASM:1549 / :1540): insert, keeping the list
 * sorted by increasing z and, within equal z, increasing y.
 *
 * The walk advances while the new object's z is GREATER than the current
 * one's, and on equal z while its y is greater. So an object whose keys equal
 * an existing one's is inserted BEFORE it, not after -- `jrgt #lp` fails on
 * equality and falls into the insert. That is the opposite of a stable
 * append, and it is what the source does.
 */
void wm_obj_insert(wm_obj_pool *p, int handle, wm_obj_list_id list);

/*
 * DELOBJ (DISPLAY.ASM:1601): unlink from the foreground list, clear OXPOS,
 * and return the block to the free list. Returns false when the handle is not
 * on the list -- the source's `delerr`, CALLERR 1.
 */
bool wm_obj_delete(wm_obj_pool *p, int handle);

/*
 * obj_delc (DISPLAY.ASM:1689): delete every foreground object whose OID,
 * masked, matches. obj_del1c is this with mask 0 (DISPLAY.ASM:1682, `clr a1`
 * then fall through).
 *
 * `mask` is the source's A1, the bits to REMOVE -- it is applied with `andn`
 * to both the wanted id and each candidate's. Returns how many were deleted.
 */
int wm_obj_delete_class(wm_obj_pool *p, int32_t id, int32_t mask);

/* Walk a list in draw order. Returns WM_OBJ_NONE at the end. */
int wm_obj_first(const wm_obj_pool *p, wm_obj_list_id list);
int wm_obj_next(const wm_obj_pool *p, int handle);

/* How many objects are on a list, and how many remain free. */
int wm_obj_list_count(const wm_obj_pool *p, wm_obj_list_id list);
int wm_obj_free_count(const wm_obj_pool *p);

#ifdef __cplusplus
}
#endif
#endif
