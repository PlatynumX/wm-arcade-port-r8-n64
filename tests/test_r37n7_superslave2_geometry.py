#!/usr/bin/env python3
"""Executable model of the recovered WRESTLE2/ANIM superslave geometry math."""

def calc(raw_x,raw_y,att_x,att_y,def_x,def_y,def_w,att_fliph,att_facing_right,att_facing_left,table_flip):
    if not att_fliph:
        match=not att_facing_left
    else:
        match=not att_facing_right
    dx=def_x
    if (match and not table_flip) or ((not match) and table_flip):
        dx=def_w-dx
    x=raw_x+dx-(att_x & 0xffff)
    if not match: x=-x
    y=raw_y-def_y+att_y
    defender_fliph=bool(att_facing_right)
    if table_flip: defender_fliph=not defender_fliph
    return x,y,defender_fliph,match

# Deliberately asymmetric geometry so all four paths have distinct expected X.
A=dict(raw_x=13,raw_y=-7,att_x=21,att_y=40,def_x=17,def_y=31,def_w=83)
cases=[
    # attacker visual flip matches facing, table no-flip -> complement defender xoff
    (False,True,False,False,(58,2,True,True)),
    # match + table flip -> leave defender xoff
    (False,True,False,True,(9,2,False,True)),
    # mismatch + no table flip -> leave defender xoff, mirror whole X
    (False,False,True,False,(-9,2,False,False)),
    # mismatch + table flip -> complement defender xoff, mirror whole X
    (False,False,True,True,(-58,2,True,False)),
]
for fliph,fr,fl,tflip,expect in cases:
    got=calc(**A,att_fliph=fliph,att_facing_right=fr,att_facing_left=fl,table_flip=tflip)
    assert got==expect,(fliph,fr,fl,tflip,got,expect)
# Exercise the other mismatch source branch: sprite FLIPH set while facing right.
got=calc(**A,att_fliph=True,att_facing_right=True,att_facing_left=False,table_flip=False)
assert got==(-9,2,True,False),got
print('R37N7 superslave2 four-path geometry model: PASS')
