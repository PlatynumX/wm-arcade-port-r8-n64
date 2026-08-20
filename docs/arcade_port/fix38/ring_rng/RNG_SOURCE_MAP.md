# RNDRNG0 source map

## WrestleMania confirmation

`WRESTLE.ASM` advances the global `RAND` value once per main dispatch loop:

```asm
move @RAND,a1,L
rl   a1,a1
move @HCOUNT,a14
rl   a14,a1
add  sp,a1
move a1,@RAND,L
```

This establishes the exact random-state mixing used by WrestleMania itself.

## Williams/Midway shared utility implementation

The historical Williams `UTIL.ASM` source exposes the complete
`RNDRNG`, `RNDRNG0`, and `RNDRNGS` bodies.

`RNDRNG0`:

```asm
addk 1,a0
move @RAND,a1,L
rl   a1,a1
move @HCOUNT,a14
rl   a14,a1
add  sp,a1
move a1,@RAND,L
mpyu a1,a0
rets
```

Portable translation: `wmania_rng.c/.h`.

## Critical TMS34010 details

### `RL Rs,Rd`

This is **not** a fixed one-bit rotate.

TMS34010 `RL Rs,Rd` rotates `Rd` left by the low five bits of `Rs`.

So:

```asm
rl a1,a1
```

means:

```c
rand = rol32(rand, rand & 31);
```

and:

```asm
rl a14,a1
```

means:

```c
rand = rol32(rand, HCOUNT & 31);
```

### `MPYU A1,A0`

A0 is an even destination register.

For an even destination, TMS34010 `MPYU` writes a 64-bit product into the
destination register pair, with the **high 32 bits in A0**.

Therefore RNDRNG0's range conversion is exactly:

```c
high32((uint64_t)new_rand * (X + 1))
```

This produces `0..X` **inclusive** without modulo bias.

## Environment inputs

The source mixes:
- hardware `HCOUNT`
- TMS stack pointer `SP`

The N64 port must not silently substitute `rand()` for these.

`WmRng` supports:
- callbacks for translated/emulated HCOUNT and SP values, or
- explicitly latched values for deterministic testing/replay.

Exact cabinet random sequences are hardware-timing-dependent by design.

## Main-loop hook

The N64 game's translated main dispatch loop should call:

```c
wm_rng_mainloop_step(&rng);
```

at the point corresponding to WrestleMania's `mainpok` randomize block.

`RNDRNG0` calls advance the state again independently.

## Callers corrected in this package

### High-score dirty/empty initials

HSTD.ASM does:

```asm
movk 6,a0
calla RNDRNG0
```

The high-score entry module now requests an inclusive `0..6` range directly
instead of taking an arbitrary random integer modulo seven.

### Attract `ONE_BALL`

ATTRACT.ASM calls RNDRNG0 with:

- `0x80000`, then subtracts `0x40000` for X velocity
- `0x60000`, then subtracts `0x30000` for Y velocity
- `[255,0]` for 16.16 Y position
- `[400,0]` for 16.16 X position

The attract operator module now uses `WmRng` directly. Position randomization
is fixed-point across the full range; it no longer chooses only integer
pixels. Positive velocity endpoints are included, matching RNDRNG0.
