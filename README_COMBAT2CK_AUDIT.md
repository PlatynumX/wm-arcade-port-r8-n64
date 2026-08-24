# Combat2CL — ANI_CODE native callback translation pass

Combat2CJ proved the full ANIM.ASM VM and service table were live, but most ANI_CODE targets outside ANIM.ASM still fell through to a diagnostic event. Combat2CL translates the stateful native callbacks that directly mutate fighter movement, recovery, attachment, opponent state, ring state, or branch status and are used by the shipped wrestler scripts.

This pass adds source-backed handlers for pause/opponent state, saved/merged/reversed velocity, head-hold timing and button counters, opponent facing/X flip, attachment restoration, dead/ground checks, ring-facing/target helpers, buckoff/get-off velocities, free-toss setup, climbing safety, opponent launch/immobilize/delay state, dead-animation bit, opponent gravity, smart targeting, blocked velocity response, and optimal opponent positioning. It also corrects `no_bk_xvel` to clear both X and Z velocity, matching the original sequence routine.

Audio/commentary-only native callbacks remain event/audio-service boundaries rather than invented gameplay functions. Visual-only callbacks remain presentation boundaries. No collision tuning or presenter-world ownership is reintroduced.
