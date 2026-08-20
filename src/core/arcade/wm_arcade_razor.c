#include "wm/arcade/wm_arcade_razor.h"
#include "wm/arcade/wm_arcade_attach_anim.h"
#include "wm/arcade/wm_arcade_damage.h"

/* Exact secret input records from RAZOR.ASM. */
static const wm_arcade_razor_sequence_step_t sec_neck[] = {
    { WM_B_SPUNCH, (uint16_t)(WM_J_REAL_LR|WM_J_TOWARD|WM_J_AWAY|WM_J_UP) },
    { WM_J_TOWARD, WM_J_REAL_LR }, { WM_J_TOWARD, WM_J_REAL_LR }
};
static const wm_arcade_razor_sequence_step_t sec_grab_fling[] = {
    { WM_B_SPUNCH, WM_J_ALL }, { WM_J_AWAY, WM_J_REAL_LR }, { WM_J_AWAY, WM_J_REAL_LR }
};
static const wm_arcade_razor_sequence_step_t sec_hip_toss[] = {
    { WM_B_PUNCH, WM_J_ALL }, { WM_J_AWAY, WM_J_REAL_LR }, { WM_J_AWAY, WM_J_REAL_LR }
};
static const wm_arcade_razor_sequence_step_t sec_grab_fling2[] = {
    { (uint16_t)(WM_B_SPUNCH|WM_J_AWAY), (uint16_t)(WM_J_REAL_LR|WM_J_UP|WM_J_DOWN) }
};
static const wm_arcade_razor_sequence_step_t sec_hip_toss2[] = {
    { (uint16_t)(WM_B_PUNCH|WM_J_AWAY), (uint16_t)(WM_J_REAL_LR|WM_J_UP|WM_J_DOWN) }
};
static const wm_arcade_razor_sequence_step_t sec_down_slash[] = {
    { WM_B_PUNCH, WM_J_ALL }, { WM_J_TOWARD, WM_J_REAL_LR },
    { (uint16_t)(WM_J_DOWN|WM_J_TOWARD), WM_J_REAL_LR }, { WM_J_DOWN, WM_J_REAL_LR }
};
const wm_arcade_razor_secret_pattern_t wm_arcade_razor_secret_patterns[6] = {
    { WM_RZR_SECRET_NECK_GRAB, sec_neck, 3, 30 },
    { WM_RZR_SECRET_GRAB_FLING, sec_grab_fling, 3, 32 },
    { WM_RZR_SECRET_HIP_TOSS, sec_hip_toss, 3, 32 },
    { WM_RZR_SECRET_GRAB_FLING2, sec_grab_fling2, 1, 10 },
    { WM_RZR_SECRET_HIP_TOSS2, sec_hip_toss2, 1, 10 },
    { WM_RZR_SECRET_DOWN_SLASH, sec_down_slash, 4, 50 }
};

static const wm_arcade_razor_sequence_step_t mon_pile[] = {
    { WM_J_DOWN, 0 }, { WM_J_DOWN, 0 }, { WM_B_SKICK, WM_J_ALL }
};
static const wm_arcade_razor_sequence_step_t mon_combo1[] = {
    { WM_J_TOWARD, 0 }, { WM_J_TOWARD, 0 }, { WM_B_SPUNCH, WM_J_ALL }
};
static const wm_arcade_razor_sequence_step_t mon_edge[] = {
    { WM_J_TOWARD, 0 }, { WM_J_TOWARD, 0 }, { WM_B_SPUNCH, WM_J_ALL }
};
static const wm_arcade_razor_sequence_step_t mon_rug[] = {
    { WM_J_DOWN, 0 }, { WM_J_DOWN, 0 }, { WM_B_KICK, WM_J_ALL }
};
static const wm_arcade_razor_sequence_step_t mon_grab_air[] = {
    { WM_J_AWAY, 0 }, { WM_J_AWAY, 0 }, { WM_B_PUNCH, WM_J_ALL }
};
static const wm_arcade_razor_sequence_step_t mon_combo2[] = {
    { WM_J_TOWARD, 0 }, { WM_J_TOWARD, 0 }, { WM_B_KICK, WM_J_ALL }
};
static const wm_arcade_razor_sequence_step_t mon_sliding_rug[] = {
    { WM_J_TOWARD, 0 }, { WM_J_TOWARD, 0 }, { WM_B_KICK, WM_J_ALL }
};
static const wm_arcade_razor_sequence_step_t mon_finish1[] = {
    { WM_J_UP, 0 }, { WM_J_DOWN, 0 },
    { WM_J_TOWARD, (uint16_t)(WM_J_DOWN|WM_J_UP) },
    { WM_J_TOWARD, (uint16_t)(WM_J_DOWN|WM_J_UP) }, { WM_B_PUNCH, WM_J_ALL }
};
static const wm_arcade_razor_sequence_step_t mon_finish2[] = {
    { WM_J_UP, 0 }, { WM_J_UP, 0 }, { WM_J_RIGHT, WM_J_UP },
    { WM_J_RIGHT, WM_J_UP }, { WM_B_SPUNCH, WM_J_ALL }
};
const wm_arcade_razor_monitor_pattern_t wm_arcade_razor_monitor_patterns[9] = {
    { WM_RZR_MON_HEADHOLD_PILE, mon_pile, 3, 60 },
    { WM_RZR_MON_HEADHOLD_COMBO1, mon_combo1, 3, 60 },
    { WM_RZR_MON_HEADHOLD_EDGE, mon_edge, 3, 60 },
    { WM_RZR_MON_HEADHOLD_RUG, mon_rug, 3, 60 },
    { WM_RZR_MON_GRAB_TOSS_AIR, mon_grab_air, 3, 40 },
    { WM_RZR_MON_HEADHOLD_COMBO2, mon_combo2, 3, 60 },
    { WM_RZR_MON_SLIDING_RUG, mon_sliding_rug, 3, 60 },
    { WM_RZR_MON_FINISH1, mon_finish1, 5, 60 },
    { WM_RZR_MON_FINISH2, mon_finish2, 5, 60 }
};

