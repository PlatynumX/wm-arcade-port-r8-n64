#include "wm_arcade_drone_source_bodies.h"
#include "wmania_rng.h"
#include "wm_arcade_drone_source_services.h"
typedef struct WmFix39DroneGeneratedBody { const char *label; wm_arcade_drone_source_service_handler_t handler; } WmFix39DroneGeneratedBody;
static uint32_t wm_fix39_drone_body_rnd(void *user,uint32_t mask){return wm_rng_rnd_mask((WmRng*)user,mask);}
static uint32_t wm_fix39_drone_body_rndrng0(void *user,uint32_t maxv){return wm_rng_rndrng0((WmRng*)user,maxv);}
#include "wm_arcade_drone_source_bodies_generated.h"
int wm_arcade_drone_source_generated_body_count(void){ return WM_FIX39_DRONE_TRANSLATED_BODY_COUNT; }
int wm_arcade_drone_source_install_generated_bodies(void){ int i,n=0; for(i=0;i<WM_FIX39_DRONE_TRANSLATED_BODY_COUNT;i++) n += wm_arcade_drone_source_service_attach(wm_fix39_generated_bodies[i].label,wm_fix39_generated_bodies[i].handler)?1:0; return n; }
