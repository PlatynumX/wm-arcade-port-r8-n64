#!/usr/bin/env python3
"""Behavior model for the source distinction R37N14 restores.

The offensive process's CHECKHIT/ATTACK box is independent of its own OBJ_COLL
box. A defender must have OBJ_COLL, but an unrelated missing defender must not
starve the entire collision pass.
"""
from dataclasses import dataclass

@dataclass
class A:
    active: bool = True
    checkhit: bool = False
    hurt_valid: bool = False
    attack_overlaps: tuple = ()

def scan(actors, odd=False):
    order = range(len(actors)-1, -1, -1) if odd else range(len(actors))
    for ai in order:
        a = actors[ai]
        if not a.active or not a.checkhit:
            continue
        # Source set_xyz does NOT require a.hurt_valid.
        for vi, v in enumerate(actors):
            if vi == ai or not v.active or not v.hurt_valid:
                continue
            if vi in a.attack_overlaps:
                return ai, vi
    return None

# Critical regression: attacker has no portable hurt snapshot, defender does.
actors = [A(checkhit=True, hurt_valid=False, attack_overlaps=(1,)),
          A(hurt_valid=True)]
assert scan(actors) == (0, 1)

# One unrelated invalid actor must not globally disable a valid pair.
actors = [A(checkhit=True, hurt_valid=True, attack_overlaps=(1,)),
          A(hurt_valid=True),
          A(hurt_valid=False)]
assert scan(actors) == (0, 1)

# Candidate defender without OBJ_COLL is skipped, not synthesized.
actors = [A(checkhit=True, hurt_valid=True, attack_overlaps=(1,)),
          A(hurt_valid=False)]
assert scan(actors) is None

# Preserve COLLIS.ASM odd-tick reverse attacker order.
actors = [A(checkhit=True, hurt_valid=True, attack_overlaps=(1,)),
          A(checkhit=True, hurt_valid=True, attack_overlaps=(0,))]
assert scan(actors, odd=False) == (0, 1)
assert scan(actors, odd=True) == (1, 0)

print("R37N14 collision readiness model: PASS")