static void anim(wm_arcade_actor_t *a, wm_arcade_razor_anim_id_t id,
                 const wm_arcade_razor_callbacks_t *cb)
{ if (cb && cb->change_anim) cb->change_anim(a,id,cb->user); }
static void snd(wm_arcade_actor_t *a, wm_arcade_razor_sound_id_t id,
                const wm_arcade_razor_callbacks_t *cb)
{ if (cb && cb->sound) cb->sound(a,id,cb->user); }
static int setmode(wm_arcade_actor_t *a,uint16_t m)
{ if(!a||a->player_mode==WM_PMODE_DEAD)return 0;a->player_mode=m;return 1; }
static int face2(const wm_arcade_actor_t *a){return a&&(a->facing_dir&WM_MOVE_UP);}
static wm_arcade_razor_anim_id_t f24(const wm_arcade_actor_t*a,wm_arcade_razor_anim_id_t i2,wm_arcade_razor_anim_id_t i4)
{ return face2(a)?i2:i4; }
static int near(const wm_arcade_actor_t*a,int x,int z)
{ return a&&a->closest_xdist<=x&&a->closest_zdist<=z; }
static int groundish(const wm_arcade_actor_t*o)
{ return o&&(o->player_mode==WM_PMODE_ONGROUND||o->player_mode==WM_PMODE_DEAD); }

static const wm_arcade_razor_action_id_t action_table[32]={
 WM_RZR_ACT_NONE,WM_RZR_ACT_PUNCH,WM_RZR_ACT_BLOCK,WM_RZR_ACT_BLOCK,
 WM_RZR_ACT_SUPER_PUNCH,WM_RZR_ACT_SUPER_PUNCH,WM_RZR_ACT_BLOCK,WM_RZR_ACT_BLOCK,
 WM_RZR_ACT_KICK,WM_RZR_ACT_PUNCHKICK,WM_RZR_ACT_BLOCK,WM_RZR_ACT_BLOCK,
 WM_RZR_ACT_SUPER_PUNCH,WM_RZR_ACT_PUNCHKICK,WM_RZR_ACT_BLOCK,WM_RZR_ACT_BLOCK,
 WM_RZR_ACT_SUPER_KICK,WM_RZR_ACT_SUPER_KICK,WM_RZR_ACT_BLOCK,WM_RZR_ACT_BLOCK,
 WM_RZR_ACT_GRABOH,WM_RZR_ACT_GRABOH,WM_RZR_ACT_BLOCK,WM_RZR_ACT_BLOCK,
 WM_RZR_ACT_SUPER_KICK,WM_RZR_ACT_PUNCHKICK,WM_RZR_ACT_BLOCK,WM_RZR_ACT_BLOCK,
 WM_RZR_ACT_GRABOH,WM_RZR_ACT_GRABOH,WM_RZR_ACT_BLOCK,WM_RZR_ACT_BLOCK
};

