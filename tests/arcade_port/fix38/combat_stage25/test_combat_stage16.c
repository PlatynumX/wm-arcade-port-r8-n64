#include "test_roster_common.h"
#include "wm_arcade_taker.h"
int main(void){wm_arcade_actor_t a={0},o={0};rt_t t={0};wm_arcade_taker_callbacks_t c=rt_callbacks(&t);const wm_arcade_wrestler_profile_t*p=&wm_arcade_profile_taker;
assert(p->source_lines==3266&&p->charge_ticks==110&&p->secret_count==7);assert(strcmp(p->secrets[6].source_label,"tomb_smash")==0&&p->secrets[6].max_ticks==32);
a.player_mode=WM_PMODE_NORMAL;o.player_mode=WM_PMODE_NORMAL;assert(!wm_arcade_taker_release_charge(&a,&o,109,&c));assert(wm_arcade_taker_release_charge(&a,&o,110,&c));assert(strcmp(t.special,"scrt_spirit")==0);
rt_reset(&t);c=rt_callbacks(&t);a.player_mode=WM_PMODE_NORMAL;a.attach_proc=&o;assert(wm_arcade_taker_fire_secret(&a,&o,WM_TAKER_SECRET_TOMB_SMASH,100,&c));assert(a.attach_proc==NULL&&strcmp(t.anim,"und_tombstone_smash_anim")==0&&t.kills==1);
rt_reset(&t);c=rt_callbacks(&t);a.player_mode=WM_PMODE_NORMAL;a.facing_dir=WM_MOVE_RIGHT;a.but_val_down=WM_BTN_PUNCH;a.closest_xdist=40;a.closest_zdist=20;o.player_mode=WM_PMODE_NORMAL;wm_arcade_move_taker(&a,&o,NULL,&c);assert(strcmp(t.anim,"und_2_butt_anim")==0);
puts("Stage 16 Undertaker direct-port tests: PASS");}
