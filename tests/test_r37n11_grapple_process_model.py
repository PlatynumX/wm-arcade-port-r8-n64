#!/usr/bin/env python3
"""Independent model of MPROC PTIME/process-list behavior used by R37N11."""

KOD = 1 << 7


class P:
    def __init__(self, ptime=1, kod=False):
        self.ptime = ptime & 0xFFFF
        self.kod = kod
        self.runs = 0
        self.drone_runs = 0
        self.getup_runs = 0


def s16(v):
    v &= 0xFFFF
    return v - 0x10000 if v & 0x8000 else v


def dispatch(p):
    p.ptime = (p.ptime - 1) & 0xFFFF
    if s16(p.ptime) <= 0:
        p.runs += 1
        return True
    return False


def sleep_loop(p):
    p.ptime = 0x7FFF if p.kod else 1


def wake(p):
    p.ptime = 1


p = P(2)
assert dispatch(p) is False and p.ptime == 1
assert dispatch(p) is True and p.ptime == 0
sleep_loop(p)
assert p.ptime == 1

p = P(0)
assert dispatch(p) is True and p.ptime == 0xFFFF

p = P(1, True)
sleep_loop(p)
assert p.ptime == 0x7FFF
assert dispatch(p) is False and p.ptime == 0x7FFE

# Critical process-list property: A may wake later B and B runs in same pass.
a = P(1)
b = P(0x7FFF, True)
assert dispatch(a)
sleep_loop(a)
wake(b)
assert dispatch(b)

# GETUP is a separate process: it still gets a scheduler slice while wrestler
# B remains in a long KOD sleep. DRONE belongs to wrestler_main and does not.
b = P(0x7FFF, True)
if dispatch(b):
    b.drone_runs += 1
    sleep_loop(b)
b.getup_runs += 1
assert b.runs == 0 and b.drone_runs == 0 and b.getup_runs == 1

print("R37N11 grapple-process PTIME model: PASS")
