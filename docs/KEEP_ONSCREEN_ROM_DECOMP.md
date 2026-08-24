# `keep_onscreen` — recovered from WWF WrestleMania arcade ROM

Source status: the historical source snapshot calls `keep_onscreen` from
`WRESTLE.ASM`, but its source body is absent.

Ring Chunk 5 recovers the shipped rev 1.30 implementation directly from the
user-supplied arcade program ROM.

## ROM identity

Program ROM pair:

- `wwf.54` SHA1 `d93df59aed1672ab38af231d909d9df1a8e30f44`
- `wwf.63` SHA1 `cf548ff199428a93b9bc5f4fc1347c4a3cbdf106`

These two 512-KiB ROMs are interleaved as the 1-MiB TMS34010 main program
region mapped at bit addresses `0xFF800000..0xFFFFFFFF`.

## Binary anchor

The source wrestler loop says:

```asm
callr update_joystat
callr count_button_presses
calla keep_onscreen
calla wrestler_veladd
callr wrestler_friction
```

The corresponding compiled call pattern is unique in the program image.

Recovered addresses:

- source call site: `0xFF85CAB0`
- `keep_onscreen`: `0xFF8711C0`
- next CALLA, `wrestler_veladd`: `0xFF871730`

TMS34010 `CALLA` opcode is `0x0D5F` followed by the 32-bit absolute bit
address, making the target recovery direct rather than heuristic once the
call-site sequence is located.

## Main routine

Recovered disassembly:

```asm
FF8711C0: MOVE @010F2260,A14,0
FF8711F0: CMPI 0003,A14
FF871210: JRNE FF8714F0

FF871220: MOVE @01000140,A0,0
FF871250: ADDI 00C8,A0
FF871270: MOVE A0,A1
FF871280: CMPI 0432,A0
FF8712A0: JRGT FF871300
FF8712B0: SUBI 00B9,A0
FF8712D0: ADDI 00B9,A1
FF8712F0: JR FF871340
FF871300: SUBI 00B9,A0
FF871320: ADDI 00B9,A1

FF871340: MOVE @010F2A60,A10,1
FF871370: MOVE @010F2A80,A11,1

FF8713A0: MOVE @010F4400,A14,0
FF8713D0: JREQ FF871430
FF8713E0: DEC A14
FF8713F0: MOVE A14,@010F4400,0
FF871420: JRNE FF8714F0

FF871430: MOVE *A10(0560),A14,0
FF871450: JRNE FF871490
FF871460: MOVE *A11(0560),A14,0
FF871480: JREQ FF8714F0

FF871490: MOVE A10,A9
FF8714A0: CALLR FF871500
FF8714C0: MOVE A11,A9
FF8714D0: CALLR FF871500
FF8714F0: RETS
```

## Global mapping

### `0x010F2260` = `OLD_PSTATUS`

The routine returns unless `OLD_PSTATUS == 3`.

### `0x01000140` = integer half of `WORLDTLX`

`DISPLAY.ASM` defines `WORLDTLX` as 16:16 world-screen left X.

The ROM reads the integer half, adds 200, and therefore obtains the current
world-screen center X:

```text
center = WORLDTLX.int + 200
```

It then forms:

```text
left  = center - 185
right = center + 185
```

On a 400-pixel-wide view this leaves a 15-pixel safety margin from each
screen edge.

### `0x010F2A60`, `0x010F2A80` = `process_ptrs[0]`, `[1]`

The routine operates on the first two wrestler processes only.

### `0x010F4400` = `allow_offscrn`

If zero, confinement proceeds.

If nonzero:
1. decrement
2. store
3. return if the new value is still nonzero

Therefore `allow_offscrn == 1` becomes zero and confinement resumes on that
same call.

## Shipped inert branch

The ROM compares the screen center to:

`0x432 == RING_X_CENTER == 1074`

but both paths execute the exact same:

```asm
SUBI 00B9,A0
ADDI 00B9,A1
```

Ring Chunk 5 preserves the comparison as audit metadata but does not invent
a behavior difference that is not present in the shipped binary.

## Outside-ring gate

The helper runs for both P1 and P2 if **either** wrestler is outside:

```text
if P1.INRING != 0:
    check P1 and P2
else if P2.INRING != 0:
    check P1 and P2
else:
    return
```

