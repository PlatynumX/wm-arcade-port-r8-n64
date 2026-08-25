#include "wm_arcade_match_lifecycle.h"
#include <string.h>

static unsigned live_bits(wm_arcade_actor_t **actors,size_t n){
    unsigned bits=0,i;
    for(i=0;i<n && i<2u;++i){wm_arcade_actor_t*a=actors[i];if(!a||!a->active)continue;
        if(a->player_mode!=WM_PMODE_DEAD || (a->status_flags&WM_STATUS_ZOMBIE))bits|=(1u<<i);}
    return bits;
}
void wm_arcade_match_lifecycle_init(wm_arcade_match_lifecycle *m,bool attract_wrap){
    if(!m)return;memset(m,0,sizeof(*m));m->tens=9;m->ones=9;m->step=1500u;
    m->startup_delay=(uint16_t)(2u*WM_ARCADE_TSEC);m->pin_timeout=(uint16_t)(4u*WM_ARCADE_TSEC);m->attract_wrap=attract_wrap;
}
bool wm_arcade_match_clock_zero(const wm_arcade_match_lifecycle *m){return m&&m->tens==0u&&m->ones==0u;}
void wm_arcade_match_lifecycle_tick(wm_arcade_match_lifecycle *m,wm_arcade_actor_t **actors,size_t actor_count,uint32_t pcnt){
    unsigned bits;uint16_t old;
    if(!m||m->halt)return;
    if(m->startup_delay){--m->startup_delay;return;}
    if(wm_arcade_match_clock_zero(m)){if(m->attract_wrap){m->tens=9;m->ones=9;m->fraction=0;}else m->halt=true;return;}
    bits=live_bits(actors,actor_count);
    if(bits!=3u){
        if((uint32_t)(pcnt-m->last_dead_pcnt)!=1u)m->pin_timeout=(uint16_t)(5u*WM_ARCADE_TSEC);
        m->last_dead_pcnt=pcnt;
        if(m->pin_timeout){--m->pin_timeout;if(!m->pin_timeout)m->round_award_pending=true;}
        return;
    }
    old=m->fraction;m->fraction=(uint16_t)(m->fraction-m->step);
    if(old>=m->step)return;
    if(m->ones>0u){--m->ones;return;}
    m->ones=9u;if(m->tens>0u)--m->tens;
}
