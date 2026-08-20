#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "wm_arcade_wrestler_port.h"
#include "test_roster_common.h"

static void actor_eq(const wm_arcade_actor_t *a,const wm_arcade_actor_t *b){assert(memcmp(a,b,sizeof(*a))==0);}
static void log_eq(const rt_t *a,const rt_t *b){assert(memcmp(a,b,sizeof(*a))==0);}
static wm_arcade_actor_t run_actor(void){wm_arcade_actor_t a;memset(&a,0,sizeof(a));a.player_mode=WM_PMODE_RUNNING;a.move_dir=WM_MOVE_RIGHT;a.facing_dir=WM_MOVE_RIGHT;a.new_facing_dir=WM_MOVE_RIGHT;a.but_val_down=WM_BTN_KICK;return a;}
static wm_arcade_actor_t normal_opp(void){wm_arcade_actor_t o;memset(&o,0,sizeof(o));o.player_mode=WM_PMODE_NORMAL;return o;}

int main(void)
{
    wm_arcade_roster_env_t env={1000,0,0,0,0};
    wm_arcade_actor_t a1,a2,o1,o2;
    rt_t d,u;
    wm_arcade_roster_callbacks_t dc,uc;
    wm_arcade_wrestler_port_bindings_t b;
    wm_arcade_roster_step_result_t ur;

    memset(&b,0,sizeof(b));

#define PROVE(NAME,PROFILE,MOVE,FIELD,STEP_ACTION,STEP_EXTERNAL) do { \
    rt_reset(&d);rt_reset(&u);dc=rt_callbacks(&d);uc=rt_callbacks(&u);b.FIELD=&uc; \
    a1=run_actor();o1=normal_opp();a2=a1;o2=o1; \
    int dr=(int)MOVE(&a1,&o1,&env,&dc); \
    ur=wm_arcade_move_ported_wrestler(&PROFILE,&a2,&o2,&env,&b); \
    assert(ur==(dr==(int)STEP_ACTION?WM_ROSTER_STEP_ACTION:dr==(int)STEP_EXTERNAL?WM_ROSTER_STEP_EXTERNAL:WM_ROSTER_STEP_IDLE)); \
    actor_eq(&a1,&a2);actor_eq(&o1,&o2);log_eq(&d,&u); \
    b.FIELD=NULL; \
} while(0)

    PROVE(Taker,wm_arcade_profile_taker,wm_arcade_move_taker,taker,WM_TAKER_STEP_ACTION,WM_TAKER_STEP_EXTERNAL);
    PROVE(Yoko,wm_arcade_profile_yoko,wm_arcade_move_yoko,yoko,WM_YOKO_STEP_ACTION,WM_YOKO_STEP_EXTERNAL);
    PROVE(Shawn,wm_arcade_profile_shawn,wm_arcade_move_shawn,shawn,WM_SHAWN_STEP_ACTION,WM_SHAWN_STEP_EXTERNAL);
    PROVE(Bam,wm_arcade_profile_bam,wm_arcade_move_bam,bam,WM_BAM_STEP_ACTION,WM_BAM_STEP_EXTERNAL);
    PROVE(Doink,wm_arcade_profile_doink,wm_arcade_move_doink,doink,WM_DOINK_STEP_ACTION,WM_DOINK_STEP_EXTERNAL);
    PROVE(Lex,wm_arcade_profile_lex,wm_arcade_move_lex,lex,WM_LEX_STEP_ACTION,WM_LEX_STEP_EXTERNAL);
#undef PROVE

    /* The unified charge API must enter each dedicated source module by its literal source-table label. */
    rt_reset(&u);uc=rt_callbacks(&u);b.taker=&uc;a1=normal_opp();o1=normal_opp();assert(wm_arcade_port_release_charge(&wm_arcade_profile_taker,&a1,&o1,"button_hold",110,&b));assert(strcmp(u.special,"scrt_spirit")==0);b.taker=NULL;
    rt_reset(&u);uc=rt_callbacks(&u);b.yoko=&uc;a1=normal_opp();o1=normal_opp();a1.facing_dir=WM_MOVE_RIGHT;assert(wm_arcade_port_release_charge(&wm_arcade_profile_yoko,&a1,&o1,"charge_salt",85,&b));assert(strcmp(u.anim,"yok_2_salt_anim")==0);b.yoko=NULL;
    rt_reset(&u);uc=rt_callbacks(&u);b.shawn=&uc;a1=normal_opp();o1=normal_opp();assert(wm_arcade_port_release_charge(&wm_arcade_profile_shawn,&a1,&o1,"charge_flying_kick",85,&b));assert(strcmp(u.anim,"shn_flying_kick_anim")==0);b.shawn=NULL;
    rt_reset(&u);uc=rt_callbacks(&u);b.bam=&uc;a1=normal_opp();o1=normal_opp();a1.facing_dir=WM_MOVE_RIGHT;assert(wm_arcade_port_release_charge(&wm_arcade_profile_bam,&a1,&o1,"firepnch",85,&b));assert(strcmp(u.anim,"bam_2_fpunch_anim")==0);b.bam=NULL;
    rt_reset(&u);uc=rt_callbacks(&u);b.doink=&uc;a1=normal_opp();o1=normal_opp();a1.facing_dir=WM_MOVE_RIGHT;a1.new_facing_dir=WM_MOVE_RIGHT;a1.stick_val_cur=0;assert(wm_arcade_port_release_charge(&wm_arcade_profile_doink,&a1,&o1,"charge_buzz",100,&b));assert(strcmp(u.anim,"dnk_2_buzz_anim")==0);b.doink=NULL;
    rt_reset(&u);uc=rt_callbacks(&u);b.lex=&uc;a1=normal_opp();o1=normal_opp();a1.facing_dir=WM_MOVE_RIGHT;assert(wm_arcade_port_release_charge(&wm_arcade_profile_lex,&a1,&o1,"charge_clobber",100,&b));assert(strcmp(u.anim,"lex_2_clobber_anim")==0);b.lex=NULL;

    /* Cross-character labels are rejected instead of substituted. */
    assert(!wm_arcade_port_release_charge(&wm_arcade_profile_lex,&a1,&o1,"charge_buzz",100,&b));
    assert(!wm_arcade_port_fire_secret(&wm_arcade_profile_taker,&a1,&o1,"sliding_elbow",1000,&b));

    puts("Stage 23 six dedicated wrestler direct-port modules: PASS");
    return 0;
}
