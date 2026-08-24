# Combat2DQ — coherent ATTRACT ABI + source parity

DN/DP mixed the Fix39 ATTRACT implementation with an older core data ABI (5 active hints / 5-field WmAttractHint). DQ makes all overlapping wmania_attract_* modules Fix39-owned while preserving the newer core wm_arcade_roster.c. The public ATTRACT data header is converged to the same expanded structure.

ATTR.ASM WHICH_HINT order is HNT_2,HNT_4,HNT_3,HNT_7,HNT_5,HNT_8,HNT_1,HNT_6,HNT_9,HNT_9 with body counts 4,6,6,6,5,6,4,3,5,5.
