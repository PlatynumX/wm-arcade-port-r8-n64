#!/usr/bin/env python3
# R37N13-corrected compatibility model for the original R37N12 test name.
FIRST = "calc_closest"
POST = "post_sleep"

def dispatch(resume):
    seq = []
    if resume == FIRST:
        seq += ["calc_closest"]
    else:
        seq += [
            "risk", "update_joystat", "count_button_presses",
            "keep_onscreen", "veladd", "friction", "animate",
            "set_collision_boxes_1", "confine_fix1", "calc_closest2",
            "move_wrestler", "update_links", "set_collision_boxes_2",
            "overlap_collision", "master_keep_attached", "set_wrestler_xflip",
            "update_joy_dtime", "countdowns",
        ]
    seq += [
        "are_we_in_ring", "set_collision_boxes_loop", "confine_fix2",
        "update_newfacing", "update_positions", "drone_main", "SLEEPR",
    ]
    return POST, seq

resume, first = dispatch(FIRST)
assert resume == POST
assert first == [
    "calc_closest", "are_we_in_ring", "set_collision_boxes_loop",
    "confine_fix2", "update_newfacing", "update_positions", "drone_main",
    "SLEEPR",
]
resume, steady = dispatch(resume)
assert steady.count("veladd") == 1
assert steady.index("animate") < steady.index("move_wrestler")
assert steady.index("move_wrestler") < steady.index("update_links")
assert steady.index("countdowns") < steady.index("are_we_in_ring")
assert steady[-1] == "SLEEPR"

# MPROC/WRESTLE creation order for two WMAIN groups is W1 then W0.
ptime = [1, 0x7fff]
ran = []
for slot in (1, 0):
    ptime[slot] = (ptime[slot] - 1) & 0xffff
    signed = ptime[slot] if ptime[slot] < 0x8000 else ptime[slot] - 0x10000
    if signed <= 0:
        ran.append(slot)
        # If W0 wakes W1 here, W1 has already been visited and waits until next pass.
        if slot == 0:
            ptime[1] = 1
assert ran == [0]
assert ptime[1] == 1
print("R37N12/R37N13 wrestler_main compatibility model: PASS")
