#include "wm_arcade_match_lifecycle.h"
#include <string.h>

static void emit(wm_arcade_match_lifecycle *m,wm_arcade_match_event_t e,int dead_team)
{ if(m && m->config.event) m->config.event(e,dead_team,m->config.user); }

unsigned wm_arcade_get_live_bits(wm_arcade_actor_t **actors,size_t n)
{
    unsigned bits=0u; size_t i;
    if(!actors) return 0u;
    for(i=0;i<n;++i){
        wm_arcade_actor_t *a=actors[i];
        if(!a||!a->active||a->player_side<0||a->player_side>1) continue;
        if(a->player_mode!=WM_PMODE_DEAD || (a->status_flags&WM_STATUS_ZOMBIE)!=0u)
            bits|=1u<<(unsigned)a->player_side;
    }
    return bits;
}

uint16_t wm_arcade_match_timer_step(const wm_arcade_match_lifecycle_config *c)
{
    static const uint16_t tbl[5]={1050u,1275u,1500u,1725u,1950u};
    unsigned adj=3u; uint32_t step;
    if(c && c->adjust_speed>=1u && c->adjust_speed<=5u) adj=c->adjust_speed;
    step=tbl[adj-1u];
    if(c && (c->royal_rumble || (c->pstatus!=0u && c->pstatus!=3u && c->num_opps==3u))){
        step=(step*0xaaaau)>>16;
        if(c->final_match || c->royal_rumble) step>>=1;
    }
    return (uint16_t)step;
}

void wm_arcade_match_lifecycle_init(wm_arcade_match_lifecycle *m,bool attract_wrap)
{
    if(!m)return;
    memset(m,0,sizeof(*m));
    m->tens=9u;m->ones=9u;m->startup_delay=(uint16_t)(2u*WM_ARCADE_TSEC);
    m->pin_timeout=(uint16_t)(4u*WM_ARCADE_TSEC);m->config.adjust_speed=3u;
    m->step=wm_arcade_match_timer_step(&m->config);m->attract_wrap=attract_wrap;
}

void wm_arcade_match_lifecycle_configure(wm_arcade_match_lifecycle *m,const wm_arcade_match_lifecycle_config *c)
{
    if(!m)return;
    if(c)m->config=*c; else { memset(&m->config,0,sizeof(m->config));m->config.adjust_speed=3u; }
    m->step=wm_arcade_match_timer_step(&m->config);
}

bool wm_arcade_match_clock_zero(const wm_arcade_match_lifecycle *m)
{ return m&&m->tens==0u&&m->ones==0u; }

static int should_clear_reduce_bog(const wm_arcade_match_lifecycle *m,unsigned live_bits)
{
    if(m->config.royal_rumble){
        if((live_bits&1u)==0u) return 1;
        return m->config.wrestler_count<1;
    }
    if(m->config.eight_on_one){
        if(((unsigned)m->config.pstatus & live_bits)==0u) return 1;
        return m->config.wrestler_count<1;
    }
    return 1;
}

static void dead_team_tick(wm_arcade_match_lifecycle *m,unsigned live_bits,uint32_t pcnt)
{
    int dead_team=(int)(((live_bits^3u)>>1)&1u);
    if((uint32_t)(pcnt-m->last_dead_pcnt)!=1u){
        m->pin_timeout=(uint16_t)(5u*WM_ARCADE_TSEC);
        if(should_clear_reduce_bog(m,live_bits)){
            emit(m,WM_MATCH_EVENT_CLEAR_REDUCE_BOG,dead_team);
            emit(m,WM_MATCH_EVENT_WAKE_CROWD,dead_team);
        }
        emit(m,WM_MATCH_EVENT_PIN_PROMPT,dead_team);
    }
    m->last_dead_pcnt=pcnt;
    if(m->pin_timeout==0u)return;
    --m->pin_timeout;
    if(m->pin_timeout==0u){
        m->round_award_pending=true;
        emit(m,WM_MATCH_EVENT_ANNOUNCE_ROUND_WINNER,dead_team);
    }
}

static void dec_timer(wm_arcade_match_lifecycle *m)
{
    uint16_t old=m->fraction;
    m->fraction=(uint16_t)(m->fraction-m->step);
    if(old>=m->step)return;
    if(m->ones!=0u)--m->ones;
    else {m->ones=9u;if(m->tens!=0u)--m->tens;if(m->tens==0u)emit(m,WM_MATCH_EVENT_TIMER_RED,-1);}
    if((unsigned)m->tens*10u+(unsigned)m->ones<=10u)emit(m,WM_MATCH_EVENT_TIMER_WARNING,-1);
}

void wm_arcade_match_lifecycle_tick(wm_arcade_match_lifecycle *m,wm_arcade_actor_t **actors,size_t actor_count,uint32_t pcnt)
{
    unsigned bits;
    if(!m||m->halt)return;
    if(m->startup_delay!=0u){--m->startup_delay;return;}
    if(wm_arcade_match_clock_zero(m)){
        if(m->attract_wrap){m->tens=9u;m->ones=9u;m->fraction=0u;emit(m,WM_MATCH_EVENT_TIMER_NORMAL,-1);}
        return;
    }
    bits=wm_arcade_get_live_bits(actors,actor_count);
    if(bits!=3u){dead_team_tick(m,bits,pcnt);return;}
    dec_timer(m);
}
