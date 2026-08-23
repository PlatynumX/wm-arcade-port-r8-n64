#include "wm_arcade_source_animation_runtime.h"
#include "wm_arcade_anim_combat.h"
#include "wm_arcade_attach_anim.h"
#include "wm_arcade_target_offsets.h"
#include "wmania_ring_geometry.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define SRC_GRAVITY 0x00008000
#define SRC_TGT_GROUND 0x1000
#define SRC_AM_ABS 0
#define SRC_AM_FACE_REL 1
#define SRC_AM_HIT_REL 2
#define SRC_AM_NEWFACE_REL 3
#define SRC_ATM_CLOSEST 0
#define SRC_RC_PLAYER 0x0000
#define SRC_RC_OPPONENT 0x0100
#define SRC_RC_FRONT 0
#define SRC_RC_BACK 1
#define SRC_RC_EITHER 2

static int32_t av(const wm_source_anim_ins_t *i,unsigned n){return (i&&n<i->argc)?i->a[n].value:0;}
static const char *at(const wm_source_anim_ins_t *i,unsigned n){return (i&&n<i->argc&&i->a[n].text)?i->a[n].text:"";}
static uint32_t uabs32(int32_t v){return v<0?(uint32_t)(-(int64_t)v):(uint32_t)v;}
static uint32_t isqrt32(uint32_t n){uint32_t x=0,bit=1u<<30;while(bit>n)bit>>=2;while(bit){uint32_t y=x+bit;x>>=1;if(n>=y){n-=y;x+=bit;}bit>>=2;}return x;}
static wm_arcade_actor_t *opp(wm_arcade_actor_t *a){return a?(a->attach_proc?a->attach_proc:(a->who_i_hit?a->who_i_hit:a->smart_target)):0;}
static int attached_pair(wm_arcade_actor_t *a,wm_arcade_actor_t **o){wm_arcade_actor_t*q;if(!a)return 0;q=a->attach_proc;if(!q||q->attach_proc!=a)return 0;if(o)*o=q;return 1;}
static uint16_t rtick(const wm_source_anim_runtime_t*s){return(s&&s->services&&s->services->round_tick)?s->services->round_tick(s->services->user):0;}
static uint32_t pcnt(const wm_source_anim_runtime_t*s){return(s&&s->services&&s->services->pcnt)?s->services->pcnt(s->services->user):0;}
static uint32_t rnd(const wm_source_anim_runtime_t*s,uint32_t max){return(s&&s->services&&s->services->rndrng0)?s->services->rndrng0(max,s->services->user):0;}
static int rndper(const wm_source_anim_runtime_t*s,uint16_t p){return(s&&s->services&&s->services->rndper_hi)?s->services->rndper_hi(p,s->services->user):0;}
static void set_status(wm_arcade_actor_t*a,int on){if(!a)return;if(on)a->anim_mode|=WM_ARCADE_MODE_STATUS;else a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;}
static int face_right(const wm_arcade_actor_t*a){return a&&((a->facing_dir&WM_MOVE_RIGHT)!=0);}
static int face_down(const wm_arcade_actor_t*a){return a&&((a->facing_dir&WM_MOVE_DOWN)!=0);}
static int32_t rel_x(const wm_arcade_actor_t*a,int32_t v,int mode){if(mode==SRC_AM_ABS)return v;if(mode==SRC_AM_FACE_REL)return face_right(a)?v:-v;if(mode==SRC_AM_HIT_REL)return(a&&((a->hit_side&WM_MOVE_RIGHT)!=0))?v:-v;return(a&&((a->new_facing_dir&WM_MOVE_RIGHT)!=0))?v:-v;}
static int32_t rel_z(const wm_arcade_actor_t*a,int32_t v,int mode){if(mode==SRC_AM_ABS)return v;if(mode==SRC_AM_FACE_REL)return face_down(a)?v:-v;if(mode==SRC_AM_HIT_REL)return(a&&((a->hit_side&WM_MOVE_UP)!=0))?v:-v;return(a&&((a->new_facing_dir&WM_MOVE_DOWN)!=0))?v:-v;}

