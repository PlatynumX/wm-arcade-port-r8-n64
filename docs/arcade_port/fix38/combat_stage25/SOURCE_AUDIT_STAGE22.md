# Stage 22 source audit — unified dispatch seam (superseded by Stage 23 structure)

Stage 22 introduced the common roster/dispatch API. Stage 23 keeps that API but removes the Stage 16-21 generic six-wrestler behavior implementation.

Bret and Razor remain their dedicated direct modules. Undertaker, Yokozuna, Shawn, Bam Bam, Doink and Lex now also execute dedicated `wm_arcade_<wrestler>.c` modules.

The Stage 22 public integration seam remains valid; its old `wm_arcade_move_remaining_wrestler()` implementation is intentionally gone.
