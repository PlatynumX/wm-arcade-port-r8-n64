#!/usr/bin/env python3
"""Model the Midway process-list ordering used by wrestler_main.

No N64 behavior is invented here.  This is a tiny executable model of:
- MPROC.ASM GETPRC/GETSPRC: insert immediately after current parent.
- MPROC.ASM GETPRC_INSERT: insert immediately before parent.
- WRESTLE.ASM wrestler creation: SCREATE WMAIN then CREATE GETUP.
- WRESTLE2.ASM init_smoves: GETPRC_INSERT watchdogs before owning WMAIN.
"""


def insert_after(lst, parent, item):
    k = lst.index(parent)
    lst.insert(k + 1, item)


def insert_before(lst, parent, item):
    k = lst.index(parent)
    lst.insert(k, item)


def source_list(actor_count, smoves_per_actor=2):
    parent = "PARENT"
    lst = [parent]
    # Creator makes WMAIN then GETUP for each wrestler in ascending player slot.
    for i in range(actor_count):
        insert_after(lst, parent, f"W{i}")
        insert_after(lst, parent, f"G{i}")

    # When WMAIN first executes init_smoves, GETPRC_INSERT puts watchdogs
    # directly before WMAIN, preserving source table order.
    for i in range(actor_count):
        for s in range(smoves_per_actor):
            insert_before(lst, f"W{i}", f"S{i}.{s}")
    return lst[1:]


def translated_list(actor_count, smoves_per_actor=2):
    out = []
    for i in reversed(range(actor_count)):
        out.append(f"G{i}")
        out.extend(f"S{i}.{s}" for s in range(smoves_per_actor))
        out.append(f"W{i}")
    return out


def n12_wrong_list(actor_count, smoves_per_actor=2):
    # N12 globally ticked all SMOVEs, then WMAIN->GETUP in ascending actor order.
    out = [f"S{i}.{s}" for i in range(actor_count) for s in range(smoves_per_actor)]
    for i in range(actor_count):
        out.extend([f"W{i}", f"G{i}"])
    return out


def dispatch_ptime(ptime):
    # PTIME is a 16-bit word; branch decision is signed after decrement.
    ptime = (ptime - 1) & 0xFFFF
    signed = ptime if ptime < 0x8000 else ptime - 0x10000
    return ptime, signed <= 0


def main():
    assert source_list(2) == ["G1", "S1.0", "S1.1", "W1", "G0", "S0.0", "S0.1", "W0"]
    assert source_list(4) == translated_list(4)
    assert source_list(2) == translated_list(2)
    assert source_list(2) != n12_wrong_list(2)

    # MPROC signed 16-bit decrement semantics.
    assert dispatch_ptime(1) == (0, True)
    assert dispatch_ptime(2) == (1, False)
    assert dispatch_ptime(0) == (0xFFFF, True)
    assert dispatch_ptime(0x7FFF) == (0x7FFE, False)

    # Wake ordering is directional within a dispatcher pass.
    # Source order for two WMAINs is W1 before W0.  W1 can set W0 PTIME=1
    # before W0 is visited, allowing W0 this pass.  A wake from W0 to W1 is
    # necessarily deferred because W1 has already been visited.
    order = [x for x in source_list(2, 0) if x.startswith("W")]
    assert order == ["W1", "W0"]
    assert order.index("W1") < order.index("W0")

    print("R37N13 process-dispatch model: PASS")


if __name__ == "__main__":
    main()