static const wm_source_anim_table_t *table_for(const wm_source_anim_runtime_t*s,const wm_source_anim_ins_t*i,unsigned n){const wm_source_anim_table_t*t=0;if(!i||n>=i->argc)return 0;if(i->a[n].kind==WM_SRC_ARG_TABLE)t=wm_source_anim_table_by_id((uint16_t)i->a[n].value);if(!t)t=wm_source_anim_table_find(s&&s->program?s->program->source_file:0,at(i,n));return t;}
static const wm_source_anim_arg_t *table_entry(const wm_source_anim_runtime_t*s,const wm_source_anim_ins_t*i,unsigned n,unsigned idx){const wm_source_anim_table_t*t=table_for(s,i,n);return(t&&idx<t->count)?&t->entries[idx]:0;}
static int branch_arg(wm_source_anim_runtime_t*s,wm_arcade_actor_t*a,const wm_source_anim_arg_t*x){const wm_source_anim_program_t*p;if(!s||!x)return 0;if(x->kind==WM_SRC_ARG_LOCAL_PC){if(x->value>=0&&s->program&&x->value<s->program->count){s->pc=(uint16_t)x->value;return 1;}return 0;}p=wm_source_anim_program_find((uint8_t)a->wrestler_num,x->text?x->text:"");if(p){s->program=p;s->pc=0;return 1;}return 0;}
static int branch_n(wm_source_anim_runtime_t*s,wm_arcade_actor_t*a,const wm_source_anim_ins_t*i,unsigned n){return(i&&n<i->argc)?branch_arg(s,a,&i->a[n]):0;}
static int change_program(wm_source_anim_runtime_t*s,wm_arcade_actor_t*a,const char*label,int reset_state){const wm_source_anim_program_t*p;if(!s||!a||!label)return 0;p=wm_source_anim_program_find((uint8_t)a->wrestler_num,label);if(!p)return 0;s->program=p;s->pc=0;if(reset_state){a->anim_mode=0;a->ani_count=1;a->gravity=SRC_GRAVITY;}return 1;}
static int set_target_offsets(wm_arcade_actor_t*a,wm_arcade_actor_t*o,uint16_t area){int16_t x=0,y=0,z=0;if(!a||!o)return 0;if(!wm_source_target_offsets((uint16_t)o->player_mode,(uint8_t)o->wrestler_num,(uint16_t)(area&~SRC_TGT_GROUND),&x,&y,&z))return 0;a->tgt_xoff=x;a->tgt_yoff=(area&SRC_TGT_GROUND)?0:y;a->tgt_zoff=z;return 1;}
static int32_t *word_field(wm_arcade_actor_t*a,const char*n){if(!a||!n)return 0;if(!strcasecmp(n,"USR_VAR1"))return&a->usr_var1;if(!strcasecmp(n,"USR_VAR2"))return&a->usr_var2;if(!strcasecmp(n,"DEBRIS_X"))return&a->debris_x;if(!strcasecmp(n,"OBJ_GRAVITY"))return&a->gravity;return 0;}
static int32_t *butcount_field(wm_arcade_actor_t*a,const char*n){if(!a||!n)return 0;if(!strcasecmp(n,"PUNCHB_COUNT"))return&a->punchb_count;if(!strcasecmp(n,"BLOCKB_COUNT"))return&a->blockb_count;if(!strcasecmp(n,"SPUNCHB_COUNT"))return&a->spunchb_count;if(!strcasecmp(n,"KICKB_COUNT"))return&a->kickb_count;if(!strcasecmp(n,"SKICKB_COUNT"))return&a->skickb_count;return 0;}
static int rope_near(const wm_arcade_actor_t*a,int mode,int dist){const WmRingBoundarySeed*line;int x;if(!a||a->in_ring)return 0;if((mode&0xff)==SRC_RC_FRONT)line=face_right(a)?wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_ROPE):wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_ROPE);else if((mode&0xff)==SRC_RC_BACK)line=face_right(a)?wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_ROPE):wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_ROPE);else line=(a->x_int>WM_RING_X_CENTER)?wm_ring_boundary_seed(WM_RING_BOUNDARY_RIGHT_ROPE):wm_ring_boundary_seed(WM_RING_BOUNDARY_LEFT_ROPE);x=wm_ring_calc_line_x(line,(int16_t)a->z_int);return abs(a->x_int-x)<=dist;}
static void leapatpos(wm_arcade_actor_t*a,int ticks,int maxdist,int ax,int ay,int az){int64_t dx,dz;uint32_t d;if(!a||ticks<=0)return;dx=((int64_t)a->tgt_xoff<<16)-(a->x_fixed+((int64_t)(face_right(a)?ax:-ax)<<16));dz=((int64_t)a->tgt_zoff<<16)-(a->z_fixed+((int64_t)az<<16));a->x_vel=(int32_t)(dx/ticks);a->z_vel=(int32_t)(dz/ticks);d=isqrt32((uint32_t)(((uabs32((int32_t)dx)>>16)*(uabs32((int32_t)dx)>>16))+((uabs32((int32_t)dz)>>16)*(uabs32((int32_t)dz)>>16))));if(maxdist>=0&&d>(uint32_t)maxdist&&d){a->x_vel=(int32_t)(((int64_t)a->x_vel*maxdist)/d);a->z_vel=(int32_t)(((int64_t)a->z_vel*maxdist)/d);} {int64_t dy=((int64_t)a->tgt_yoff<<16)-(a->y_fixed-((int64_t)ay<<16));int64_t grav=((int64_t)a->gravity*ticks*ticks)/2;int64_t vy=(dy+grav)/ticks;if(vy>0x0f0000)vy=0x0f0000;a->y_vel=(int32_t)vy;} (void)az;}

