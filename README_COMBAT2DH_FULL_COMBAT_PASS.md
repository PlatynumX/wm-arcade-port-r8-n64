# Combat2DH — full combat ANI_CODE source pass

This revision stops treating ANI_CODE completion as a sequence of isolated labels. It inventories the shipped wrestler animation programs and completes the remaining state-changing native callbacks whose bodies are present in the supplied Midway ASM payload.

New direct source translations cover roll status/motion, fling/hiptoss/special cooldown gates, elbow and falling-opponent targeting, source Z-velocity calculation, pin ground-hit alignment/animation, reappearance positioning, coffin movement speeds, turnbuckle confinement/targeting, palette restore, and coffin door state. Source-only visual/announcement helpers are routed through the existing source event sink rather than silently discarded.

External symbols referenced by the animation modules but not defined in the supplied payload remain diagnostics; this revision does not invent their behavior. The package adds an inventory regression so the source-contained state callbacks above cannot silently fall back again.