static int do_block(wm_arcade_actor_t*a,const wm_arcade_razor_env_t*e,const wm_arcade_razor_callbacks_t*cb)
{
 if(e&&e->blocking_off)return 0;
 if(cb&&cb->round_award_block)cb->round_award_block(a,cb->user);
 anim(a,f24(a,WM_RZR_ANIM_BLOCK2,WM_RZR_ANIM_BLOCK4),cb);snd(a,WM_RZR_SND_BLOCK_WOOSH,cb);a->block_time=0;return 1;
}
static void std_punch(wm_arcade_actor_t*a,const wm_arcade_razor_callbacks_t*cb)
{anim(a,f24(a,WM_RZR_ANIM_PUNCH2,WM_RZR_ANIM_PUNCH4),cb);snd(a,WM_RZR_SND_PUNCH,cb);}
static void do_punch(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_razor_callbacks_t*cb)
{
 if(o&&(o->player_mode==WM_PMODE_ONTURNBKL||o->player_mode==WM_PMODE_CLIMBTURNBKL)){std_punch(a,cb);return;}
 if(groundish(o)){if(near(a,160,140)){anim(a,f24(a,WM_RZR_ANIM_GROUND_PUNCH2,WM_RZR_ANIM_GROUND_PUNCH4),cb);snd(a,WM_RZR_SND_LBOWDROP,cb);}else std_punch(a,cb);return;}
 if(near(a,44,45)){anim(a,f24(a,WM_RZR_ANIM_BUTT2,WM_RZR_ANIM_BUTT4),cb);snd(a,WM_RZR_SND_HDBUTT,cb);}else std_punch(a,cb);
}
static void ground_spunch(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_razor_callbacks_t*cb)
{
 int32_t dx;
 if(!o||o->player_mode==WM_PMODE_DEAD)goto fallback;
 dx=a->x_fixed-o->x_fixed;if(dx<0)dx=-dx;dx>>=16;if(dx<0x30)goto fallback;
 if((a->obj_control&WM_OBJ_FLIPH)!=(o->obj_control&WM_OBJ_FLIPH)){
  anim(a,f24(a,WM_RZR_ANIM_HAIR_PICKUP2,WM_RZR_ANIM_HAIR_PICKUP4),cb);snd(a,WM_RZR_SND_LBOWDROP,cb);return;
 }
 if(dx>=0x40){a->status_flags|=WM_STATUS_SMART_ATTACK;a->smart_target=o;anim(a,WM_RZR_ANIM_RUGSHAKE,cb);return;}
fallback:
 anim(a,f24(a,WM_RZR_ANIM_GROUND_PUNCH2,WM_RZR_ANIM_GROUND_PUNCH4),cb);snd(a,WM_RZR_SND_LBOWDROP,cb);
}
static void do_super_punch(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_razor_callbacks_t*cb)
{
 uint16_t m=o?o->player_mode:WM_PMODE_NORMAL;
 if(m==WM_PMODE_RUNNING||m==WM_PMODE_BOUNCING||m==WM_PMODE_INAIR||m==WM_PMODE_INAIR2){anim(a,WM_RZR_ANIM_DSLASH3,cb);snd(a,WM_RZR_SND_UPRCUT,cb);return;}
 if(groundish(o)){if(near(a,160,140))ground_spunch(a,o,cb);else std_punch(a,cb);return;}
 if(near(a,85,45)){
  if(a->stick_val_cur&WM_MOVE_DOWN){anim(a,WM_RZR_ANIM_UPPERCUT4,cb);snd(a,WM_RZR_SND_UPRCUT,cb);return;}
  if(a->closest_xdist>65){std_punch(a,cb);return;}
  anim(a,f24(a,WM_RZR_ANIM_PUMMEL2,WM_RZR_ANIM_PUMMEL4),cb);snd(a,WM_RZR_SND_HDBUTT,cb);return;
 }
 anim(a,WM_RZR_ANIM_USLASH3,cb);snd(a,WM_RZR_SND_UPRCUT,cb);
}
static void std_kick(wm_arcade_actor_t*a,const wm_arcade_razor_callbacks_t*cb)
{anim(a,f24(a,WM_RZR_ANIM_KICK2,WM_RZR_ANIM_KICK4),cb);snd(a,WM_RZR_SND_KICK,cb);}
static void do_kick(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_razor_callbacks_t*cb)
{
 uint16_t m=o?o->player_mode:WM_PMODE_NORMAL;
 if(m==WM_PMODE_INAIR2){anim(a,WM_RZR_ANIM_KICK_TB,cb);snd(a,WM_RZR_SND_KICK,cb);return;}
 if(m==WM_PMODE_ONTURNBKL||m==WM_PMODE_CLIMBTURNBKL){std_kick(a,cb);return;}
 if(groundish(o)){if(near(a,160,140))anim(a,f24(a,WM_RZR_ANIM_STOMP2,WM_RZR_ANIM_STOMP4),cb);else{std_kick(a,cb);return;}snd(a,WM_RZR_SND_KICK,cb);return;}
 if(near(a,70,50))anim(a,f24(a,WM_RZR_ANIM_KNEE2,WM_RZR_ANIM_KNEE4),cb);else anim(a,f24(a,WM_RZR_ANIM_KICK2,WM_RZR_ANIM_KICK4),cb);snd(a,WM_RZR_SND_KICK,cb);
}
static void do_super_kick(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_razor_callbacks_t*cb)
{
 uint16_t m=o?o->player_mode:WM_PMODE_NORMAL;int closez=60;
 if(m==WM_PMODE_RUNNING||m==WM_PMODE_BOUNCING){anim(a,WM_RZR_ANIM_BIGBOOT4,cb);snd(a,WM_RZR_SND_FLYKICK,cb);return;}
 if(m==WM_PMODE_INAIR2){anim(a,WM_RZR_ANIM_KICK_TB,cb);snd(a,WM_RZR_SND_KICK,cb);return;}
 if(m==WM_PMODE_HEADHELD){std_kick(a,cb);return;}
 if(groundish(o)){if(near(a,160,140)){anim(a,f24(a,WM_RZR_ANIM_STOMP2,WM_RZR_ANIM_STOMP4),cb);snd(a,WM_RZR_SND_KICK,cb);}else std_kick(a,cb);return;}
 if(m==WM_PMODE_ONTURNBKL||m==WM_PMODE_CLIMBTURNBKL){if(near(a,70,96)){anim(a,f24(a,WM_RZR_ANIM_SUPER_KICK2,WM_RZR_ANIM_SUPER_KICK4),cb);snd(a,WM_RZR_SND_FLYKICK,cb);}else std_kick(a,cb);return;}
 if(m==WM_PMODE_WAITANIM||m==WM_PMODE_GRAPPLE||m==WM_PMODE_MASTER||m==WM_PMODE_SLAVE||m==WM_PMODE_HEADHOLD||m==WM_PMODE_CHOKEHOLD||m==WM_PMODE_PUPPET||m==WM_PMODE_PUPPET2)closez=62;
 if(!near(a,70,closez)){anim(a,f24(a,WM_RZR_ANIM_SUPER_KICK2,WM_RZR_ANIM_SUPER_KICK4),cb);snd(a,WM_RZR_SND_FLYKICK,cb);return;}
 if(a->stick_val_cur==(uint16_t)(a->new_facing_dir&0x0c)){anim(a,WM_RZR_ANIM_KNEE_FALL4,cb);snd(a,WM_RZR_SND_GRABHOLD,cb);}else{anim(a,f24(a,WM_RZR_ANIM_KNEE2,WM_RZR_ANIM_KNEE4),cb);snd(a,WM_RZR_SND_KICK,cb);}
}
static void normal_action(wm_arcade_actor_t*a,wm_arcade_actor_t*o,wm_arcade_razor_action_id_t ac,const wm_arcade_razor_env_t*e,const wm_arcade_razor_callbacks_t*cb)
{
 switch(ac){case WM_RZR_ACT_PUNCH:do_punch(a,o,cb);break;case WM_RZR_ACT_BLOCK:(void)do_block(a,e,cb);break;case WM_RZR_ACT_SUPER_PUNCH:case WM_RZR_ACT_GRABOH:do_super_punch(a,o,cb);break;case WM_RZR_ACT_KICK:do_kick(a,o,cb);break;case WM_RZR_ACT_PUNCHKICK:anim(a,WM_RZR_ANIM_START_RUN,cb);break;case WM_RZR_ACT_SUPER_KICK:do_super_kick(a,o,cb);break;default:break;}
}

