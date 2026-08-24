# Combat2DQ — ATTRACT source-parity correction

Combat2DQ keeps Combat2DP dependency-closed ownership and corrects only the active ATTRACT hint table to the executable table in the original arcade ATTR.ASM.

Original WHICH_HINT order: `HNT_2, HNT_4, HNT_3, HNT_7, HNT_5, HNT_8, HNT_1, HNT_6, HNT_9, HNT_9`.

Executable body counts: `4, 6, 6, 6, 5, 6, 4, 3, 5, 5`.

The tenth row deliberately repeats HNT_9. HNT_A exists as data but is not in WHICH_HINT. HNT_7 has a count of six; two additional pointers follow it but are not executed by the count-driven loop.

The core-owned attract data provider is patched in place so the newer core roster/presenter remains authoritative. The bundled Fix39 data copy is corrected too.
