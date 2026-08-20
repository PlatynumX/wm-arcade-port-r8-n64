#include "test_roster_common.h"
#include "wm_arcade_bam.h"
int main(void){wm_arcade_actor_t a={0},o={0};rt_t t={0};wm_arcade_bam_callbacks_t c=rt_callbacks(&t);const wm_arcade_wrestler_profile_t*p=&wm_arcade_profile_bam;
assert(p->source_lines==2937&&p->charge_ticks==85&&p->secret_count==10);assert(strcmp(p->secrets[6].source_label,"jumpkick")==0&&p->secrets[6].max_ticks==32);assert(strcmp(p->secrets[9].source_label,"napalm")==0&&p->secrets[9].max_ticks==50);
a.player_mode=WM_PMODE_NORMAL;o.player_mode=WM_PMODE_NORMAL;a.facing_dir=WM_MOVE_RIGHT;assert(wm_arcade_bam_release_charge(&a,&o,85,&c));assert(strcmp(t.anim,"bam_2_fpunch_anim")==0);
rt_reset(&t);c=rt_callbacks(&t);assert(wm_arcade_bam_fire_secret(&a,&o,WM_BAM_SECRET_JUMPKICK,0,&c));assert(strcmp(t.anim,"bam_4_jumpkick_anim")==0);
rt_reset(&t);c=rt_callbacks(&t);assert(wm_arcade_bam_fire_secret(&a,&o,WM_BAM_SECRET_NAPALM,0,&c));assert(strcmp(t.anim,"bam_2_napalm_anim")==0);
puts("Stage 19 Bam Bam direct-port tests: PASS");}
