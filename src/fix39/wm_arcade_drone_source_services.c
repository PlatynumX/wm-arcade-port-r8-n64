#include "wm_arcade_drone_source_services.h"
#include "wm_arcade_drone_source_services_generated.h"
#include <stddef.h>
#include <string.h>
static int eqi(const char *a,const char *b){ unsigned char x,y; if(!a||!b)return 0; while(*a&&*b){x=(unsigned char)*a++;y=(unsigned char)*b++;if(x>='A'&&x<='Z')x+=('a'-'A');if(y>='A'&&y<='Z')y+=('a'-'A');if(x!=y)return 0;}return *a==*b; }
bool wm_arcade_drone_source_services_ready(void){ return WM_FIX39_DRONE_SERVICES_GENERATED != 0 && WM_FIX39_DRONE_SERVICE_COUNT > 0; }
int wm_arcade_drone_source_service_id(const char *label){ int i; if(!label)return -1; for(i=0;i<WM_FIX39_DRONE_SERVICE_COUNT;i++) if(eqi(label,wm_fix39_drone_service_labels[i])) return i; return -1; }
uint32_t wm_arcade_drone_source_service_addr(const char *label){ int id=wm_arcade_drone_source_service_id(label); return id>=0 ? wm_fix39_drone_service_source_addr[id] : 0u; }
int wm_arcade_drone_source_service_kind(const char *label){ int id=wm_arcade_drone_source_service_id(label); return id>=0 ? (int)wm_fix39_drone_service_kind[id] : -1; }
static wm_arcade_drone_source_service_handler_t wm_fix39_drone_service_handlers[WM_FIX39_DRONE_SERVICE_COUNT > 0 ? WM_FIX39_DRONE_SERVICE_COUNT : 1];
void wm_arcade_drone_source_service_reset_handlers(void){ memset(wm_fix39_drone_service_handlers,0,sizeof(wm_fix39_drone_service_handlers)); }
int wm_arcade_drone_source_service_attach(const char *label,wm_arcade_drone_source_service_handler_t handler){ int id=wm_arcade_drone_source_service_id(label); if(id<0||!handler)return 0; wm_fix39_drone_service_handlers[id]=handler; return 1; }
int wm_arcade_drone_source_service_body_ready(const char *label){ int id=wm_arcade_drone_source_service_id(label); return id>=0 && wm_fix39_drone_service_handlers[id]!=0; }
int wm_arcade_drone_source_service_dispatch(wm_arcade_actor_t *self,wm_arcade_actor_t *opp,wm_arcade_drone_state_t *drone,const char *label,void *user){ int id=wm_arcade_drone_source_service_id(label); if(id<0||!drone||!wm_fix39_drone_service_handlers[id]) return 0; return wm_fix39_drone_service_handlers[id](self,opp,drone,user) ? 1 : 0; }
