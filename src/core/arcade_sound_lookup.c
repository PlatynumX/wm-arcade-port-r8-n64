#include "wm/arcade_sound_lookup.h"
#include "wm_arcade_combat.h"
#include <string.h>
#include <stddef.h>

typedef struct { const char *name; uint16_t index; } move_name;
static const move_name moves[] = {
    {"PUNCH_T1",0},{"PUNCH_T2",1},{"PUNCH_L1",2},{"PUNCH_L2",3},
    {"HDBUTT_T1",4},{"HDBUTT_T2",5},{"HDBUTT_L1",6},{"HDBUTT_L2",7},
    {"KICK_T1",8},{"KICK_T2",9},{"KICK_L1",10},{"KICK_L2",11},
    {"FLYKICK_T1",12},{"FLYKICK_T2",13},{"FLYKICK_L1",14},{"FLYKICK_L2",15},
    {"GRABTHROW_T1",16},{"GRABTHROW_T2",17},{"GRABTHROW_L1",18},{"GRABTHROW_L2",19},
    {"UPRCUT_T1",20},{"UPRCUT_T2",21},{"UPRCUT_L1",22},{"UPRCUT_L2",23},
    {"LBOWDROP_T1",24},{"LBOWDROP_T2",25},{"LBOWDROP_L1",26},{"LBOWDROP_L2",27},
    {"GRABHOLD_T1",28},{"GRABHOLD_T2",29},{"GRABHOLD_L1",30},{"GRABHOLD_L2",31},
    {"GRABFLING_T1",32},{"GRABFLING_T2",33},{"GRABFLING_L1",34},{"GRABFLING_L2",35},
    {"PUSH_T1",36},{"PUSH_T2",37},{"PUSH_L1",38},{"PUSH_L2",39},
    {"HIPTOSS_T1",40},{"HIPTOSS_T2",41},{"HIPTOSS_L1",42},{"HIPTOSS_L2",43},
    {"SPUNCH_T1",48},{"SPUNCH_T2",49},{"SPUNCH_L1",50},{"SPUNCH_L2",51},
    {"TURNDIVE_T1",52},{"TURNDIVE_T2",53},{"RUGSLAM_YELL",54},{"RUGSLAM_IMPACT",55},
    {"RSLASH_L1",56},{"RSLASH_L2",57},{"YELL_THROW",58}
};
static int lookup(const char *name, size_t n) {
    size_t i;
    for (i=0;i<sizeof(moves)/sizeof(moves[0]);++i)
        if (strlen(moves[i].name)==n && memcmp(moves[i].name,name,n)==0) return (int)moves[i].index;
    return -1;
}
bool wm_arcade_sound_wrsnd(wm_arcade_sound *s, uint8_t wrestler, uint16_t sound_index) {
    if (!s || wrestler >= 9u || sound_index > 58u) return false;
    return wm_sound_wrtable_sound(s, sound_index, wrestler).played;
}
bool wm_arcade_sound_wrsnd_pair(wm_arcade_sound *s, uint8_t wrestler, uint16_t a, int b) {
    bool r=wm_arcade_sound_wrsnd(s,wrestler,a);
    if (b>=0) r=wm_arcade_sound_wrsnd(s,wrestler,(uint16_t)b)||r;
    return r;
}
bool wm_arcade_sound_play_label(wm_arcade_sound *s, const struct wm_arcade_actor *actor, const char *label) {
    const wm_arcade_actor_t *a=(const wm_arcade_actor_t *)actor;
    const char *slash;
    int i0,i1;
    if(!s||!a||!label||a->wrestler_num<0||a->wrestler_num>8)return false;
    if(strcmp(label,"PUNCH")==0)return wm_arcade_sound_wrsnd_pair(s,(uint8_t)a->wrestler_num,0,1);
    if(strcmp(label,"HDBUTT")==0)return wm_arcade_sound_wrsnd_pair(s,(uint8_t)a->wrestler_num,4,5);
    if(strcmp(label,"KICK")==0)return wm_arcade_sound_wrsnd_pair(s,(uint8_t)a->wrestler_num,8,9);
    if(strcmp(label,"FLYKICK")==0)return wm_arcade_sound_wrsnd_pair(s,(uint8_t)a->wrestler_num,12,13);
    if(strcmp(label,"UPRCUT")==0)return wm_arcade_sound_wrsnd_pair(s,(uint8_t)a->wrestler_num,20,21);
    if(strcmp(label,"LBOWDROP")==0)return wm_arcade_sound_wrsnd_pair(s,(uint8_t)a->wrestler_num,24,25);
    if(strcmp(label,"GRABHOLD")==0)return wm_arcade_sound_wrsnd_pair(s,(uint8_t)a->wrestler_num,28,29);
    if(strcmp(label,"GRABFLING")==0)return wm_arcade_sound_wrsnd_pair(s,(uint8_t)a->wrestler_num,32,33);
    if(strcmp(label,"PUSH")==0)return wm_arcade_sound_wrsnd_pair(s,(uint8_t)a->wrestler_num,36,37);
    if(strcmp(label,"HIPTOSS")==0)return wm_arcade_sound_wrsnd_pair(s,(uint8_t)a->wrestler_num,40,41);
    if(strcmp(label,"SPUNCH")==0)return wm_arcade_sound_wrsnd_pair(s,(uint8_t)a->wrestler_num,48,49);
    if(strcmp(label,"TURNDIVE")==0)return wm_arcade_sound_wrsnd_pair(s,(uint8_t)a->wrestler_num,52,53);
    /* DCSSOUND.ASM::BLOCK_WOOSH is exactly triple_sound(0x16). */
    if(strcmp(label,"BLOCK_WOOSH")==0)return wm_sound_triple_sound(s,0x16u).played;
    slash=strchr(label,'/');
    if(!slash){i0=lookup(label,strlen(label));return i0>=0?wm_arcade_sound_wrsnd(s,(uint8_t)a->wrestler_num,(uint16_t)i0):false;}
    i0=lookup(label,(size_t)(slash-label)); i1=lookup(slash+1,strlen(slash+1));
    if(i0<0||i1<0)return false;
    return wm_arcade_sound_wrsnd_pair(s,(uint8_t)a->wrestler_num,(uint16_t)i0,i1);
}
