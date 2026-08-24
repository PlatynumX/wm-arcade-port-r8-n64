# Combat2CQ audit

Combat2CQ removes the stale attract-demo collision regression that contradicted the Combat2CO/CP single-authority cutover.

The WIMP/IANI3 decoder and runtime frame-box APIs remain required. The regression now explicitly forbids `fix39_bind_demo_frame_boxes` and presenter-to-world writes in ATTR gameplay, while requiring translated runtime frame-box ownership.

The Termux revision/log/branch/ROM labels are also advanced to Combat2CQ so diagnostics identify the package that actually ran.
