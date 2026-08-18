#include "wm/composite.h"

void wm_secondary_display_offsets(int primary_xani, int primary_yani,
                                  int attach_x, int attach_y,
                                  int secondary_xani, int secondary_yani,
                                  int *out_xoff, int *out_yoff) {
    if (out_xoff)
        *out_xoff = primary_xani - attach_x + secondary_xani;
    if (out_yoff)
        *out_yoff = primary_yani - attach_y + secondary_yani;
}
