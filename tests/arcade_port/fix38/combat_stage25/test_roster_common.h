#ifndef TEST_ROSTER_COMMON_H
#define TEST_ROSTER_COMMON_H
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wm_arcade_roster.h"
typedef struct rt {char anim[96];char sound[96];char special[96];size_t secret_count;int kills;int reversals;int combo_result;} rt_t;
static void rt_anim(wm_arcade_actor_t*a,const char*l,void*u){(void)a;rt_t*t=u;snprintf(t->anim,sizeof t->anim,"%s",l?l:"");}
static void rt_sound(wm_arcade_actor_t*a,const char*l,void*u){(void)a;rt_t*t=u;snprintf(t->sound,sizeof t->sound,"%s",l?l:"");}
static void rt_secret(wm_arcade_actor_t*a,const wm_arcade_input_pattern_t*p,size_t n,void*u){(void)a;(void)p;((rt_t*)u)->secret_count=n;}
static void rt_kill(wm_arcade_actor_t*a,void*u){(void)a;((rt_t*)u)->kills++;}
static void rt_rev(wm_arcade_actor_t*a,void*u){(void)a;((rt_t*)u)->reversals++;}
static int rt_combo(wm_arcade_actor_t*a,void*u){(void)a;return ((rt_t*)u)->combo_result;}
static uintptr_t rt_tok(const char*l,void*u){(void)u;uintptr_t h=1469598103934665603ull;while(l&&*l){h^=(unsigned char)*l++;h*=1099511628211ull;}return h;}
static void rt_special(wm_arcade_actor_t*a,const char*l,void*u){(void)a;rt_t*t=u;snprintf(t->special,sizeof t->special,"%s",l?l:"");}
static wm_arcade_roster_callbacks_t rt_callbacks(rt_t*t){wm_arcade_roster_callbacks_t c;memset(&c,0,sizeof c);c.change_anim_label=rt_anim;c.sound_label=rt_sound;c.check_secret_moves=rt_secret;c.find_and_kill_endless=rt_kill;c.do_reversal=rt_rev;c.do_reversal_message=rt_rev;c.check_combo_go=rt_combo;c.resolve_label_token=rt_tok;c.start_special_label=rt_special;c.user=t;return c;}
static void rt_reset(rt_t*t){memset(t,0,sizeof *t);}
#endif
