#ifndef WM_ARCADE_OBJUTIL_H
#define WM_ARCADE_OBJUTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The small shared routines UTIL.ASM, DISPLAY.ASM, MENU.ASM and
 * AUDIT.ASM export, which a pass over the file's "this is all
 * hardware" group turned out to be nothing of the kind.
 *
 * Fifty-six routines in those files plus MAIN, DIAG, MPROC, TEST and
 * SCREEN were sitting open on the assumption that they all talk to
 * the coin door, the PIC or the DMA. Most of them do. These six do
 * not: they are object geometry, a list search, a bar height and a
 * checksum, and every one of them is arithmetic that can be
 * translated and tested exactly.
 *
 * The TMS34010's packed XY format matters throughout: a single
 * 32-bit register holds Y in the high word and X in the low, and
 * `addxy` adds the two halves independently without carry between
 * them. DISPLAY.EQU puts OSIZEX at 130h and OSIZEY at 140h, and
 * OXPOS/OYPOS at 090h/0b0h, so a long read at OSIZE or a `movx` into
 * a Y register produces exactly that layout.
 */

/* ---- packed XY --------------------------------------------------- */

/* Y in the high word, X in the low. */
static inline int32_t wm_xy_pack(int16_t x, int16_t y) {
    return (int32_t)(((uint32_t)(uint16_t)y << 16) | (uint16_t)x);
}
static inline int16_t wm_xy_x(int32_t xy) {
    return (int16_t)(uint16_t)((uint32_t)xy & 0xffffu);
}
static inline int16_t wm_xy_y(int32_t xy) {
    return (int16_t)(uint16_t)(((uint32_t)xy >> 16) & 0xffffu);
}
/* `addxy` -- the two halves add independently, no carry across. */
int32_t wm_xy_add(int32_t a, int32_t b);

/* ---- GETCPNT (UTIL.ASM:1634) ------------------------------------- */

/*
 * "Get the center xy position of an object", six instructions:
 *
 *     move *a8(OYVAL),a1,L    ; Y integer:fraction
 *     move *a8(OXPOS),a0      ; X integer
 *     movx a0,a1              ; a1 = Y:X
 *     move *a8(OSIZE),a0,L    ; a0 = ysize:xsize
 *     srl  1,a0               ; halve BOTH, as one 32-bit shift
 *     andi >7fff7fff,a0       ; and clear what bled across
 *     addxy a1,a0
 *
 * The `srl 1` is the interesting one: it halves the packed pair with
 * a single shift of the whole register, which drops the Y size's
 * bottom bit into the TOP bit of the X size. `andi >7fff7fff` clears
 * that bit, so an odd height does not silently add 32768 to the
 * centre X. This reproduces the masking rather than halving the two
 * fields separately, because the two are not the same for an odd
 * height. The mask's other half, `7fff` in the high word, clears bit
 * 31 -- which the logical shift has already cleared, so that half is
 * dead and is kept only because the source writes it.
 */
typedef struct {
    int16_t xpos;      /* OXPOS, integer X */
    int16_t ypos;      /* OYPOS, integer Y (the high word of OYVAL) */
    uint16_t sizex;    /* OSIZEX */
    uint16_t sizey;    /* OSIZEY */
} wm_obj_geom_t;

/* Returns the centre as a packed XY. */
int32_t wm_obj_getcpnt(const wm_obj_geom_t *o);

/* ---- obj_find (UTIL.ASM:1016) ------------------------------------ */

/*
 * "Find an object by OID". A linked walk with a masked compare:
 *
 *     andn a2,a1              ; wanted &= ~mask
 *   of20
 *     move *a0(OID),a3
 *     andn a2,a3              ; candidate &= ~mask
 *     cmp  a1,a3
 *     jreq of50
 *     move *a0,a0,L           ; next, the link is at offset 0
 *     jrnz of20
 *   of50
 *     move a0,a0              ; sets Z if nothing matched
 *
 * a2 is documented "!Mask" and used as `andn` -- the bits to REMOVE
 * before comparing, not the bits to keep. Getting that backwards
 * would find the wrong object every time, so it is worth stating:
 * pass the bits you want IGNORED.
 */
typedef struct {
    uint16_t oid;
    /* The caller's own payload; obj_find only returns the node. */
    void *payload;
} wm_obj_node_t;

/*
 * The source walks a linked list whose link is the first field; this
 * takes the array the port keeps instead, and `count` is how many
 * entries are live. Returns the index of the first match, or -1 --
 * the source's `move a0,a0 / rets` Z flag.
 */
int wm_obj_find(const wm_obj_node_t *objs, size_t count,
                uint16_t want, uint16_t ignore_mask);

/* ---- obj_addworldxy (DISPLAY.ASM:1786) --------------------------- */

