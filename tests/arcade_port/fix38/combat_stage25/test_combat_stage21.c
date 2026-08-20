#include "test_roster_common.h"
#include "wm_arcade_lex.h"
int main(void){wm_arcade_actor_t a={0},o={0};rt_t t={0};wm_arcade_lex_callbacks_t c=rt_callbacks(&t);const wm_arcade_wrestler_profile_t*p=&wm_arcade_profile_lex;
assert(p->source_lines==2749&&p->charge_ticks==100&&p->secret_count==8);assert(strcmp(p->secrets[6].source_label,"sliding_elbow")==0&&p->secrets[6].max_ticks==30);assert(strcmp(p->secrets[7].source_label,"hammer")==0&&p->secrets[7].max_ticks==32);
a.player_mode=WM_PMODE_NORMAL;o.player_mode=WM_PMODE_NORMAL;a.facing_dir=WM_MOVE_RIGHT;assert(wm_arcade_lex_release_charge(&a,&o,100,&c));assert(strcmp(t.anim,"lex_2_clobber_anim")==0);
rt_reset(&t);c=rt_callbacks(&t);assert(wm_arcade_lex_fire_secret(&a,&o,WM_LEX_SECRET_SLIDING_ELBOW,0,&c));assert(strcmp(t.anim,"lex_sliding_elbow_anim")==0);
rt_reset(&t);c=rt_callbacks(&t);a.but_val_down=WM_BTN_PUNCH;a.closest_xdist=20;a.closest_zdist=20;a.facing_dir=WM_MOVE_RIGHT;wm_arcade_move_lex(&a,&o,NULL,&c);assert(strcmp(t.anim,"lex_2_butt_anim")==0);
puts("Stage 21 Lex direct-port tests: PASS");}
