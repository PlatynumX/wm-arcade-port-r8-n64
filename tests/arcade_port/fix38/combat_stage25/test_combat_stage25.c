#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wm_arcade_drone.h"

typedef struct T {
    uint32_t r[64]; int n, p;
    const char *last;
    const char *called;
    int seek_result;
} T;

static uint32_t rnext(uint32_t m, void *u) {
    T *t = (T *)u;
    uint32_t v = t->r[t->p < t->n ? t->p++ : (t->n ? t->n - 1 : 0)];
    return m ? v % (m + 1u) : 0u;
}
static wm_arcade_actor_t *cl(wm_arcade_actor_t *a, void *u){(void)u;return a->smart_target;}
static wm_arcade_actor_t *clf(wm_arcade_actor_t *a, void *u){(void)u;return a->smart_target;}
static int32_t cdf(const wm_arcade_actor_t *a,void*u){(void)u;return a->closest_dist;}
static int bb(int s,void*u){(void)s;(void)u;return 100;}
static int ba(int m,void*u){(void)m;(void)u;return 0;}
static int hd(int s,void*u){(void)s;(void)u;return 0;}
static int combo(wm_arcade_actor_t*a,void*u){(void)a;(void)u;return 0;}
static void sel(wm_arcade_actor_t*a,const char*l,void*u){(void)a;((T*)u)->last=l;}
static int sseek(wm_arcade_actor_t*a,wm_arcade_actor_t*o,wm_arcade_drone_state_t*d,const char*l,void*u){(void)a;(void)o;(void)d;(void)l;return ((T*)u)->seek_result;}
static int spct(const char*l,int s,void*u){(void)l;(void)s;(void)u;return 100;}
static int scall(wm_arcade_actor_t*a,wm_arcade_actor_t*o,wm_arcade_drone_state_t*d,const char*l,void*u){(void)a;(void)o;(void)d;((T*)u)->called=l;return WM_DRONE_CALL_CONTINUE;}

static const char *short_scripts[] = {"short_exact", "short_alt"};
static const wm_arcade_drone_script_list_t short_list = {1, short_scripts, 2};
static const wm_arcade_drone_script_list_t *range_list(
    const wm_arcade_actor_t*a,const wm_arcade_actor_t*b,int band,int mm,int om,void*u){
    (void)a;(void)b;(void)mm;(void)om;(void)u;
    return band==0 ? &short_list : NULL;
}

static const wm_arcade_drone_script_op_t vm_ops[] = {
    {WM_DRONE_SC_CALL_CODE,0,0,0,0,"call_exact"},
    {WM_DRONE_SC_RANDOM_JUMP,0,0,100,3,NULL},
    {WM_DRONE_SC_DONE,0,0,0,0,NULL},
    {WM_DRONE_SC_INPUT,(uint16_t)(WM_BTN_PUNCH | (WM_MOVE_RIGHT<<5)),4,0,0,NULL},
    {WM_DRONE_SC_DONE,0,0,0,0,NULL}
};
static const wm_arcade_drone_script_t vm_script = {"vm_exact",vm_ops,sizeof(vm_ops)/sizeof(vm_ops[0])};
static const wm_arcade_drone_script_op_t seek_ops[] = {
    {WM_DRONE_SC_SEEK,0,0,0,0,NULL},
    {WM_DRONE_SC_INPUT,(uint16_t)(WM_BTN_KICK | (WM_MOVE_LEFT<<5)),2,0,0,NULL}
};
static const wm_arcade_drone_script_t seek_script = {"seek_exact",seek_ops,2};
static const wm_arcade_drone_script_t *resolve(const char*l,void*u){
    (void)u;
    if(l && strcmp(l,"vm_exact")==0) return &vm_script;
    if(l && strcmp(l,"seek_exact")==0) return &seek_script;
    return NULL;
}

static wm_arcade_drone_callbacks_t mkcb(T*t){
    wm_arcade_drone_callbacks_t c;
    memset(&c,0,sizeof(c));
    c.rnd_upto=rnext;c.rndrng0_upto=rnext;
    c.closest_actor=cl;c.closest_actor_for=clf;c.closest_dist_for=cdf;
    c.block_base_pct=bb;c.block_attack_pct=ba;
    c.headhold_delay_max=hd;c.headheld_delay_max=hd;
    c.check_combo_go=combo;c.range_script_list=range_list;
    c.resolve_script=resolve;c.script_skill_pct=spct;c.script_seek=sseek;c.script_call=scall;
    c.script_selected=sel;c.user=t;
    return c;
}

static void init_pair(wm_arcade_actor_t *a, wm_arcade_actor_t *b) {
    memset(a,0,sizeof(*a)); memset(b,0,sizeof(*b));
    a->active=b->active=1;
    a->player_mode=b->player_mode=WM_PMODE_NORMAL;
    a->smart_target=b;b->smart_target=a;
    a->player_side=0;b->player_side=1;
    a->closest_dist=50;a->closest_xdist=50;a->closest_zdist=10;
    a->facing_dir=WM_MOVE_RIGHT;
}

