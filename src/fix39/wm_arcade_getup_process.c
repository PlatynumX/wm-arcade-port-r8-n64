#include "wm_arcade_getup_process.h"
#include <string.h>

static void kill_meter(wm_arcade_getup_process *p,wm_arcade_actor_t *a)
{
    if(!p)return;
    p->active=false;p->phase=WM_GETUP_INACTIVE;p->initial_getup=0;p->display_value=0;
    if(a&&a->meter_proc==p)a->meter_proc=0;
}

void wm_arcade_getup_process_init(wm_arcade_getup_process *p)
{ if(p)memset(p,0,sizeof(*p)); }

void wm_arcade_getup_process_begin(wm_arcade_getup_process *p,wm_arcade_actor_t *a,wm_arcade_actor_t **actors,size_t actor_count,bool drone_meters_on,uint8_t num_opps,bool royal_rumble,wm_arcade_sound *sound)
{
    if(!p||!a)return;
    memset(p,0,sizeof(*p));p->active=true;p->phase=WM_GETUP_INITIAL_SLEEP;p->initial_sleep=2u;
    p->actors=actors;p->actor_count=actor_count;p->drone_meters_on=drone_meters_on;p->num_opps=num_opps;p->royal_rumble=royal_rumble;p->sound=sound;
}

void wm_arcade_getup_process_attach(wm_arcade_getup_process *p,wm_arcade_actor_t *a,wm_arcade_sound *sound)
{ wm_arcade_getup_process_begin(p,a,0,0u,true,0u,false,sound); }

static int eligible(const wm_arcade_getup_process *p,const wm_arcade_actor_t *a)
{
    size_t i;
    if(a->player_type==WM_PTYPE_PLAYER)return 1;
    if(p->actors){
        for(i=0;i<p->actor_count;++i){
            const wm_arcade_actor_t *o=p->actors[i];
            if(!o||!o->active||o==a||o->player_side!=a->player_side)continue;
            if(o->player_type==WM_PTYPE_PLAYER)return 0;
        }
    }
    if(!p->drone_meters_on)return 0;
    if(p->num_opps>=2u)return 0;
    return 1;
}

static void slide_offscreen(wm_arcade_getup_process *p,wm_arcade_actor_t *a)
{
    p->phase=WM_GETUP_OFFSCREEN;p->offscreen_counter=10u;p->initial_getup=0;p->display_value=0;
    if(a)a->delay_meter=WM_GETUP_DELAY_AFTER_SLIDE;
}

static void enter_onscreen(wm_arcade_getup_process *p,wm_arcade_actor_t *a)
{
    p->phase=WM_GETUP_ONSCREEN;p->initial_getup=a->getup_time;p->display_value=WM_GETUP_SIZE;
    p->dufus_countdown=WM_GETUP_DUFUS_TICKS;p->dufus_start_getup=a->getup_time;p->dufus_message_pending=false;
    if(p->sound)(void)wm_sound_triple_sound(p->sound,0x0bdu);
}

static int offscreen_ready(const wm_arcade_actor_t *a)
{
    if(a->who_hit_me&&a->who_hit_me->combo_count!=0)return 0;
    if(a->delay_meter!=0)return 0;
    if(a->player_mode==WM_PMODE_DEAD)return 0;
    if(a->life<=20 && a->getup_time!=WM_FLUNG_TIME)return 0;
    return a->getup_time!=0;
}

void wm_arcade_getup_process_tick(wm_arcade_getup_process *p,wm_arcade_actor_t *a)
{
    int32_t cur,scaled;
    if(!p||!a||!p->active)return;
    if(p->phase==WM_GETUP_INITIAL_SLEEP){
        if(p->initial_sleep!=0u){--p->initial_sleep;if(p->initial_sleep!=0u)return;}
        if(!eligible(p,a)){kill_meter(p,a);return;}
        a->meter_proc=p;slide_offscreen(p,a);return;
    }
    if(p->phase==WM_GETUP_OFFSCREEN){
        /* WRESTLE2.ASM slide_offscr: A11 counts down once from 10.
           After it reaches zero, #update executes every source tick. */
        if(p->offscreen_counter!=0u){--p->offscreen_counter;return;}
        if(offscreen_ready(a))enter_onscreen(p,a);
        return;
    }
    if(p->phase!=WM_GETUP_ONSCREEN)return;
    cur=a->getup_time;
    if(cur>p->initial_getup)p->initial_getup=cur;
    if(p->initial_getup<=0){slide_offscreen(p,a);return;}
    scaled=(WM_GETUP_SIZE*cur)/p->initial_getup;
    if(scaled>p->display_value){p->initial_getup=cur;scaled=cur>0?WM_GETUP_SIZE:0;}
    if(p->dufus_countdown!=0u){
        --p->dufus_countdown;
        if(p->dufus_countdown==0u && p->dufus_start_getup-cur<=175)p->dufus_message_pending=true;
    }
    p->display_value=(p->display_value+scaled)>>1;
    if(scaled==0 || a->player_mode==WM_PMODE_DEAD)slide_offscreen(p,a);
}

void wm_arcade_getup_process_ditch(wm_arcade_getup_process *p,wm_arcade_actor_t *a)
{
    if(!p||!a||!p->active)return;
    if(a->getup_time==0 || a->dizzy!=0)return;
    slide_offscreen(p,a);
}

void wm_arcade_getup_process_kill(wm_arcade_getup_process *p,wm_arcade_actor_t *a)
{ kill_meter(p,a); }

void wm_arcade_inc_getup_time(wm_arcade_actor_t *a,int32_t ticks)
{ if(a&&a->getup_time>=20)a->getup_time+=ticks; }
