#include "test_roster_common.h"
#include "wm_arcade_doink.h"
int main(void){wm_arcade_actor_t a={0},o={0};rt_t t={0};wm_arcade_doink_callbacks_t c=rt_callbacks(&t);const wm_arcade_wrestler_profile_t*p=&wm_arcade_profile_doink;
assert(p->source_lines==4037&&p->charge_ticks==100&&p->secret_count==9);assert(strcmp(p->secrets[5].source_label,"earslap")==0&&p->secrets[5].max_ticks==50);assert(p->secrets[8].step_count==7&&p->secrets[8].max_ticks==60);
a.player_mode=WM_PMODE_NORMAL;o.player_mode=WM_PMODE_NORMAL;a.facing_dir=WM_MOVE_RIGHT;a.new_facing_dir=WM_MOVE_RIGHT;a.stick_val_cur=0;assert(wm_arcade_doink_release_charge(&a,&o,100,&c));assert(strcmp(t.anim,"dnk_2_buzz_anim")==0);
rt_reset(&t);c=rt_callbacks(&t);a.player_mode=WM_PMODE_RUNNING;assert(wm_arcade_doink_release_charge(&a,&o,100,&c));assert(strcmp(t.anim,"dnk_2_buzz2_anim")==0);
rt_reset(&t);c=rt_callbacks(&t);a.player_mode=WM_PMODE_NORMAL;a.combo_count=0;assert(wm_arcade_doink_fire_secret(&a,&o,WM_DOINK_SECRET_BOXING_PNCH,0,&c));assert(strcmp(t.anim,"dnk_2_box_anim")==0);
puts("Stage 20 Doink direct-port tests: PASS");}