void wm_arcade_razor_ani_init(wm_arcade_actor_t*a,const wm_arcade_razor_callbacks_t*cb)
{
 if(!a)return;
 if(a->facing_dir&WM_MOVE_RIGHT){anim(a,WM_RZR_ANIM_STAND2,cb);if(cb&&cb->change_torso_anim)cb->change_torso_anim(a,WM_RZR_ANIM_TORSO2,cb->user);}else{anim(a,WM_RZR_ANIM_STAND4,cb);if(cb&&cb->change_torso_anim)cb->change_torso_anim(a,WM_RZR_ANIM_TORSO4,cb->user);}
}
static wm_arcade_razor_step_result_t mode_normal(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_razor_env_t*e,const wm_arcade_razor_callbacks_t*cb)
{
 if(a->anim_mode&WM_MODE_UNINT)return WM_RZR_STEP_IDLE;
 if(a->i_will_die&&!a->immobilize_time){anim(a,WM_RZR_ANIM_FALL_BACK,cb);if(cb&&cb->adjust_health)cb->adjust_health(a,-10,cb->user);setmode(a,WM_PMODE_DEAD);a->i_will_die=0;return WM_RZR_STEP_ACTION;}
 if(o&&o->player_mode==WM_PMODE_DEAD&&!(o->status_flags&WM_STATUS_ZOMBIE)){
  int reciprocal=a->attach_proc&&a->attach_proc->attach_proc==a;
  if(!reciprocal){if((cb&&cb->teammate_pin&&cb->teammate_pin(a,cb->user))||(cb&&cb->raisearm_check&&cb->raisearm_check(a,cb->user))){anim(a,f24(a,WM_RZR_ANIM_RAISE_ARM2,WM_RZR_ANIM_RAISE_ARM4),cb);if(cb&&cb->set_raisearm_bit)cb->set_raisearm_bit(a,cb->user);if(cb&&cb->drone_change_back)cb->drone_change_back(a,cb->user);return WM_RZR_STEP_ACTION;}if(a->but_val_cur&&cb&&cb->can_pin&&cb->can_pin(a,o,cb->user)){anim(a,f24(a,WM_RZR_ANIM_PIN2,WM_RZR_ANIM_PIN4),cb);a->status_flags|=WM_STATUS_DID_PIN;if(cb->drone_change_back)cb->drone_change_back(a,cb->user);return WM_RZR_STEP_ACTION;}}
 }
 if(a->immobilize_time){a->move_dir=0;if(cb&&cb->execute_walk)cb->execute_walk(a,cb->user);return WM_RZR_STEP_EXTERNAL;}
 if(a->but_val_cur&WM_BTN_BLOCK){if(do_block(a,e,cb)){a->attack_type=0;return WM_RZR_STEP_ACTION;}}
 if((a->but_val_cur&WM_BTN_ATTACK_MASK)==(WM_BTN_PUNCH|WM_BTN_KICK)){normal_action(a,o,WM_RZR_ACT_PUNCHKICK,e,cb);return WM_RZR_STEP_ACTION;}
 normal_action(a,o,action_table[a->but_val_down&WM_BTN_ATTACK_MASK],e,cb);
 if(a->anim_mode&WM_MODE_UNINT)return WM_RZR_STEP_ACTION;
 a->move_dir=a->stick_val_cur;
 if(cb&&cb->climb_turnbuckle&&cb->climb_turnbuckle(a,cb->user)){if(cb->jump_rope_audio)cb->jump_rope_audio(a,cb->user);return WM_RZR_STEP_EXTERNAL;}
 if(cb&&cb->execute_walk)cb->execute_walk(a,cb->user);
 return WM_RZR_STEP_ACTION;
}
static void fly_elbow(wm_arcade_actor_t*a,const wm_arcade_razor_callbacks_t*cb)
{anim(a,WM_RZR_ANIM_FLYING_ELBOW,cb);a->x_vel>>=1;snd(a,WM_RZR_SND_FLYKICK,cb);}
static wm_arcade_razor_step_result_t mode_running(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_razor_env_t*e,const wm_arcade_razor_callbacks_t*cb)
{
 wm_arcade_razor_action_id_t ac;a->run_time++;
 if(!a->usr_var1){int32_t v=WM_RZR_XRUN;if(cb&&cb->bounce_off_ropes)cb->bounce_off_ropes(a,cb->user);if(e&&e->hyper_speed_on>0&&e->hyper_speed_on<15)v<<=e->hyper_speed_on;if(!(a->move_dir&WM_MOVE_RIGHT))v=-v;a->x_vel=v;}
 if(a->getup_time)return WM_RZR_STEP_IDLE;
 if(!(a->anim_mode&WM_MODE_UNINT)&&(((a->stick_val_cur|a->move_dir)&(WM_MOVE_LEFT|WM_MOVE_RIGHT))==(WM_MOVE_LEFT|WM_MOVE_RIGHT)))setmode(a,WM_PMODE_NORMAL);
 if(a->stick_val_cur&WM_MOVE_UP)a->z_vel=-WM_RZR_ZDRIFT;else if(a->stick_val_cur&WM_MOVE_DOWN)a->z_vel=WM_RZR_ZDRIFT;else a->z_vel=0;if(a->delay_butns)return WM_RZR_STEP_IDLE;
 ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];
 if(ac==WM_RZR_ACT_BLOCK){a->x_vel>>=1;setmode(a,WM_PMODE_NORMAL);(void)do_block(a,e,cb);return WM_RZR_STEP_ACTION;}
 if(ac==WM_RZR_ACT_KICK||ac==WM_RZR_ACT_SUPER_KICK){if(groundish(o)){fly_elbow(a,cb);return WM_RZR_STEP_ACTION;}if(cb&&cb->ck_ignore&&cb->ck_ignore(a,cb->user))return WM_RZR_STEP_IDLE;anim(a,WM_RZR_ANIM_FLYING_KICK,cb);snd(a,WM_RZR_SND_FLYKICK,cb);setmode(a,WM_PMODE_INAIR);return WM_RZR_STEP_ACTION;}
 if(ac==WM_RZR_ACT_PUNCH||ac==WM_RZR_ACT_SUPER_PUNCH||ac==WM_RZR_ACT_PUNCHKICK||ac==WM_RZR_ACT_GRABOH){if(groundish(o)){if(near(a,176,176)){fly_elbow(a,cb);return WM_RZR_STEP_ACTION;}return WM_RZR_STEP_IDLE;}if((a->facing_dir&a->new_facing_dir&(WM_MOVE_LEFT|WM_MOVE_RIGHT))==0)return WM_RZR_STEP_IDLE;a->run_time=0;setmode(a,WM_PMODE_NORMAL);if(a->closest_xdist<70){anim(a,f24(a,WM_RZR_ANIM_BUTT2,WM_RZR_ANIM_BUTT4),cb);snd(a,WM_RZR_SND_HDBUTT,cb);}else{anim(a,WM_RZR_ANIM_USLASH3,cb);snd(a,WM_RZR_SND_GRABHOLD,cb);}return WM_RZR_STEP_ACTION;}
 return WM_RZR_STEP_IDLE;
}
static wm_arcade_razor_step_result_t mode_bouncing(wm_arcade_actor_t*a,const wm_arcade_razor_callbacks_t*cb)
{a->x_vel=0;a->z_vel=0;if(a->anim_mode&WM_MODE_END){a->move_dir^=(WM_MOVE_LEFT+WM_MOVE_RIGHT);a->facing_dir=(a->new_facing_dir&(WM_MOVE_UP+WM_MOVE_DOWN))|a->move_dir;anim(a,WM_RZR_ANIM_RUN2,cb);setmode(a,WM_PMODE_RUNNING);return WM_RZR_STEP_ACTION;}return WM_RZR_STEP_IDLE;}
static wm_arcade_razor_step_result_t mode_turnbuckle(wm_arcade_actor_t*a,const wm_arcade_razor_callbacks_t*cb)
{if(a->stick_val_cur&WM_MOVE_DOWN){anim(a,WM_RZR_ANIM_CLIMB_DOWN,cb);setmode(a,WM_PMODE_CLIMBTURNBKL);return WM_RZR_STEP_ACTION;}if(!(a->but_val_down&WM_BTN_ATTACK_MASK))return WM_RZR_STEP_IDLE;if(action_table[a->but_val_down&WM_BTN_ATTACK_MASK]==WM_RZR_ACT_NONE)return WM_RZR_STEP_IDLE;setmode(a,WM_PMODE_INAIR);anim(a,WM_RZR_ANIM_TBUKL_ELBOW,cb);snd(a,WM_RZR_SND_TURNDIVE,cb);if(cb&&cb->jump_rope_audio)cb->jump_rope_audio(a,cb->user);return WM_RZR_STEP_ACTION;}
static wm_arcade_razor_step_result_t mode_block(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_razor_env_t*e,const wm_arcade_razor_callbacks_t*cb)
{uint16_t d;a->block_time++;if(a->block_time>=160&&o&&o->player_mode==WM_PMODE_BLOCK&&a->closest_xdist<0x61&&a->closest_xdist>=45&&a->closest_zdist<30){setmode(a,WM_PMODE_NORMAL);anim(a,WM_RZR_ANIM_PUSH4,cb);snd(a,WM_RZR_SND_PUSH,cb);return WM_RZR_STEP_ACTION;}d=a->but_val_down&WM_BTN_ATTACK_MASK;if(!d)return WM_RZR_STEP_IDLE;if(!(a->but_val_cur&WM_BTN_BLOCK)){setmode(a,WM_PMODE_NORMAL);return mode_normal(a,o,e,cb);}if(d==1||d==3){setmode(a,WM_PMODE_NORMAL);anim(a,WM_RZR_ANIM_PUSH4,cb);snd(a,WM_RZR_SND_PUSH,cb);return WM_RZR_STEP_ACTION;}return WM_RZR_STEP_IDLE;}
static wm_arcade_razor_step_result_t mode_headhold(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_razor_env_t*e,const wm_arcade_razor_callbacks_t*cb)
{wm_arcade_razor_action_id_t ac;if(cb&&cb->bozo_check&&cb->bozo_check(a,cb->user)){snd(a,WM_RZR_SND_GRABHOLD,cb);anim(a,(e&&(e->pcnt&1))?WM_RZR_ANIM_PILE_DRIVER3:WM_RZR_ANIM_RAZORS_EDGE,cb);return WM_RZR_STEP_ACTION;}if(!o||o->player_mode!=WM_PMODE_HEADHELD){a->z_fixed-=6<<16;a->facing_dir=a->new_facing_dir=(a->obj_control&WM_OBJ_FLIPH)?WM_MOVE_DOWN_LEFT:WM_MOVE_DOWN_RIGHT;setmode(a,WM_PMODE_NORMAL);return WM_RZR_STEP_ACTION;}if(a->anim_mode&WM_MODE_UNINT)return WM_RZR_STEP_IDLE;ac=action_table[a->but_val_down&WM_BTN_ATTACK_MASK];if(ac==WM_RZR_ACT_PUNCH||ac==WM_RZR_ACT_PUNCHKICK||ac==WM_RZR_ACT_KICK||ac==WM_RZR_ACT_SUPER_PUNCH){if(cb&&cb->find_and_kill_endless)cb->find_and_kill_endless(a,cb->user);}if(ac==WM_RZR_ACT_PUNCH||ac==WM_RZR_ACT_PUNCHKICK){if(a->stick_val_cur&WM_MOVE_UP){anim(a,WM_RZR_ANIM_USLASHES_TO_HEAD,cb);snd(a,WM_RZR_SND_UPRCUT_T2,cb);}else if(a->stick_val_cur&WM_MOVE_DOWN){anim(a,WM_RZR_ANIM_DSLASHES_TO_HEAD,cb);snd(a,WM_RZR_SND_UPRCUT_T2,cb);}else{anim(a,WM_RZR_ANIM_KICK2_4,cb);snd(a,WM_RZR_SND_KICK,cb);}return WM_RZR_STEP_ACTION;}if(ac==WM_RZR_ACT_SUPER_PUNCH){if(a->stick_val_cur&WM_MOVE_DOWN){a->special_damage_time=(e?e->pcnt:0)+15;a->next_damage=WM_D_UPRCUT/2;anim(a,WM_RZR_ANIM_UPPERCUT4,cb);snd(a,WM_RZR_SND_UPRCUT,cb);return WM_RZR_STEP_ACTION;}return WM_RZR_STEP_IDLE;}if(ac==WM_RZR_ACT_KICK){anim(a,WM_RZR_ANIM_KICK2_4,cb);snd(a,WM_RZR_SND_KICK,cb);return WM_RZR_STEP_ACTION;}return WM_RZR_STEP_IDLE;}
static wm_arcade_razor_step_result_t mode_headheld(wm_arcade_actor_t*a,const wm_arcade_razor_env_t*e,const wm_arcade_razor_callbacks_t*cb)
{if(a->anim_mode&WM_MODE_NOGRAVITY){if(cb&&cb->mode_choking)cb->mode_choking(a,cb->user);return WM_RZR_STEP_EXTERNAL;}if(cb&&cb->bozo_check&&cb->bozo_check(a,cb->user)){if(cb->do_reversal)cb->do_reversal(a,cb->user);if(cb->do_reversal_message)cb->do_reversal_message(a,cb->user);snd(a,WM_RZR_SND_GRABHOLD,cb);anim(a,(e&&(e->pcnt&1))?WM_RZR_ANIM_PILE_DRIVER3:WM_RZR_ANIM_RAZORS_EDGE,cb);return WM_RZR_STEP_ACTION;}if(!a->attach_proc&&a->y_int<=a->ground_y){anim(a,WM_RZR_ANIM_HEAD_HELD_STAND3,cb);return WM_RZR_STEP_ACTION;}return WM_RZR_STEP_IDLE;}

