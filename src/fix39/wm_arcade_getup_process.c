#include "wm_arcade_getup_process.h"
#include <string.h>
void wm_arcade_getup_process_init(wm_arcade_getup_process*p){if(p)memset(p,0,sizeof(*p));}
void wm_arcade_getup_process_attach(wm_arcade_getup_process*p,wm_arcade_actor_t*a,wm_arcade_sound*sound){
    if(!p||!a||p->active||a->getup_time<=0)return;p->active=true;p->initial_getup=a->getup_time;p->display_value=WM_GETUP_SIZE;a->meter_proc=p;
    if(sound)(void)wm_sound_triple_sound(sound,0x0bdu);
}
void wm_arcade_getup_process_tick(wm_arcade_getup_process*p,wm_arcade_actor_t*a){
    int32_t v;if(!p||!a||!p->active)return;if(a->getup_time<=0||a->player_mode==WM_PMODE_DEAD){wm_arcade_getup_process_ditch(p,a);return;}
    if(p->initial_getup<=0)return;v=(WM_GETUP_SIZE*a->getup_time)/p->initial_getup;
    p->display_value=(p->display_value+v)>>1;
}
void wm_arcade_getup_process_ditch(wm_arcade_getup_process*p,wm_arcade_actor_t*a){if(!p)return;p->active=false;p->initial_getup=0;p->display_value=0;if(a&&a->meter_proc==p)a->meter_proc=0;}
