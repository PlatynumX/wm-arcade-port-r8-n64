#include "wm_arcade_drone_source_services.h"
#include "wm_arcade_drone_source_services_generated.h"
#include <stddef.h>
static int eqi(const char *a,const char *b){ unsigned char x,y; if(!a||!b)return 0; while(*a&&*b){x=(unsigned char)*a++;y=(unsigned char)*b++;if(x>='A'&&x<='Z')x+=('a'-'A');if(y>='A'&&y<='Z')y+=('a'-'A');if(x!=y)return 0;}return *a==*b; }
bool wm_arcade_drone_source_services_ready(void){ return WM_FIX39_DRONE_SERVICES_GENERATED != 0 && WM_FIX39_DRONE_SERVICE_COUNT > 0; }
int wm_arcade_drone_source_service_id(const char *label){ int i; if(!label)return -1; for(i=0;i<WM_FIX39_DRONE_SERVICE_COUNT;i++) if(eqi(label,wm_fix39_drone_service_labels[i])) return i; return -1; }
uint32_t wm_arcade_drone_source_service_addr(const char *label){ int id=wm_arcade_drone_source_service_id(label); return id>=0 ? wm_fix39_drone_service_source_addr[id] : 0u; }
int wm_arcade_drone_source_service_kind(const char *label){ int id=wm_arcade_drone_source_service_id(label); return id>=0 ? (int)wm_fix39_drone_service_kind[id] : -1; }
int wm_arcade_drone_source_service_dispatch(wm_arcade_actor_t *self,const char *label,void *user){ (void)self;(void)user; /* C5d attaches exact historical entry addresses/kinds to every seam. Translated execution bodies land next; remain fail-closed until a body is present. */ return wm_arcade_drone_source_service_id(label) >= 0 ? 0 : 0; }
