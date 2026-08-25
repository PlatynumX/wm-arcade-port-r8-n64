# Combat2ES R37C clean build handoff

- base: fix39-v13e-r37-remaining-gap-closure
- preserves R37 MATCH_INIT, SD-card, renderer, and PSTATUS adapter closure
- SD runtime path: `sd:/wm_arcade/hiscore.whs`
- host SD regression residue removed and ignored
- renderer audit checks `wm_render_layer_order_is_source_safe`
- intended next action: existing `.github/workflows/build.yml` N64 ROM job
- hardware screenshot/pixel-perfect renderer comparison remains external validation