void wm_source_anim_runtime_init(wm_source_anim_runtime_t*s){if(s)memset(s,0,sizeof(*s));}
void wm_source_anim_runtime_bind(wm_source_anim_runtime_t*s,const wm_source_anim_services_t*v){if(s)s->services=v;}
void wm_source_anim_runtime_force_frame(wm_source_anim_runtime_t*s,const char*f){if(s)s->current_frame=f;}
bool wm_source_anim_runtime_change(wm_source_anim_runtime_t*s,wm_arcade_actor_t*a,uint8_t roster,const char*label){const wm_source_anim_services_t*v;if(!s||!a||!label)return false;v=s->services;s->program=wm_source_anim_program_find(roster,label);s->pc=0;s->current_frame=0;s->instructions_executed=0;s->fault=0;s->services=v;if(!s->program)return false;a->anim_mode=0;a->ani_count=1;a->gravity=SRC_GRAVITY;return true;}

static int exec_cmd(wm_source_anim_runtime_t*s,wm_arcade_actor_t*a,const wm_source_anim_ins_t*i){wm_arcade_actor_t*o;int32_t v,*fp;const wm_source_anim_arg_t*te;const wm_source_anim_table_t*t;unsigned idx;int cond=0;wm_arcade_attack_on_args_t ao;wm_arcade_attack_on_z_args_t az;
#define NEXT() do{s->pc++;return 0;}while(0)
#define HOLD() do{a->ani_count=1;return 1;}while(0)
    switch(i->opcode){
    case 0: a->anim_mode|=WM_ARCADE_MODE_END; return 1; /* ZIP is a return address in source; portable VM ends this stream. */
    case 1: s->pc=0; return 0;
    case 2: a->anim_mode=(uint16_t)av(i,0); a->status_flags&=~(WM_STATUS_SCROLL_CTRL|WM_STATUS_DEAD_ANIM|WM_STATUS_DID_RAISEARM|WM_STATUS_KOD|WM_STATUS_COMBO_BROKEN|WM_STATUS_PUSH);if(a->ptime)a->ptime=1;NEXT();
    case 3: a->x_vel=a->y_vel=a->z_vel=0;NEXT();
    case 4: a->climbing_thru=0;v=av(i,0);if(a->player_mode!=WM_PMODE_DEAD){if(v==WM_PMODE_HEADHOLD&&a->delay_meter<6*60)a->delay_meter=9*60;a->player_mode=(uint16_t)v;}NEXT();
    case 5: a->y_vel=av(i,0);NEXT();
    case 6: ao.attack_mode=(uint16_t)av(i,0);ao.xoff=(int16_t)av(i,1);ao.yoff=(int16_t)av(i,2);ao.width=(int16_t)av(i,3);ao.height=(int16_t)av(i,4);wm_arcade_ani_attack_on(a,&ao);NEXT();
    case 7: wm_arcade_ani_attack_off(a,rtick(s));NEXT();
    case 8: o=a->smart_target;if(o){set_target_offsets(a,o,(uint16_t)av(i,5));a->tgt_xoff+=o->x_int;a->tgt_yoff+=o->y_int;a->tgt_zoff+=o->z_int;leapatpos(a,av(i,0),av(i,1),av(i,6),av(i,7),av(i,8));}NEXT();
    case 9: a->attach_xoff=av(i,0);a->attach_yoff=av(i,1);NEXT();
    case 10: wm_arcade_anim_detach(a);NEXT();
    case 11: o=0;if((a->anim_mode&WM_ARCADE_MODE_KEEPATTACHED)&&attached_pair(a,&o)&&!(o->anim_mode&WM_ARCADE_MODE_GHOST)){if(o->y_vel<=0&&o->y_int<=o->ground_y){if(s->services&&s->services->code)s->services->code(a,"SMALL_BOUNCE",s->services->user);NEXT();}}if(a->y_vel<=0&&a->y_int<=a->ground_y){if(s->services&&s->services->code)s->services->code(a,"SMALL_BOUNCE",s->services->user);NEXT();}HOLD();
    case 12: a->obj_control^=WM_OBJ_FLIPH;NEXT();
    case 13: a->y_vel=av(i,0)<<16;NEXT();
    case 14: az.attack_mode=(uint16_t)av(i,0);az.xoff=(int16_t)av(i,1);az.yoff=(int16_t)av(i,2);az.zoff=(int16_t)av(i,3);az.width=(int16_t)av(i,4);az.height=(int16_t)av(i,5);az.depth=(int16_t)av(i,6);wm_arcade_ani_attack_on_z(a,&az);NEXT();
    case 15: a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_NOGRAVITY;NEXT();
    case 16: a->anim_mode|=WM_ARCADE_MODE_NOGRAVITY;NEXT();
    case 17: if(!branch_n(s,a,i,0))s->fault=17;return 0;
    case 18: a->attach_xoff=av(i,0);a->attach_yoff=av(i,1);a->attach_zoff=av(i,2);NEXT();
    case 19: NEXT();
    case 20: if(a->but_val_cur&(uint16_t)av(i,0)){a->facing_dir=a->new_facing_dir;if(face_right(a))a->obj_control&=(uint16_t)~WM_OBJ_FLIPH;else a->obj_control|=WM_OBJ_FLIPH;HOLD();}NEXT();
    case 21: v=av(i,0);if(!face_right(a))v=-v;a->x_int+=v;a->x_fixed=a->x_int<<16;v=av(i,1);a->y_int+=v;a->y_fixed=a->y_int<<16;a->scroll_y-=v;a->z_int+=av(i,2);a->z_fixed=a->z_int<<16;NEXT();
    case 22: a->obj_friction=av(i,0);a->anim_mode|=WM_ARCADE_MODE_FRICTION;NEXT();
    case 23: if(a->y_vel<av(i,0))a->y_vel=av(i,0);NEXT();
    case 24: if(attached_pair(a,&o)){o->y_vel=av(i,1);o->z_vel=av(i,2);o->x_vel=face_right(a)?av(i,0):-av(i,0);}NEXT();
    case 25: NEXT();
    case 26: if(s->services&&s->services->sound)s->services->sound(a,at(i,0),av(i,0),s->services->user);NEXT();
    case 27: a->facing_dir=a->new_facing_dir;NEXT();
    case 28: s->pc++;a->ani_count=(int32_t)av(i,0);return a->ani_count!=0;
    case 29: if(a->anim_mode&WM_ARCADE_MODE_STATUS){if(!branch_n(s,a,i,0))s->fault=29;return 0;}NEXT();
    case 30: if(s->services&&s->services->code)s->services->code(a,at(i,0),s->services->user);NEXT();
    case 31: if(s->services&&s->services->shake)s->services->shake(a,31,av(i,0),s->services->user);NEXT();
    case 32: if(!change_program(s,a,at(i,0),0))s->fault=32;return 0;
    case 33: a->facing_dir=(a->obj_control&WM_OBJ_FLIPH)?WM_MOVE_UP_LEFT:WM_MOVE_UP_RIGHT;NEXT();
    case 34: a->facing_dir=(a->obj_control&WM_OBJ_FLIPH)?WM_MOVE_DOWN_LEFT:WM_MOVE_DOWN_RIGHT;NEXT();
    case 35: if(s->services&&s->services->rope)s->services->rope(a,(a->x_int<=WM_RING_X_CENTER)?2:3,av(i,0)<0?5:4,av(i,0),s->services->user);NEXT();
    case 36: if(!a->in_ring&&s->services&&s->services->rope){s->services->rope(a,0,6,av(i,0)&3,s->services->user);s->services->rope(a,face_right(a)?3:2,6,av(i,0)&3,s->services->user);}NEXT();
    case 37: if(s->services&&s->services->rope)s->services->rope(a,(a->x_int<=WM_RING_X_CENTER)?2:3,av(i,0)<0?3:2,av(i,0),s->services->user);NEXT();
    case 38: a->ani_speed=(uint16_t)av(i,0);NEXT();
    case 39: leapatpos(a,av(i,0),av(i,1),av(i,2),av(i,3),av(i,4));NEXT();
    case 40: a->x_vel=a->z_vel=0;NEXT();
    case 41: if(s->services&&s->services->set_rope_z)s->services->set_rope_z(a,av(i,0),av(i,1),s->services->user);NEXT();
    case 42: HOLD();
    case 44: a->x_vel=rel_x(a,av(i,0),av(i,1));NEXT();
    case 45: if(!(a->anim_mode&WM_ARCADE_MODE_STATUS)){if(!branch_n(s,a,i,0))s->fault=45;return 0;}NEXT();
    case 46: o=a->who_i_hit;if((a->anim_mode&WM_ARCADE_MODE_STATUS)&&o){int dist=abs(a->x_int-o->x_int);if(dist<=av(i,0)){a->x_vel=(a->x_int<=o->x_int)?av(i,1):-av(i,1);a->obj_friction=0x3000;a->anim_mode|=WM_ARCADE_MODE_FRICTION;NEXT();}}if(!branch_n(s,a,i,2))s->fault=46;return 0;
    case 47: wm_arcade_ani_clr_damage(a);NEXT();
    case 48: a->z_vel=rel_z(a,av(i,0),av(i,1));NEXT();
    case 49: fp=word_field(a,at(i,0));set_status(a,fp&&*fp);NEXT();
    case 50: v=av(i,0);if(a->obj_control&WM_OBJ_FLIPH)v^=(WM_MOVE_LEFT|WM_MOVE_RIGHT);a->facing_dir=v;NEXT();
    case 51: fp=word_field(a,at(i,0));if(fp)*fp=av(i,1);NEXT();
    case 52: if(!a->dizzy)a->getup_time=av(i,0);NEXT();
    case 53: if(a->getup_time)HOLD();NEXT();
    case 54: a->stars_flag=0;NEXT();
    case 55: if(!a->in_ring&&s->services&&s->services->rope){for(v=0;v<4;v++)s->services->rope(a,v,6,av(i,0)&3,s->services->user);}NEXT();
    case 56: wm_arcade_ani_damage(a,(int16_t)av(i,0),s->services?s->services->react:0);NEXT();
    case 57: a->dizzy=1;a->stars_flag=1;NEXT();
    case 58: wm_arcade_ani_clr_status(a);NEXT();
    case 59: o=a->smart_target;if(o)set_target_offsets(a,o,(uint16_t)av(i,0));NEXT();
    case 60: case 61: case 62: NEXT();
    case 63: o=0;if(attached_pair(a,&o)){t=table_for(s,i,0);if(t){unsigned base=(unsigned)o->wrestler_num;te=(base<t->count)?&t->entries[base]:0;if(te&&te->text&&s->services&&s->services->force_other_frame)s->services->force_other_frame(a,o,te->text,s->services->user);}}NEXT();
    case 64: o=0;if(attached_pair(a,&o)){te=table_entry(s,i,0,(unsigned)o->wrestler_num);if(te&&te->text&&s->services&&s->services->change_other_anim)s->services->change_other_anim(a,o,te->text,s->services->user);}NEXT();
    case 65: if(s->services&&s->services->sound)s->services->sound(a,at(i,0),av(i,0),s->services->user);NEXT();
    case 66: if(s->services&&s->services->combat_runtime)wm_arcade_ani_damageopp(a,(int16_t)av(i,0),(int16_t)av(i,1),s->services->combat_runtime,s->services->react);NEXT();
    case 67: if(rndper(s,(uint16_t)av(i,0))){if(!branch_n(s,a,i,1))s->fault=67;return 0;}NEXT();
    case 68: wm_arcade_ani_waithitopp(a);NEXT();
    case 69: a->attachimg_frame=(av(i,0)==0&&!strcmp(at(i,0),"0"))?0:at(i,0);a->attachimg_xoff=a->attachimg_yoff=0;a->attachimg_zoff=av(i,1);if(s->services&&s->services->attach_image)s->services->attach_image(a,a->attachimg_frame,0,0,a->attachimg_zoff,s->services->user);NEXT();
    case 70: o=a->smart_target;v=av(i,0);cond=o&&((v<0)?(o->player_mode!=(uint16_t)~v):(o->player_mode==(uint16_t)v));if(cond){if(!branch_n(s,a,i,1))s->fault=70;return 0;}NEXT();
    case 71: if((a->but_val_cur&(uint16_t)av(i,0))==(uint16_t)av(i,0)){if(!branch_n(s,a,i,1))s->fault=71;return 0;}NEXT();
    case 72: if(!a->hit_blocker){if(!branch_n(s,a,i,0))s->fault=72;return 0;}NEXT();
    case 73: a->anim_mode|=WM_ARCADE_MODE_END;return 1;
    case 74: case 75: {wm_arcade_actor_t*q=((av(i,0)&SRC_RC_OPPONENT)!=0)?a->smart_target:a;int near=rope_near(q,av(i,0),av(i,1));if(i->opcode==75)near=!near;if(near){if(!branch_n(s,a,i,2))s->fault=i->opcode;return 0;}NEXT();}
    case 76: o=opp(a);if(o){v=av(i,0);if(v<0){o->delay_meter=0;v=-v;}if(!o->dizzy)o->getup_time=v;}NEXT();
    case 77: if(s->services&&s->services->rope){s->services->rope(a,0,6,1,s->services->user);s->services->rope(a,a->x_int<=WM_RING_X_CENTER?2:3,6,1,s->services->user);}NEXT();
    case 78: if(!(s->services&&s->services->buttons_down&&s->services->buttons_down(a,s->services->user)))HOLD();NEXT();
    case 79: o=0;if(attached_pair(a,&o)){s->pc++;a->ani_count=(int32_t)(((int64_t)av(i,0)*(a->ani_speed?a->ani_speed:0x100))>>8);s->current_frame=at(i,1);te=table_entry(s,i,2,(unsigned)o->wrestler_num);if(te){const wm_source_anim_table_t*st=wm_source_anim_table_find(s->program?s->program->source_file:0,te->text);if(st){unsigned off=(unsigned)av(i,3)*4u;if(off+3u<st->count){const char*fr=st->entries[off].text;if(fr&&s->services&&s->services->force_other_frame)s->services->force_other_frame(a,o,fr,s->services->user);a->attach_xoff=st->entries[off+1].value;a->attach_yoff=st->entries[off+2].value;if(st->entries[off+3].value)o->obj_control^=WM_OBJ_FLIPH;}}}return 1;}NEXT();
    case 80: wm_arcade_anim_set_opp_mode_bits(a,(uint16_t)av(i,0));NEXT();
    case 81: wm_arcade_anim_clear_opp_mode_bits(a,(uint16_t)av(i,0));NEXT();
    case 82: o=0;if(attached_pair(a,&o)){t=table_for(s,i,0);if(t){idx=(unsigned)o->wrestler_num*2u;if(idx+1u<t->count){v=t->entries[idx].value;if(!face_right(o))v=-v;o->x_int+=v;o->x_fixed=o->x_int<<16;o->y_int+=t->entries[idx+1].value;o->y_fixed=o->y_int<<16;}}}NEXT();
    case 83: if(a->hit_blocker){if(!branch_n(s,a,i,0))s->fault=83;return 0;}NEXT();
    case 84: if(a->player_mode!=WM_PMODE_DEAD&&!a->i_will_die){a->player_mode=WM_PMODE_ONGROUND;if(a->immobilize_time||a->getup_time)HOLD();a->stars_flag=0;if(s->services&&s->services->do_roll&&s->services->do_roll(a,s->services->user))HOLD();NEXT();}HOLD();
    case 85: o=0;if(attached_pair(a,&o))o->facing_dir=o->new_facing_dir;NEXT();
    case 86: o=a->smart_target;set_status(a,0);if(o){for(idx=0;idx<i->argc;idx++){v=av(i,idx);if(v<0)break;if(o->wrestler_num==v){set_status(a,1);break;}}}NEXT();
    case 87: if(s->services&&s->services->create_proc)s->services->create_proc(a,i,s->services->user);NEXT();
    case 88: fp=butcount_field(a,at(i,0));if(fp&&*fp>=av(i,1)){if(!branch_n(s,a,i,2))s->fault=88;return 0;}NEXT();
    case 89: fp=butcount_field(a,at(i,0));if(fp&&*fp<av(i,1)){if(!branch_n(s,a,i,2))s->fault=89;return 0;}NEXT();
    case 90: if(a->rpt_count){if(!branch_n(s,a,i,0))s->fault=90;return 0;}NEXT();
    case 91: if(!a->rpt_count){if(!branch_n(s,a,i,0))s->fault=91;return 0;}NEXT();
    case 92: o=a->smart_target;if(o&&o->in_ring!=a->in_ring){if(!branch_n(s,a,i,0))s->fault=92;return 0;}NEXT();
    case 93: o=0;if(attached_pair(a,&o)&&s->services&&s->services->debris)s->services->debris(o,i,s->services->user);NEXT();
    case 94: if(!a->in_ring&&s->services&&s->services->debris)s->services->debris(a,i,s->services->user);NEXT();
    case 95: if(face_right(a))a->obj_control&=(uint16_t)~WM_OBJ_FLIPH;else a->obj_control|=WM_OBJ_FLIPH;NEXT();
    case 96: o=a->smart_target;if(o&&o->in_ring==a->in_ring){set_target_offsets(a,o,(uint16_t)av(i,3));a->x_vel=face_right(a)?av(i,1):-av(i,1);}NEXT();
    case 97: a->punchb_count=a->blockb_count=a->spunchb_count=a->kickb_count=a->skickb_count=0;NEXT();
    case 98: v=av(i,0);a->rpt_count=v<0?(int32_t)rnd(s,(uint32_t)-v):v;NEXT();
    case 99: if(a->rpt_count)a->rpt_count--;NEXT();
    case 100: if(s->services&&s->services->shadowtrail)s->services->shadowtrail(a,i,s->services->user);NEXT();
    case 101: if(s->services&&s->services->create_proc)s->services->create_proc(a,i,s->services->user);NEXT();
    case 102: o=a->smart_target;if(o){int t1=av(i,0),t2=av(i,1),feet=((a->obj_control^o->obj_control)&WM_OBJ_FLIPH)==0;int choosehi=(av(i,2)==SRC_ATM_CLOSEST)?feet:!feet;int area=choosehi?(t1>t2?t1:t2):(t1<t2?t1:t2);set_target_offsets(a,o,(uint16_t)area);a->tgt_yoff=0;}NEXT();
    case 103: v=a->hit_blocker?av(i,2):((a->anim_mode&WM_ARCADE_MODE_STATUS)?av(i,0):av(i,1));s->pc++;a->ani_count=v;return v!=0;
    case 104: a->safe_time=av(i,0);NEXT();
    case 105: wm_arcade_anim_set_opp_player_mode(a,(uint16_t)av(i,0));NEXT();
    case 106: wm_arcade_anim_xflip_opp(a);NEXT();
    case 107: fp=word_field(a,at(i,0));if(fp)*fp=av(i,1);NEXT();
    case 108: o=a->who_i_hit;if(o&&!a->hit_blocker&&!o->dizzy){o->immobilize_time=av(i,0);o->x_vel=o->y_vel=o->z_vel=0;}NEXT();
    case 109: o=0;if(attached_pair(a,&o)){t=table_for(s,i,0);idx=(unsigned)o->wrestler_num;if(t&&idx<t->count&&t->entries[idx].value)o->obj_control^=WM_OBJ_FLIPH;}NEXT();
    case 110: o=0;if(!attached_pair(a,&o))o=a->who_i_hit;if(o){o->y_vel=av(i,1);o->x_vel=face_right(a)?av(i,0):-av(i,0);o->z_vel=face_down(a)?av(i,2):-av(i,2);}NEXT();
    case 111: v=av(i,0);o=0;if((a->anim_mode&WM_ARCADE_MODE_KEEPATTACHED)&&attached_pair(a,&o)&&!(o->anim_mode&WM_ARCADE_MODE_GHOST)&&o->y_vel<=0&&o->y_int<=o->ground_y+v){o->y_int=o->ground_y+v;o->y_fixed=o->y_int<<16;if(s->services&&s->services->code)s->services->code(a,"SMALL_BOUNCE",s->services->user);NEXT();}if(a->y_vel<=0&&a->y_int<=a->ground_y+v){a->y_int=a->ground_y+v;a->y_fixed=a->y_int<<16;if(s->services&&s->services->code)s->services->code(a,"SMALL_BOUNCE",s->services->user);NEXT();}HOLD();
    case 112: o=0;if(attached_pair(a,&o))o->x_vel=rel_x(a,av(i,0),av(i,1));NEXT();
    case 113: wm_arcade_anim_set_attach_from_whoihit(a);NEXT();
    case 114: a->combo_count++;o=a->who_i_hit;if(o)o->immobilize_time=30;NEXT();
    case 115: o=opp(a);if(a->combo_count){a->combo_count=0;if(o){o->immobilize_time=0;o->getup_time=0;o->delay_meter=10*60;}}else{a->combo_count=0;if(o){o->immobilize_time=80;o->anti_combo_time=pcnt(s);o->getup_time=0;o->delay_meter=10*60;}}NEXT();
    case 116: if(s->services&&s->services->add_move)s->services->add_move(a,av(i,0),av(i,1),av(i,2),s->services->user);NEXT();
    case 117: a->attack_type=av(i,0);v=av(i,1);if(v<0)v=30;a->attack_time=(uint16_t)(rtick(s)+v);NEXT();
    case 118: t=table_for(s,i,0);idx=(unsigned)a->wrestler_num;te=(t&&idx<t->count)?&t->entries[idx]:0;if(te&&te->text&&change_program(s,a,te->text,0))return 0;s->fault=118;return 1;
    case 119: if(a->rpt_count>=av(i,0)){if(!branch_n(s,a,i,1))s->fault=119;return 0;}NEXT();
    case 120: if(a->rpt_count<av(i,0)){if(!branch_n(s,a,i,1))s->fault=120;return 0;}NEXT();
    case 121: if(a->y_int<=a->ground_y){if(s->services&&s->services->code)s->services->code(a,"SMALL_BOUNCE",s->services->user);NEXT();}if(a->anim_mode&WM_ARCADE_MODE_STATUS)NEXT();HOLD();
    case 122: if(s->services&&s->services->draw_name)s->services->draw_name(a,av(i,0),s->services->user);NEXT();
    case 123: if(s->services&&s->services->set_allow_offscreen)s->services->set_allow_offscreen(80,s->services->user);NEXT();
    case 124: a->attachimg_frame=at(i,0);a->attachimg_xoff=av(i,1);a->attachimg_yoff=av(i,2);a->attachimg_zoff=av(i,3);if(s->services&&s->services->attach_image)s->services->attach_image(a,a->attachimg_frame,a->attachimg_xoff,a->attachimg_yoff,a->attachimg_zoff,s->services->user);NEXT();
    case 125: a->y_int=a->ground_y;a->y_fixed=a->y_int<<16;NEXT();
    case 126: HOLD();
    case 127: if(av(i,0)>=0)a->scroll_y=av(i,0);a->status_flags|=WM_STATUS_SCROLL_CTRL;if(s->services&&s->services->scroll_ctrl)s->services->scroll_ctrl(a,av(i,0),s->services->user);NEXT();
    case 128: a->climbing_thru=0;a->safe_time=1;NEXT();
    case 129: o=0;if(attached_pair(a,&o)){v=av(i,0);if(o->obj_control&WM_OBJ_FLIPH)v^=(WM_MOVE_LEFT|WM_MOVE_RIGHT);o->facing_dir=v;}NEXT();
    case 130: a->status_flags|=(uint32_t)av(i,0);NEXT();
    default: a->source_vm_fault=i->opcode;s->fault=i->opcode;return 1;
    }
#undef NEXT
#undef HOLD
}

