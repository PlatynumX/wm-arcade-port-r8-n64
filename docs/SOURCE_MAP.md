# Original-source map — r6

| Original material | Portable/N64 use |
|---|---|
| `ANIM.EQU` | animation command IDs used by the C VM |
| `FINISEQ.ASM::hrt_finish1_move` | translated command-stream smoke test |
| `FINISEQ.ASM::und_coffin_up` and below | the Undertaker's coffin finish: the state machine, velocities, ramps and RNDRNG0 draws in `wm/arcade/wm_arcade_coffin.h`, its ANI_CODE routines in the VM registry, and its object choreography ledgered |
| `HRTSEQ1.ASM` | Bret idle, directional walk and run visible frame timing |
| `HRTSEQ2.ASM` | Bret punch/kick visible frame timing |
| `IMG/BRET.LOD` | maps source frame labels to WIMP `.IMG` containers |
| Bret WIMP `.IMG` containers | CI8 pixels, palette and animation registration points converted for N64 |

r6's health, AI and contact windows are new portable scaffolding used to exercise the source art and animation paths on hardware. They are intentionally separated from claims of original arcade combat semantics. The next combat milestone is to translate the source hitbox/attack command behavior and replace these temporary contact windows.
