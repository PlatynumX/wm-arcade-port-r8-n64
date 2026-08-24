# Combat2EH — full Combat2DS visual restore on EG runtime

Base runtime commit: `5d91be9b5ed4f3c6a73cc78dba4bd15740fba4cc`

Full visual generator provenance:
- Combat2DS commit: `1c037c02695ed4c83bb920090435bd50add0e583`
- generator blob: `5e4d7f9b1b6de60d36dce9f0d276f74d9465a205`

This keeps Combat2EG's native 15/15 DRONE runtime and restores the exact
development character generator that produced the hardware-built Combat2DS
4,885-frame wrestler corpus.

Expected streamed wrestler frame counts:
- Bret 0: 589
- Razor 1: 625
- Undertaker 2: 583
- Yokozuna 3: 578
- Shawn 4: 618
- Bam Bam 5: 663
- Doink 6: 614
- Lex 8: 615
- total: 4,885

The strict EC WIMP converter remains the final source-proof authority.
