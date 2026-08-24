# Combat2ED — live teammate source parity

Authority: `source_payload/anim/WRESTLE2.ASM`, `ck_live_teammates`.

EC's live callback returned false unconditionally because the then-live attract path was 1v1.
That was contextually safe but was not a translation of Midway's routine.

ED replaces that shortcut with the source predicate, in source order:

1. walk `process_ptrs`;
2. skip inactive entries;
3. skip self;
4. require equal `PLYR_SIDE`;
5. skip `MODE_DEAD`;
6. return true/carry-set on the first remaining wrestler;
7. otherwise return false/carry-clear.

No gameplay rule was invented. The N64 runtime currently exposes two actor slots, but the
callback now has Midway's actual teammate semantics for every slot the runtime exposes.

Regression proof:
`tests/test_combat2ed_live_teammates_source_parity.py`