This is not a per-player `INRING` test inside the local helper.

## Player helper at `0xFF871500`

Recovered disassembly:

```asm
FF871500: MOVE *A9(0110),A14,0
FF871520: CMP A0,A14
FF871530: JRGT FF871590

FF871540: MOVE *A9(02D0),A14,1
FF871560: JREQ FF871720
FF871570: JRN FF8715F0
FF871580: RETS

FF871590: CMP A1,A14
FF8715A0: JRLT FF871720
FF8715B0: MOVE *A9(02D0),A14,1
FF8715D0: JREQ FF871720
FF8715E0: JRN FF871720

FF8715F0: MOVE *A9(07E0),A14,0
FF871610: JRNE FF871720

FF871620: CLR A14
FF871630: MOVE A14,*A9(02D0),1

FF871650: MOVE *A9(05C0),A14,0
FF871670: CMPI 0001,A14
FF871690: JRNE FF871720

FF8716A0: MOVI 0000,A14
FF8716C0: MOVE A14,*A9(05C0),0
FF8716E0: MOVE A14,*A9(0660),0
FF871700: CALLR FF86E4B0

FF871720: RETS
```

`MPROC.EQU` places PDATA at process offset `+0x100`, giving:

| process offset | PDATA field |
|---:|---|
| `+0x110` | `OBJ_XPOSINT` |
| `+0x2D0` | `OBJ_XVEL` |
| `+0x560` | `INRING` |
| `+0x5C0` | `PLYRMODE` |
| `+0x660` | `ANIMODE` |
| `+0x7E0` | `CLIMBING_THRU` |

Functional result:

```text
left side:
    if x <= left AND xvel < 0 -> stop candidate

right side:
    if x >= right AND xvel > 0 -> stop candidate

otherwise -> return
```

Boundary equality is active.

Most importantly, the routine **does not clamp X position**. It stops only
outward X velocity.

Before stopping:
- if `CLIMBING_THRU != 0`, return without touching velocity

On stop:
- `OBJ_XVEL = 0`

If player was not running:
- return

If player was running:
- `PLYRMODE = MODE_NORMAL`
- `ANIMODE = 0`
- call `0xFF86E4B0`

## Running-player helper at `0xFF86E4B0`

Wrapper:

```asm
FF86E4B0: MOVE A13,-*SP,1
FF86E4C0: MOVE A9,A13
FF86E4D0: CALLR FF86E510
FF86E4F0: MOVE *SP+,A13,1
FF86E500: RETS
```

Inner helper:

```asm
FF86E510: MOVE *A13(0460),A0,0
FF86E530: JREQ FF86E6C0
FF86E540: MOVE *A13(0600),A0,0
FF86E560: JRNE FF86E6C0
FF86E570: MOVE *A13(0870),A0,1
FF86E590: JREQ FF86E6C0
FF86E5A0: MMTM SP,[...]
FF86E5C0: MOVE *A0(00C0),A8,1
FF86E5E0: MOVE *A0(00A0),A9,1
FF86E600: MOVE *A0(0080),A10,1
FF86E620: MOVI 012B,A1
FF86E640: MOVI FF86D7D0,A7
FF86E670: CALLA FF819520
FF86E6A0: MMFM SP,[...]
FF86E6C0: RETS
```

Field mapping:

| process offset | PDATA field |
|---:|---|
| `+0x460` | `GETUP_TIME` |
| `+0x600` | `PLYR_DIZZY` |
| `+0x870` | `METER_PROC` |

It only creates the new process when:

```text
GETUP_TIME != 0
PLYR_DIZZY == 0
METER_PROC != NULL
```

It copies saved `A8/A9/A10` from the existing meter process, then loads:

- PID `0x12B` (`GETUP_PID` in `GAME.EQU`)
- entry bit address `0xFF86D7D0`

and calls the process creator.

Ring Chunk 5 exposes that exact process-create request as
`WmRingGetupSpawn`; it does not recreate the get-up meter process itself.

## Reproducibility

`verify_keep_onscreen_rom.py` validates:

- exact U54/U63 SHA1s
- CALLA opcode/target at the source call site
- following `wrestler_veladd` CALLA
- SHA-256 of the recovered keep_onscreen machine-code range
- SHA-256 of the nested running-player helper range
