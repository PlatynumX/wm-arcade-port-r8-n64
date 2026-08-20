#include "test_roster_common.h"
#include "wm_arcade_yoko.h"
int main(void){wm_arcade_actor_t a={0},o={0};rt_t t={0};wm_arcade_yoko_callbacks_t c=rt_callbacks(&t);const wm_arcade_wrestler_profile_t*p=&wm_arcade_profile_yoko;
assert(p->source_lines==2788&&p->charge_ticks==85&&p->secret_count==9);assert(strcmp(p->secrets[6].source_label,"scissors")==0&&p->secrets[6].max_ticks==32);assert(p->secrets[7].max_ticks==40&&p->secrets[8].max_ticks==50);
a.player_mode=WM_PMODE_NORMAL;o.player_mode=WM_PMODE_NORMAL;a.run_time=9;a.facing_dir=WM_MOVE_RIGHT;assert(wm_arcade_yoko_release_charge(&a,&o,85,&c));assert(a.run_time==0&&strcmp(t.anim,"yok_2_salt_anim")==0);
rt_reset(&t);c=rt_callbacks(&t);a.player_mode=WM_PMODE_NORMAL;assert(wm_arcade_yoko_fire_secret(&a,&o,WM_YOKO_SECRET_SCISSORS,0,&c));assert(strcmp(t.anim,"yok_scissor_anim")==0);
rt_reset(&t);c=rt_callbacks(&t);a.facing_dir=WM_MOVE_RIGHT;assert(wm_arcade_yoko_fire_secret(&a,&o,WM_YOKO_SECRET_GUT_PUSH,0,&c));assert(strcmp(t.anim,"yok_2_gut_push_anim")==0);
puts("Stage 17 Yokozuna direct-port tests: PASS");}