wm_arcade_razor_step_result_t wm_arcade_move_razor(wm_arcade_actor_t*a,wm_arcade_actor_t*o,const wm_arcade_razor_env_t*e,const wm_arcade_razor_callbacks_t*cb)
{
 if(!a)return WM_RZR_STEP_IDLE;
 if(cb&&cb->check_secret_moves)cb->check_secret_moves(a,wm_arcade_razor_secret_patterns,6,cb->user);
 switch(a->player_mode){case WM_PMODE_NORMAL:case 18:case 22:case 23:return mode_normal(a,o,e,cb);case WM_PMODE_RUNNING:return mode_running(a,o,e,cb);case WM_PMODE_INAIR:case WM_PMODE_ONGROUND:case WM_PMODE_DIZZY:case WM_PMODE_OPPOVERHEAD:case WM_PMODE_CLIMBTURNBKL:case WM_PMODE_GRAPPLE:case WM_PMODE_SLAVE:case WM_PMODE_PUPPET2:case WM_PMODE_CHOKEHOLD:return WM_RZR_STEP_IDLE;case WM_PMODE_ATTACHED:if(cb&&cb->keep_attached)cb->keep_attached(a,cb->user);else(void)wm_arcade_keep_attached(a);if(!a->attach_proc){setmode(a,WM_PMODE_NORMAL);a->anim_mode=0;}return WM_RZR_STEP_EXTERNAL;case WM_PMODE_BOUNCING:return mode_bouncing(a,cb);case WM_PMODE_ONTURNBKL:return mode_turnbuckle(a,cb);case WM_PMODE_BLOCK:return mode_block(a,o,e,cb);case WM_PMODE_DEAD:if(cb&&cb->mode_dead)cb->mode_dead(a,cb->user);return WM_RZR_STEP_EXTERNAL;case WM_PMODE_WAITANIM:if((a->anim_mode&WM_MODE_END)&&cb&&cb->code_addr)cb->code_addr(a,a->code_addr,cb->user);return WM_RZR_STEP_EXTERNAL;case WM_PMODE_MASTER:if(cb&&cb->master_keep_attached)cb->master_keep_attached(a,cb->user);else(void)wm_arcade_master_keep_attached(a);return WM_RZR_STEP_EXTERNAL;case WM_PMODE_HEADHOLD:return mode_headhold(a,o,e,cb);case WM_PMODE_HEADHELD:return mode_headheld(a,e,cb);case WM_PMODE_PUPPET:if(cb&&cb->mode_puppet)cb->mode_puppet(a,cb->user);return WM_RZR_STEP_EXTERNAL;case WM_PMODE_INAIR2:if(cb&&cb->mode_inair2)cb->mode_inair2(a,cb->user);return WM_RZR_STEP_EXTERNAL;case WM_PMODE_CHOKING:if(cb&&cb->mode_choking)cb->mode_choking(a,cb->user);return WM_RZR_STEP_EXTERNAL;default:return WM_RZR_STEP_IDLE;}
}