void wm_source_anim_runtime_tick(wm_source_anim_runtime_t*s,wm_arcade_actor_t*a){unsigned budget=512;if(!s||!a||!s->program)return;if(a->anim_mode&WM_ARCADE_MODE_END)return;if(a->ani_count>0){a->ani_count--;if(a->ani_count>0)return;}while(budget--&&s->program&&s->pc<s->program->count){const wm_source_anim_ins_t*i=&s->program->ins[s->pc];s->instructions_executed++;if(i->kind==WM_SRC_INS_FRAME){uint32_t ticks=((uint32_t)i->ticks*(uint32_t)(a->ani_speed?a->ani_speed:0x100u))>>8;s->current_frame=i->name;s->pc++;a->ani_count=(int32_t)(ticks?ticks:1u);return;}if(exec_cmd(s,a,i))return;if(s->fault){a->source_vm_fault=s->fault;return;}}if(!budget){s->fault=-2;a->source_vm_fault=-2;}else if(s->program&&s->pc>=s->program->count){a->anim_mode|=WM_ARCADE_MODE_END;}}
const char *wm_source_anim_runtime_frame(const wm_source_anim_runtime_t*s){return s?s->current_frame:0;}
const char *wm_source_anim_runtime_label(const wm_source_anim_runtime_t*s){return(s&&s->program)?s->program->label:0;}
