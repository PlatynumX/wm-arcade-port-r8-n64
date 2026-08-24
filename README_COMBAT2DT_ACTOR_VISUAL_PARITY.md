# Combat2DT — asymmetric actor-state + frame-local visual parity

Hardware evidence from Combat2DS shows that the attract match is not globally frozen: P2 advances through multiple positions/poses while P1 remains parked. DT therefore stops treating the symptom as a global scheduler failure and records DRONE/input/position liveness independently for each actor.

The same hardware sequence shows a coherent but severely garbled P2 silhouette. The N64 character renderer previously replaced each live frame's own palette with the wrestler base-frame palette. That is unsafe for the original WIMP assets: the CI8 pixels and TLUT are a frame-local decoded pair. DT makes the current source frame's palette authoritative, falling back only if that frame has no palette, and the character asset generator now refuses any frame whose decoded pixel count or palette-index range is inconsistent.

No ownership policy is changed in DT and no synthetic AI/ring clamping is introduced.
