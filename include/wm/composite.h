#ifndef WM_COMPOSITE_H
#define WM_COMPOSITE_H

/*
 * Translate the arcade set_image() channel-2 attachment math into the
 * display-offset convention used by the portable renderer.
 *
 * Original ANIM.ASM does:
 *   secondary_dxoff = primary.IANIOFFX - primary.IANI2X + secondary.IANIOFFX
 *   secondary_dyoff = primary.IANIOFFY - primary.IANI2Y + secondary.IANIOFFY
 *
 * Keeping this as portable core logic prevents the N64 backend from growing
 * its own subtly different attachment math (especially when X-flipped).
 */
void wm_secondary_display_offsets(int primary_xani, int primary_yani,
                                  int attach_x, int attach_y,
                                  int secondary_xani, int secondary_yani,
                                  int *out_xoff, int *out_yoff);

#endif
