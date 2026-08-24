# Combat2DY strict runtime parity correction

Combat2DY folds the external Combat2DW strict-source audit into the live build contract.

* Presenter/demo screen coordinates cannot feed back into translated WRESTLE actor state. The legacy presenter-pose API is diagnostic-only behind `WM_FIX39_DIAGNOSTIC_PRESENTER_POSE`.
* The attract adapter no longer exposes an explicitly unimplemented gameplay callback. It builds the exact `ATTR.ASM::show_gameplay` plan and passes that plan across the platform boundary.
* The royal-rumble CPU-loss match-flow comment is corrected to the exact `WRESTLE.ASM #rr_cpuwon` state transition; behavior was already source-shaped and is not fabricated.
* After canonical source regeneration, the build fails if any live DRONE/attack generated header is still a committed placeholder or if scripts/services/translated bodies are empty.
* A strict runtime audit runs before host/N64 compilation.

No presenter-space coordinate clamping or invented AI behavior is introduced.
