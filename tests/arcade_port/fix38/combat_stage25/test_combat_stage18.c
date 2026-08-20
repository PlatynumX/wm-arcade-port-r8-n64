#include "test_roster_common.h"
#include "wm_arcade_shawn.h"
int main(void){wm_arcade_actor_t a={0},o={0};rt_t t={0};wm_arcade_shawn_callbacks_t c=rt_callbacks(&t);const wm_arcade_wrestler_profile_t*p=&wm_arcade_profile_shawn;
assert(p->source_lines==3329&&p->charge_button==WM_BTN_SKICK&&p->charge_ticks==85&&p->secret_count==8);assert(p->secrets[6].max_ticks==32&&strcmp(p->secrets[6].source_label,"frankensteiner")==0);
a.player_mode=WM_PMODE_NORMAL;o.player_mode=WM_PMODE_NORMAL;assert(wm_arcade_shawn_release_charge(&a,&o,85,&c));assert(a.player_mode==WM_PMODE_INAIR&&a.x_vel==1&&strcmp(t.anim,"shn_flying_kick_anim")==0);
rt_reset(&t);c=rt_callbacks(&t);a.player_mode=WM_PMODE_NORMAL;assert(wm_arcade_shawn_fire_secret(&a,&o,WM_SHAWN_SECRET_FRANKENSTEINER,0,&c));assert(strcmp(t.anim,"shn_fstein_anim")==0);
rt_reset(&t);c=rt_callbacks(&t);a.player_mode=WM_PMODE_ONTURNBKL;a.but_val_down=WM_BTN_KICK;wm_arcade_move_shawn(&a,&o,NULL,&c);assert(a.player_mode==WM_PMODE_INAIR&&strcmp(t.anim,"shn_bstomp_anim")==0);
puts("Stage 18 Shawn direct-port tests: PASS");}
