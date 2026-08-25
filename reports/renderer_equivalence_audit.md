# R36 renderer equivalence audit

- status: PASS_WITH_HARDWARE_SCREENSHOT_BOUNDARY
- scope: palette transparency, z/layer order, source coordinate conversion, WIMP frame metadata guard
- ignored deltas: intentional Midway Sports / Be a Man / PlatynumX replacement assets
- boundary: no pixel-perfect hardware screenshot claim in this pass

## checks

- [PASS] render_equivalence_core_compiled: CMake includes render equivalence core
- [PASS] renderer_regression_compiled: CMake includes renderer equivalence regression
- [PASS] transparent_ci8_index_zero: CI8 index 0 is the renderer transparent key
- [PASS] layer_order_defined: Rope halves and actor layers are distinct
- [PASS] layer_order_runtime_guard: Runtime guard validates ascending draw order
- [PASS] visual_first_frame_duration: Visual timing preserves first source frame
- [PASS] secondary_offsets_source_formula: Composite secondary offsets preserve source attach formula
- [PASS] wimp_frame_fail_closed: WIMP/IANI3 frame box adapter fails closed on invalid metadata
