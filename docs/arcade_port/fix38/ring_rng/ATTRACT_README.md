# WrestleMania Arcade -> N64 non-gameplay attract + hi-score handoff

This package supersedes `wmania_hiscore_complete.zip` for the next merge
conversation. The complete hi-score subsystem is included unchanged, plus a
source-oriented port of the remaining non-gameplay attract-mode logic.

Included:
- complete hi-score subsystem
- exact attract-loop scheduler
- DCS/Midway/title integration callbacks
- two explicit gameplay-demo placeholders (no gameplay code)
- `CRD_SCRN2` credits wrapper dependency
- five active designer-hint rotation
- fixed general-tips screen model
- eight-wrestler bio rotation + special-moves/tips variant
- source bio metadata and tune IDs
- operator/custom-message logic with external source dimensions
- current-source `dan_test` bouncing-ball backdrop simulation
- even-loop date/time system and formatter
- every-eighth-loop copyright presentation model
- AAMA advisory presentation + source gradient model
- button-wait sound suppression behavior
- hidden octopus input-sequence attract path
- source-label text extraction helper
- host-side tests

Excluded by request:
- gameplay demo implementation

Read `ATTRACT_INTEGRATION.md` before merging.
