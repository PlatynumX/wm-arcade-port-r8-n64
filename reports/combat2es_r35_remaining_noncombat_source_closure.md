# Combat2ES R35 remaining non-combat source-closure pass

- Base: fix39-v13e-dcs-r2b-decoded-port-assets
- Scope: all remaining non-combat systems manifest + install/wire obvious completed pieces + fail/report remaining gaps
- Intentional ignored asset delta: Midway Sports / Be a Man / PlatynumX replacement art
- Gameplay combat scope: excluded; R33B/R34 own combat/adjacent combat
- Main installed/wired piece: decoded DCS R2B command bindings generated into C and recorded by wm_audio_send_command/wm_audio_send_routed_command
- Gap handling: remaining non-combat gaps are reported in reports/remaining_noncombat_systems_manifest.md/json; no fake completion claim

This pass is deliberately a closure manifest and wiring pass. It is allowed to report YELLOW/RED remaining work without failing the package, but it fails if decoded DCS asset binding is missing or if fake-complete markers are discovered by the audit.