int wm_arcade_razor_release_charge_flying_kick(wm_arcade_actor_t*a,wm_arcade_actor_t*o,uint16_t charge,const wm_arcade_razor_callbacks_t*cb)
{if(!a||charge<85||a->player_mode==WM_PMODE_HEADHELD||a->player_mode==WM_PMODE_HEADHOLD||a->getup_time||(a->anim_mode&WM_MODE_UNINT)||!o||groundish(o))return 0;if(cb&&cb->ck_ignore&&cb->ck_ignore(a,cb->user))return 0;setmode(a,WM_PMODE_INAIR);a->x_vel=1;anim(a,WM_RZR_ANIM_FLYING_KICK,cb);snd(a,WM_RZR_SND_FLYKICK,cb);return 1;}
int wm_arcade_razor_release_charge_slashes(wm_arcade_actor_t*a,uint16_t charge,const wm_arcade_razor_callbacks_t*cb)
{if(!a||charge<100||(a->anim_mode&WM_MODE_UNINT)||a->getup_time||a->player_mode==WM_PMODE_HEADHELD||a->player_mode==WM_PMODE_HEADHOLD||a->player_mode==WM_PMODE_ONGROUND||a->player_mode==WM_PMODE_DEAD)return 0;a->special_move_addr=(uintptr_t)WM_RZR_ANIM_REPEAT_SLASH;snd(a,WM_RZR_SND_KICK_T2,cb);return 1;}
int wm_arcade_razor_fire_secret(wm_arcade_actor_t*a,wm_arcade_actor_t*o,wm_arcade_razor_secret_id_t id,uint32_t pcnt,const wm_arcade_razor_callbacks_t*cb)
{
 if(!a)return 0;
 switch(id){case WM_RZR_SECRET_CHARGE_FLYING_KICK:return 0;case WM_RZR_SECRET_NECK_GRAB:if((a->anim_mode&WM_MODE_UNINT)||!o||o->player_mode==WM_PMODE_ONGROUND||o->player_mode==WM_PMODE_DEAD||o->player_mode==WM_PMODE_HEADHELD)return 0;if((uint32_t)(pcnt-a->last_headhold)<120)anim(a,WM_RZR_ANIM_FAKE_HOLD3,cb);else if(a->closest_xdist<=85)anim(a,WM_RZR_ANIM_HEAD_HOLD2_3,cb);else anim(a,WM_RZR_ANIM_HEAD_HOLD3,cb);return 1;case WM_RZR_SECRET_GRAB_FLING:case WM_RZR_SECRET_GRAB_FLING2:if((a->anim_mode&WM_MODE_UNINT)||!o)return 0;if(o->player_mode==WM_PMODE_BOUNCING||o->player_mode==WM_PMODE_RUNNING){anim(a,f24(a,WM_RZR_ANIM_HIPTOSS2,WM_RZR_ANIM_HIPTOSS4),cb);snd(a,WM_RZR_SND_GRABFLING_PUNCH,cb);return 1;}if(id==WM_RZR_SECRET_GRAB_FLING2)return 0;if(o->player_mode==WM_PMODE_HEADHELD||groundish(o))return 0;anim(a,f24(a,WM_RZR_ANIM_GRABFLING2,WM_RZR_ANIM_GRABFLING4),cb);snd(a,WM_RZR_SND_GRABFLING_PUNCH,cb);return 1;case WM_RZR_SECRET_HIP_TOSS:case WM_RZR_SECRET_HIP_TOSS2:if((a->anim_mode&WM_MODE_UNINT)||!o)return 0;if(id==WM_RZR_SECRET_HIP_TOSS2&&!(o->player_mode==WM_PMODE_BOUNCING||o->player_mode==WM_PMODE_RUNNING))return 0;if(o->player_mode==WM_PMODE_ONGROUND||o->player_mode==WM_PMODE_DEAD||o->player_mode==WM_PMODE_HEADHELD)return 0;if(o->player_mode!=WM_PMODE_INAIR&&o->player_mode!=WM_PMODE_INAIR2&&a->closest_dist>0x70)return 0;anim(a,f24(a,WM_RZR_ANIM_HIPTOSS2,WM_RZR_ANIM_HIPTOSS4),cb);snd(a,WM_RZR_SND_GRABFLING_PUNCH,cb);return 1;case WM_RZR_SECRET_DOWN_SLASH:if((a->anim_mode&WM_MODE_UNINT)||!o||o->player_mode==WM_PMODE_DEAD)return 0;if(a->player_mode!=WM_PMODE_NORMAL&&a->player_mode!=WM_PMODE_RUNNING&&a->player_mode!=WM_PMODE_HEADHOLD)return 0;if(a->player_mode==WM_PMODE_RUNNING&&a->getup_time)return 0;anim(a,WM_RZR_ANIM_DSLASH3,cb);snd(a,WM_RZR_SND_KICK_T2,cb);return 1;default:return 0;}
}
static int hh_common(wm_arcade_actor_t*a,wm_arcade_actor_t*o,wm_arcade_razor_anim_id_t move,int bonus,int sound,const wm_arcade_razor_callbacks_t*cb)
{wm_arcade_actor_t*t;if(!a||!o||(a->player_mode!=WM_PMODE_HEADHOLD&&a->player_mode!=WM_PMODE_HEADHELD)||a->immobilize_time)return 0;if(a->player_mode==WM_PMODE_HEADHELD){if(a->i_will_die)return 0;if(cb&&cb->do_reversal)cb->do_reversal(a,cb->user);if(cb&&cb->do_reversal_message)cb->do_reversal_message(a,cb->user);a->smart_target=a->who_hit_me;t=a->who_hit_me;}else{if(bonus&&cb&&cb->bonus_message)cb->bonus_message(a,bonus,cb->user);a->smart_target=a->who_i_hit;t=a->who_i_hit;}if(!t)t=o;t->immobilize_time=15;if(cb&&cb->find_and_kill_endless)cb->find_and_kill_endless(a,cb->user);if(sound)snd(a,WM_RZR_SND_GRABHOLD,cb);a->special_move_addr=(uintptr_t)move;return 1;}
int wm_arcade_razor_fire_monitor(wm_arcade_actor_t*a,wm_arcade_actor_t*o,wm_arcade_razor_monitor_id_t id,const wm_arcade_razor_env_t*e,int opp_leaping,const wm_arcade_razor_callbacks_t*cb)
{
 if(!a)return 0;
 switch(id){case WM_RZR_MON_CHARGE_SLASHES:return 0;case WM_RZR_MON_HEADHOLD_PILE:return hh_common(a,o,WM_RZR_ANIM_PILE_DRIVER3,7,1,cb);case WM_RZR_MON_HEADHOLD_EDGE:return hh_common(a,o,WM_RZR_ANIM_RAZORS_EDGE,33,1,cb);case WM_RZR_MON_HEADHOLD_RUG:return hh_common(a,o,WM_RZR_ANIM_RUGSHAKE2,6,0,cb);case WM_RZR_MON_HEADHOLD_COMBO1:case WM_RZR_MON_HEADHOLD_COMBO2:if(a->player_mode!=WM_PMODE_HEADHOLD||a->immobilize_time)return 0;if(!cb||!cb->check_combo_go||cb->check_combo_go(a,cb->user)<0)return 0;if(cb->find_and_kill_endless)cb->find_and_kill_endless(a,cb->user);a->special_move_addr=(uintptr_t)(id==WM_RZR_MON_HEADHOLD_COMBO1?WM_RZR_ANIM_COMBO_PUNCH:WM_RZR_ANIM_COMBO_KICK);return 1;case WM_RZR_MON_GRAB_TOSS_AIR:if(!o||(a->anim_mode&WM_MODE_UNINT)||a->player_mode==WM_PMODE_HEADHOLD||groundish(o))return 0;if(o->player_mode==WM_PMODE_INAIR||o->player_mode==WM_PMODE_INAIR2)a->special_move_addr=(uintptr_t)f24(a,WM_RZR_ANIM_HIPTOSS2_2,WM_RZR_ANIM_HIPTOSS2_4);else{if(cb&&cb->find_and_kill_endless)cb->find_and_kill_endless(a,cb->user);if(opp_leaping)a->special_move_addr=(uintptr_t)f24(a,WM_RZR_ANIM_HIPTOSS2_2,WM_RZR_ANIM_HIPTOSS2_4);else{if(a->closest_dist>0x68)return 0;a->special_move_addr=(uintptr_t)f24(a,WM_RZR_ANIM_HIPTOSS2,WM_RZR_ANIM_HIPTOSS4);}}snd(a,WM_RZR_SND_GRABFLING_PUNCH,cb);return 1;case WM_RZR_MON_SLIDING_RUG:if(a->player_mode==WM_PMODE_HEADHELD||a->player_mode==WM_PMODE_HEADHOLD||a->player_mode==WM_PMODE_ONGROUND||a->player_mode==WM_PMODE_DEAD||(a->anim_mode&WM_MODE_UNINT)||a->getup_time||!o||o->player_mode==WM_PMODE_DEAD)return 0;if(cb&&cb->ck_ignore_reversed&&cb->ck_ignore_reversed(o,a,cb->user))return 0;snd(a,WM_RZR_SND_GRABHOLD,cb);a->special_move_addr=(uintptr_t)WM_RZR_ANIM_SLIDING_RUG;return 1;case WM_RZR_MON_FINISH1:case WM_RZR_MON_FINISH2:if(!e||((e->p1rounds|e->p2rounds)<2))return 0;a->special_move_addr=(uintptr_t)(id==WM_RZR_MON_FINISH1?WM_RZR_ANIM_FINISH1:WM_RZR_ANIM_FINISH2);return 1;default:return 0;}
}
void wm_arcade_razor_velocity_for_dir(unsigned d,int32_t*x,int32_t*z)
{static const int32_t t[8][2]={{0,-WM_RZR_WALK_VEL},{WM_RZR_WALK_DVEL,-WM_RZR_WALK_DVEL},{WM_RZR_WALK_VEL,0},{WM_RZR_WALK_DVEL,WM_RZR_WALK_DVEL},{0,WM_RZR_WALK_VEL},{-WM_RZR_WALK_DVEL,WM_RZR_WALK_DVEL},{-WM_RZR_WALK_VEL,0},{-WM_RZR_WALK_DVEL,-WM_RZR_WALK_DVEL}};d&=7;if(x)*x=t[d][0];if(z)*z=t[d][1];}
