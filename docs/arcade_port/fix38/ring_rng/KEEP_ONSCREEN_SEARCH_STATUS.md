# `keep_onscreen` source audit — Ring Chunk 4

Status: **source body still not located; no implementation invented.**

## What is proven

Current `WRESTLE.ASM` declares:

`keep_onscreen`

as an external `.ref` and calls it in the wrestler main loop immediately
after joystick/button bookkeeping and before `wrestler_veladd`.

The final `WRESTLE.CMD` link command explicitly enumerates the objects linked
into the executable. It does not name an external gameplay library.

## Linked source modules audited

Exact-symbol searches have been performed across the linked gameplay/support
modules including:

- WRESTLE / WRESTLE2
- ANIM
- SPECIAL
- GETUP
- DRONE
- AWARD
- DCSSOUND
- PATCH
- ATTRACT
- STRING
- SQUARE
- ADJUST
- AUDIT
- TEST
- DIAG
- MENU
- HSTD
- SELECT
- TABLES
- ROPES
- LIFEBAR
- SCREEN
- PROGRESS
- FIREWORK
- STORIES
- REACT1..REACT9
- BAM / BRET / DOINK / RAZOR / LEX / SHAWN / TAKER / YOKO
- COLLIS
- CROWD
- DISPLAY
- MAIN
- UTIL
- PAL
- MPROC
- BAKGND
- FINISEQ

The current linker comments out `robo.obj` and `coll2.obj`, but those source
files were searched too.

Image/palette/table-only object inputs are not plausible code providers.

## Backup audit

The repository's `BACKUP/WRESTLE.ASM`, `BACKUP/ANIM.ASM`,
`BACKUP/COLLIS.ASM`, plus previously checked backup gameplay copies, also
do not contain `keep_onscreen`.

Notably, the older backup WRESTLE source does not even carry the current
`keep_onscreen` external reference, which suggests the call was introduced
later than that backup snapshot.

## Map artifact

`WRESTLE.CMD` requests a `d:\wrestle.map` linker map, but a `WRESTLE.MAP`
file is not present/retrievable at the repository root in this source
snapshot.

## Conclusion

The checked source snapshot is missing the definition (or is a version mix
where the calling WRESTLE.ASM is newer than the file that once provided the
routine).

Because the project requires direct translation, Ring Chunk 4 deliberately
does **not** synthesize a screen/fence clamp from nearby `confine_wrestler`,
`allow_offscrn`, ring geometry, or call-site behavior.

If another historical source revision/object/map is located later, this
status should be revisited.
