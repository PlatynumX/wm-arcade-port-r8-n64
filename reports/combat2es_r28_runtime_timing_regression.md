# Combat2ES R28 runtime timing regression

Base: fix39-v13e-combat2es-r27-runtime-scheduler-regression

Adds host CTest coverage for runtime scheduler timing edges:

- first tick sleep after source monitor creation
- wrong input reset and timed-step timeout reset
- post-fire sleep rewind using manifest post_fire_sleep
- source finish filter behavior
- all 63 manifest monitors fitting under WM_ARCADE_SMOVE_MAX_PROCS
- inactive owner slot skip behavior

No gameplay implementation files are changed.
