# Combat2ES R27 runtime scheduler regression

Base: fix39-v13e-combat2es-r26b-runtime-manifest-regression

This pass adds host-side runtime coverage for scheduler behavior that sits below
the static source manifest and beyond R26B's manifest creation checks.

Covered:
- busy SPECIAL_MOVE_ADDR rewinds a monitor without firing
- combo rejection rewinds without queueing a result
- reset/kill operations are scoped to the owner slot
- ck_ignore can block a fully charged flying-kick release

No gameplay behavior was changed.
