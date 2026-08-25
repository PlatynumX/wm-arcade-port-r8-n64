# Combat2ES R37 remaining-gap closure

- Base: fix39-v13e-r35-remaining-noncombat-source-closure
- SD persistence: actual parent-directory bug fixed; target remains `sd:/wm_arcade/hiscore.whs`; write/read/reopen codec regression required.
- Renderer: R36 code-level palette/layer/source-coordinate/WIMP parity invariants retained.
- Normal progression: `MATCH_INIT` resolves CURRENT_LADDER opponent through SELECT's source-id mapping, calls `wm_fix39_match_begin`, then enters live `WM_APP_MODE_MATCH`.
- Cabinet/operator boundary: source PSTATUS bits and accepted-player-start bookkeeping are centralized in an explicit N64 adapter; no synthetic coin credits are invented.
- Intentional branding delta ignored: Midway Sports / Be a Man / PlatynumX assets only.
- Hardware screenshot pixel-perfect renderer validation remains external evidence, not a fake software PASS.
