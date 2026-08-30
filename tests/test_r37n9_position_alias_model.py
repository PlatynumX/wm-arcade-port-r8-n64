#!/usr/bin/env python3
"""Executable model for source WORD/LONG position alias semantics fixed in R37N9."""

def s16(v):
    v &= 0xffff
    return v - 0x10000 if v & 0x8000 else v

def u32(v):
    return v & 0xffffffff

def make_fixed(integer, frac):
    return u32(((integer & 0xffff) << 16) | (frac & 0xffff))

def fixed_int(fixed):
    return s16((fixed >> 16) & 0xffff)

def write_posint_word(fixed, integer):
    return u32(((integer & 0xffff) << 16) | (fixed & 0xffff))

# COLLIS.ASM writes the integer WORD only: fractional position survives.
f = make_fixed(100, 0x1234)
f = write_posint_word(f, 95)
assert fixed_int(f) == 95
assert (f & 0xffff) == 0x1234

# Signed integer aliases remain signed after high-word extraction.
f = make_fixed(-10, 0xabcd)
assert fixed_int(f) == -10
assert (f & 0xffff) == 0xabcd

# ANIM.ASM::_ani_offset (opcode 21) writes X/Y/Z POSINT WORDs.  Each offset
# changes the signed integer half while preserving the source fractional half.
x = write_posint_word(make_fixed(50, 0x1010), 50 + 7)
y = write_posint_word(make_fixed(80, 0x2020), 80 - 9)
z = write_posint_word(make_fixed(20, 0x3030), 20 + 3)
assert (fixed_int(x), x & 0xffff) == (57, 0x1010)
assert (fixed_int(y), y & 0xffff) == (71, 0x2020)
assert (fixed_int(z), z & 0xffff) == (23, 0x3030)

# ANIM.ASM::_ani_oppoffset (opcode 82) is the same kind of WORD write and is
# especially relevant while a reciprocal wrestler attachment is active.
opp_x = write_posint_word(make_fixed(125, 0x4444), 125 - 12)
opp_y = write_posint_word(make_fixed(90, 0x5555), 90 + 6)
assert (fixed_int(opp_x), opp_x & 0xffff) == (113, 0x4444)
assert (fixed_int(opp_y), opp_y & 0xffff) == (96, 0x5555)

# ANIM.ASM::_ani_ground (opcode 125) writes only OBJ_YPOSINT; it does not clear
# the fractional word merely because the target integer is GROUND_Y.
grounded = write_posint_word(make_fixed(13, 0xbeef), 0)
assert fixed_int(grounded) == 0
assert (grounded & 0xffff) == 0xbeef

# master_keep_attached source LONG writes inherit the master's fraction when
# adding integer offsets, and the integer alias is immediately the new high word.
master = make_fixed(100, 0x1111)
slave = u32(master + (10 << 16))
assert fixed_int(slave) == 110
assert (slave & 0xffff) == 0x1111

print("R37N9 position-alias model: PASS")