int main(void){
    wm_arcade_actor_t a,b; wm_arcade_actor_t *arr[2]={&a,&b};
    wm_arcade_drone_state_t d; wm_arcade_drone_world_t w; T t;
    wm_arcade_drone_callbacks_t c;

    init_pair(&a,&b); memset(&t,0,sizeof(t));
    w.actors=arr;w.actor_count=2;w.pcnt=1;w.round_tickcount=100;w.first_ladder=0;
    c=mkcb(&t);wm_arcade_drone_init(&d,10);
    assert(wm_arcade_drone_getup_pct(0)==10);assert(wm_arcade_drone_getup_pct(29)==100);

    /* Range selection is source-list based, not an AI heuristic. */
    t.r[0]=0;t.n=1;t.p=0;
    assert(wm_arcade_drone_main(&a,&d,&w,&c)==WM_DRONE_STEP_SCRIPT);
    assert(t.last && strcmp(t.last,"short_exact")==0);

    /* Source self-mode scripts. */
    d.script=NULL;t.last=NULL;a.player_mode=WM_PMODE_ONGROUND;
    assert(wm_arcade_drone_main(&a,&d,&w,&c)==WM_DRONE_STEP_SCRIPT);
    assert(t.last && strcmp(t.last,"drn_roll")==0);

    /* Normal attack block: X<=180, Z<=100; attack-specific miss slot. */
    d.script=NULL;t.last=NULL;a.player_mode=WM_PMODE_NORMAL;
    b.attack_type=WM_AT_KICK;b.attack_time=110;b.getup_time=0;
    a.closest_xdist=60;a.closest_zdist=90;t.r[0]=0;t.n=1;t.p=0;
    assert(wm_arcade_drone_main(&a,&d,&w,&c)==WM_DRONE_STEP_BLOCK);
    assert((d.but & WM_BTN_BLOCK)!=0);assert(d.delay>=15);

    /* Missile source path bypasses X limit but tightens Z to 50. */
    wm_arcade_drone_init(&d,10); b.attack_type=WM_AT_MSL;b.attack_time=110;
    a.closest_xdist=999;a.closest_zdist=50;t.r[0]=0;t.p=0;
    assert(wm_arcade_drone_main(&a,&d,&w,&c)==WM_DRONE_STEP_BLOCK);

    /* Missed blocks are tracked per attack type, source atkcnt_t semantics. */
    wm_arcade_drone_init(&d,10); c.block_base_pct=NULL; c.block_attack_pct=NULL;
    b.attack_type=WM_AT_KICK;b.attack_time=110;a.closest_xdist=60;a.closest_zdist=20;
    t.r[0]=99;t.p=0;
    (void)wm_arcade_drone_main(&a,&d,&w,&c);
    assert(d.missed_blocks[WM_AT_KICK]==1);assert(d.missed_blocks[WM_AT_PUNCH]==0);
    c=mkcb(&t);

    /* GETUP_TIME always aborts script; successful roll also subtracts five. */
    wm_arcade_drone_init(&d,10);d.script="vm_exact";a.getup_time=7;b.attack_time=0;t.r[0]=0;t.p=0;
    assert(wm_arcade_drone_main(&a,&d,&w,&c)==WM_DRONE_STEP_ABORT_SCRIPT);
    assert(a.getup_time==2 && d.script==NULL);
    d.script="vm_exact";a.getup_time=7;t.r[0]=99;t.p=0;
    assert(wm_arcade_drone_main(&a,&d,&w,&c)==WM_DRONE_STEP_ABORT_SCRIPT);
    assert(a.getup_time==7 && d.script==NULL);
    a.getup_time=0;

    /* Decoded source script VM: call, random jump, packed input. */
    wm_arcade_drone_init(&d,10);d.script="vm_exact";d.script_mode=WM_PMODE_NORMAL;
    t.r[0]=0;t.n=1;t.p=0;t.called=NULL;
    assert(wm_arcade_drone_script_step(&a,&b,&d,&vm_script,&c)==WM_DRONE_STEP_INPUT);
    assert(t.called && strcmp(t.called,"call_exact")==0);
    assert(d.but==WM_BTN_PUNCH && d.joy==WM_MOVE_RIGHT && d.delay==4 && d.script_pc==4);

    /* Facing-left source decode flips script left/right directions. */
    wm_arcade_drone_init(&d,10);d.script="seek_exact";d.script_mode=WM_PMODE_NORMAL;
    a.facing_dir=0;t.seek_result=0;
    assert(wm_arcade_drone_script_step(&a,&b,&d,&seek_script,&c)==WM_DRONE_STEP_INPUT);
    assert(d.but==WM_BTN_KICK && d.joy==WM_MOVE_RIGHT && d.delay==2);

    d.but=WM_BTN_PUNCH;d.but_charge=0;wm_arcade_drone_commit_inputs(&a,&d,0,0);
    assert(a.but_val_down==WM_BTN_PUNCH);

    puts("stage25 drone AI core + script VM tests: PASS");
    return 0;
}
