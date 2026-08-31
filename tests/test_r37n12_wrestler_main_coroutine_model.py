#!/usr/bin/env python3
FIRST = "calc_closest"
POST = "post_sleep"

def dispatch(resume):
    seq = []
    if resume == FIRST:
        seq += ["calc_closest"]
    else:
        seq += [
            "risk", "update_joystat", "count_button_presses",
            "keep_onscreen", "veladd_1", "friction_1", "animate",
            "set_collision_boxes_1", "confine_fix1", "calc_closest2",
            "move_wrestler", "veladd_2", "friction_2", "update_links",
            "set_collision_boxes_2", "overlap_collision",
            "master_keep_attached", "set_wrestler_xflip",
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
    "calc_closest",
    "are_we_in_ring", "set_collision_boxes_loop", "confine_fix2",
    "update_newfacing", "update_positions", "drone_main", "SLEEPR",
]
resume, steady = dispatch(resume)
assert steady.index("animate") < steady.index("move_wrestler")
assert steady.index("move_wrestler") < steady.index("veladd_2")
assert steady.index("veladd_2") < steady.index("update_links")
assert steady.index("countdowns") < steady.index("are_we_in_ring")
assert steady.index("update_newfacing") < steady.index("drone_main")
assert steady[-1] == "SLEEPR"

ptime = [1, 0x7fff]
ran = []
for slot in range(2):
    ptime[slot] = (ptime[slot] - 1) & 0xffff
    signed = ptime[slot] if ptime[slot] < 0x8000 else ptime[slot] - 0x10000
    if signed <= 0:
        ran.append(slot)
        if slot == 0:
            ptime[1] = 1
assert ran == [0, 1]
print("R37N12 wrestler_main coroutine model: PASS")
