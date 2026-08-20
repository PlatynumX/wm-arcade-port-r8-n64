#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wm_arcade_attach_anim.h"
#define FX16(x) ((int32_t)((x)<<16))
int main(void){wm_arcade_actor_t a,b,c;memset(&a,0,sizeof(a));memset(&b,0,sizeof(b));memset(&c,0,sizeof(c));
 a.who_i_hit=&b;assert(wm_arcade_anim_set_attach_from_whoihit(&a)==WM_ATTACH_OK&&a.attach_proc==&b&&b.attach_proc==&a);
 b.player_mode=WM_PMODE_PUPPET;assert(wm_arcade_anim_detach(&a)==WM_ATTACH_OK&&a.attach_proc==NULL&&b.attach_proc==NULL&&b.player_mode==WM_PMODE_ONGROUND);
 a.attach_proc=&b;b.attach_proc=&c;b.anim_mode=0;assert(wm_arcade_anim_set_opp_mode_bits(&a,WM_MODE_GHOST)==WM_ATTACH_OK&&(b.anim_mode&WM_MODE_GHOST));assert(wm_arcade_anim_clear_opp_mode_bits(&a,WM_MODE_GHOST)==WM_ATTACH_OK&&!(b.anim_mode&WM_MODE_GHOST));
 b.attach_proc=&a;b.player_mode=WM_PMODE_DEAD;assert(wm_arcade_anim_set_opp_player_mode(&a,WM_PMODE_NORMAL)==WM_ATTACH_OK&&b.player_mode==WM_PMODE_DEAD);b.player_mode=WM_PMODE_NORMAL;b.obj_control=0;assert(wm_arcade_anim_xflip_opp(&a)==WM_ATTACH_OK&&(b.obj_control&WM_OBJ_FLIPH));
 a.facing_dir=WM_MOVE_RIGHT|WM_MOVE_DOWN;assert(wm_arcade_anim_set_opp_vels(&a,FX16(2),FX16(3),FX16(4))==WM_ATTACH_OK&&b.x_vel==FX16(2)&&b.y_vel==FX16(3)&&b.z_vel==FX16(4));
 memset(&a,0,sizeof(a));a.x_vel=1;a.y_vel=2;a.z_vel=3;a.status_flags=WM_STATUS_KOD|WM_STATUS_DEAD_ANIM|WM_STATUS_PUSH;a.ptime=9;wm_arcade_anim_enter_slave_idle(&a);assert(a.x_vel==0&&a.y_vel==0&&a.z_vel==0&&a.ani_speed==0x100&&(a.anim_mode&WM_MODE_END)&&!(a.status_flags&WM_STATUS_KOD)&&a.ptime==1);
 memset(&a,0,sizeof(a));memset(&b,0,sizeof(b));a.attach_proc=&b;b.attach_proc=&a;a.x_fixed=FX16(100);a.y_fixed=FX16(50);a.z_fixed=FX16(7);a.attach_xoff=6;a.attach_yoff=8;a.attach_zoff=3;a.facing_dir=WM_MOVE_RIGHT;assert(wm_arcade_master_keep_attached(&a)==WM_ATTACH_OK);assert(b.x_fixed==FX16(106)&&b.y_fixed==FX16(58)&&b.z_fixed==FX16(10));
 b.attach_xoff=4;b.attach_yoff=5;b.attach_zoff=2;b.facing_dir=0;b.x_fixed=FX16(80);b.y_fixed=FX16(20);b.z_fixed=FX16(9);assert(wm_arcade_keep_attached(&a)==WM_ATTACH_OK);assert(a.x_fixed==FX16(76)&&a.y_fixed==FX16(25)&&a.z_fixed==FX16(11));
 puts("Stage 12 attachment/animation integration tests: PASS");return 0;}
