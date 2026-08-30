#!/usr/bin/env python3
"""Independent model checks for ANIM.ASM slot/change scheduler semantics."""
from dataclasses import dataclass

END = 0x0001
NOGRAVITY = 0x0020
STATUS = 0x0200
GRAVITY = 0x00008000

@dataclass
class Actor:
    primary_mode: int = 0
    primary_count: int = 1
    gravity: int = 0x123456
    frame: str | None = "old_frame"

@dataclass
class Slot:
    program: str | None = None
    mode: int = 0
    count: int = 1
    frame: str | None = "old_frame"
    secondary: bool = False
    primes: int = 0


def conditional_change(a: Actor, s: Slot, program: str) -> bool:
    if s.program == program and not (s.mode & END):
        return False
    restart_and_prime(a, s, program)
    return True


def restart_and_prime(a: Actor, s: Slot, program: str) -> None:
    s.program = program
    s.mode = 0
    s.count = 1
    # Source change_anim1a/2a does not clear CUR_FRAME before animate.
    if not s.secondary:
        a.gravity = GRAVITY
    s.primes += 1


def secondary_gravity_on(a: Actor, s: Slot) -> None:
    assert s.secondary
    # _ani_gravity_on names a13(ANIMODE), not a10(OANIMODE).
    a.primary_mode &= ~NOGRAVITY


def main() -> None:
    a = Actor(primary_mode=NOGRAVITY | STATUS)
    s = Slot(program="stand", secondary=True)
    before = (s.count, s.frame, s.primes)
    changed = conditional_change(a, s, "stand")
    assert not changed
    assert (s.count, s.frame, s.primes) == before

    # Different animation restarts and primes, but preserves old frame until a
    # frame command itself replaces current-frame state.
    changed = conditional_change(a, s, "walk")
    assert changed and s.program == "walk" and s.mode == 0 and s.count == 1
    assert s.frame == "old_frame" and s.primes == 1
    assert a.gravity == 0x123456  # change_anim2a does not reset gravity

    restart_and_prime(a, s, "walk")
    assert s.primes == 2  # change_anim1a/2a is unconditional

    secondary_gravity_on(a, s)
    assert (a.primary_mode & NOGRAVITY) == 0
    assert (a.primary_mode & STATUS) != 0

    p = Slot(program="stand", secondary=False)
    a.gravity = 0x55
    restart_and_prime(a, p, "run")
    assert a.gravity == GRAVITY  # change_anim1a resets gravity
    assert p.frame == "old_frame"

    # Exact frame tick state: ANIM.ASM stores scaled zero, it does not coerce 1.
    p.count = 0
    assert p.count == 0
    print("R37N10 animation-slot scheduler model: PASS")

if __name__ == "__main__":
    main()
