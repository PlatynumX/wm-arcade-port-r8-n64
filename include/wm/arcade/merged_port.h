#ifndef WM_ARCADE_MERGED_PORT_H
#define WM_ARCADE_MERGED_PORT_H

/* Fix38 cumulative direct-port surfaces. */
#include "wm/arcade/wmania_rng.h"
#include "wm/arcade/wmania_hiscore_system.h"
#include "wm/arcade/wmania_hiscore_adapter.h"
#include "wm/arcade/wmania_attract_adapter.h"
#include "wm/arcade/wmania_ring_geometry.h"
#include "wm/arcade/wmania_rope_command.h"
#include "wm/arcade/wmania_rope_spawn.h"
#include "wm/arcade/wmania_rope_runtime.h"
#include "wm/arcade/wmania_rope_source_data.h"
#include "wm/arcade/wmania_ring_climb.h"
#include "wm/arcade/wmania_ring_out.h"
#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wm_arcade_wrestler_port.h"
#include "wm/arcade/wm_arcade_special.h"
#include "wm/arcade/wm_arcade_drone.h"

#define WM_FIX38_COMBAT_STAGE 25
#define WM_FIX38_RING_CHUNK 4
#define WM_FIX38_HAS_EXACT_RNDRNG_FAMILY 1
#define WM_FIX38_HAS_HISCORE_SYSTEM 1
#define WM_FIX38_HAS_NONGAMEPLAY_ATTRACT 1

#endif