/*
 * Turn an object's screen-relative position into a world one by
 * adding the camera. Both axes, both 16.16, in place. Four lines of
 * assembly and the whole of what it does -- but it is what every
 * routine that hands an object between the two coordinate spaces
 * calls, so it is here rather than open.
 */
void wm_obj_addworldxy(int32_t *oxval, int32_t *oyval,
                       int32_t worldtlx, int32_t worldtly);

/* ---- anipt_getxy (DISPLAY.ASM:2219) ------------------------------ */

/*
 * "Get an objects anipt XY (Doesn't check VFLIP)" -- the animation
 * attach point, in 16.16, with the horizontal flip applied:
 *
 *     x = IANIOFFX << 16
 *     if OCTRL has B_FLIPH:  x = ((ISIZEX - 1) << 16) - x
 *
 * The subtract-one is done BEFORE the shift, so a flipped attach
 * point lands on the last pixel column rather than one past it. The
 * comment in the source is right that Y is not flipped: IANIOFFY is
 * used as-is either way.
 */
void wm_anipt_getxy(int32_t aniofx, int32_t aniofy, int32_t isizex,
                    bool fliph, int32_t *out_x, int32_t *out_y);

/* DISPLAY.EQU:123 B_FLIPH equ 4. */
#define WM_OCTRL_B_FLIPH 4

/* ---- vol_to_ht (MENU.ASM:898) ------------------------------------ */

/*
 * "Converts a 0-255 volume value to a 1-170 bar height value":
 * `mpyu 169 / divu 255 / inc`. Unsigned throughout, and the `inc` is
 * why the range starts at 1 and not 0 -- silence still draws a bar
 * one pixel high.
 */
uint32_t wm_vol_to_ht(uint32_t volume);

/* ---- form_crc32 (AUDIT.ASM) -------------------------------------- */

/*
 * It is not a CRC32. The name says so and the loop does not:
 *
 *     clr a1
 *   crc_lp
 *     calla RC_BYTEI          ; next CMOS byte into a0
 *     xor   a0,a1
 *     rl    1,a1              ; rotate the ACCUMULATOR left one
 *     dsjs  a6,crc_lp
 *
 * -- an xor-and-rotate rolling checksum over ADJ_BYTES_TO_CHECK
 * bytes of the adjustment block, with no polynomial and no table.
 * Whatever it is called, it is what decides whether the operator
 * adjustments are considered intact, so it has to match bit for bit;
 * the rotate is 32-bit and this says so explicitly rather than
 * relying on a shift.
 */
uint32_t wm_form_crc32(const uint8_t *bytes, size_t count);


/* ---- obj_on / obj_off (SELECT.ASM:2823, :2835) ------------------- */

/*
 * How the select screen hides an object: it does not touch a flag or
 * unlink anything, it sets bit 10 of OYPOS.
 *
 *     obj_off:  move *a8(OYPOS),a0 / ori   400h,a0 / move a0,*a8(OYPOS)
 *     obj_on:   move *a8(OYPOS),a0 / andni 400h,a0 / move a0,*a8(OYPOS)
 *
 * 400h is 1024, which on a 254-line screen is far enough below it
 * that the object is simply never drawn. Two things follow from it
 * being a BIT rather than an offset. It is idempotent -- hiding a
 * hidden object does nothing -- which a plain `add 1024` would not
 * be. And any object whose real Y already had bit 10 set would be
 * shown by obj_off and hidden by obj_on; nothing on this screen sits
 * below 1024, so it never comes up, but it is why this is a bit
 * operation here and not an add.
 */
#define WM_OBJ_OFFSCREEN_BIT 0x400

int16_t wm_obj_on(int16_t oypos);
int16_t wm_obj_off(int16_t oypos);
bool wm_obj_is_off(int16_t oypos);

/* ---- scrn_rel_off (PROGRESS.ASM:1786) ---------------------------- */

/*
 * Walk the object list clearing M_SCRNREL, so everything left on
 * screen scrolls with the world again instead of being pinned to the
 * display. One object is spared:
 *
 *     move  *a14(OID),a3
 *     cmpi  CREDITID|CLSDEAD,a3
 *     jrz   #sro_loop
 *
 * -- the credit message, which has to stay pinned whatever the
 * camera does. The test is an EXACT compare against the whole OID,
 * not a masked one, so an object that merely shares the class is not
 * spared.
 */
/* DISPLAY.EQU:115 `M_SCRNREL equ 2000h`. */
#define WM_OFLAGS_M_SCRNREL 0x2000

typedef struct {
    uint16_t oid;
    uint16_t oflags;
} wm_obj_flags_t;

/*
 * `spare_oid` is CREDITID|CLSDEAD as the caller's constants spell
 * it. Returns how many objects had the flag cleared.
 */
size_t wm_scrn_rel_off(wm_obj_flags_t *objs, size_t count,
                       uint16_t spare_oid);

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_OBJUTIL_H */
