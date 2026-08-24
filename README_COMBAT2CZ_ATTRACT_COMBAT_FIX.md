# Combat2CZ — attract combat walk/animation ownership fix

Input baseline: Combat2CY.

The hardware attract capture exposed a source-parity hole that earlier audits missed. `fix39_execute_walk_source_patch.py` had translated the WRESTLE.ASM velocity math but omitted `change_walk_anim` / `set_rotate_anim`. After an attack completed, CPU movement could resume while the last attack frame remained displayed, producing the frozen-pose glide visible in the capture.

Combat2CZ restores the source directional walk/stance/turn animation selection for all eight wrestlers using the common Midway label matrix and each wrestler's source prefix. It also fixes a deeper ANIM.ASM ownership error: primary `ANIMODE/ANICNT` and torso `ANIMODE2/ANICNT2` now have independent VM state. Previously both portable VM instances wrote the same actor mode/count fields, allowing a torso stream to reset/end a primary attack stream.

No new combat behavior is invented. The changes specifically restore WRESTLE.ASM `execute_walk`, `change_walk_anim`, `set_rotate_anim`, and ANIM.ASM dual-channel semantics.
