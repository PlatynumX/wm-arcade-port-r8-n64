/*
 * Two ANI_CODE routines from REACT1/REACT5 and the one place SETMODE's
 * rule about MODE_DEAD had been dropped.
 *
 * REACT1.ASM:1901 #dead_or_dying is the guard on
 * xxx_aborted_attach_anim: a wrestler whose puppet sequence was
 * interrupted normally gets up, and this decides he does not.
 *
 * REACT5.ASM:409 hit_puppet_even_if_dead is the attach every puppet
 * move goes through. Its name is about the CALLER's dead check, which
 * the source commented out -- not about SETMODE, which still refuses
 * to write over MODE_DEAD. BAMSEQ2.ASM:1224 #attach_victim wrote the
 * mode unconditionally and never turned the victim's collisions off.
 */
#include "wm/anim_program.h"
#include "wm/arcade/wm_arcade_combat_defs.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_dead_or_dying(void)
{
    wm_arcade_actor_t a;

    /* Healthy and not marked: MODE_STATUS comes off and stays off, so
       ANI_IFSTATUS falls through to the getup table. */
    memset(&a, 0, sizeof(a));
    a.life = 100;
    a.anim_mode = (uint16_t)WM_MODE_STATUS;
    assert(wm_anim_code_run(&a, NULL, "#dead_or_dying", "REACT1.ASM", 0));
    assert((a.anim_mode & WM_MODE_STATUS) == 0);

    /* Already dead. */
    a.life = 0;
    a.anim_mode = 0;
    assert(wm_anim_code_run(&a, NULL, "#dead_or_dying", "REACT1.ASM", 0));
    assert((a.anim_mode & WM_MODE_STATUS) != 0);

    /* Alive but owing a death -- I_WILL_DIE, adjust_health's deferral. */
    a.life = 100;
    a.i_will_die = 180;
    a.anim_mode = 0;
    assert(wm_anim_code_run(&a, NULL, "#dead_or_dying", "REACT1.ASM", 0));
    assert((a.anim_mode & WM_MODE_STATUS) != 0);

    /*
     * The unconditional clear at the top is not redundant: whatever the
     * previous opcode left in the flag would otherwise decide this.
     */
    a.life = 100;
    a.i_will_die = 0;
    a.anim_mode = (uint16_t)(WM_MODE_STATUS | WM_MODE_CHECKHIT);
    assert(wm_anim_code_run(&a, NULL, "#dead_or_dying", "REACT1.ASM", 0));
    assert((a.anim_mode & WM_MODE_STATUS) == 0);
    assert((a.anim_mode & WM_MODE_CHECKHIT) != 0);   /* nothing else touched */
}

static void test_attach_victim_keeps_mode_dead(void)
{
    wm_arcade_actor_t bam, victim;

    memset(&bam, 0, sizeof(bam));
    memset(&victim, 0, sizeof(victim));
    bam.who_i_hit = &victim;

    /* A live victim becomes a PUPPET. */
    victim.player_mode = (uint16_t)WM_PMODE_NORMAL;
    victim.getup_time = 40;
    victim.anim_mode = (uint16_t)WM_MODE_CHECKHIT;
    assert(wm_anim_code_run(&bam, NULL, "#attach_victim", "BAMSEQ2.ASM", 1224));
    assert(victim.player_mode == (uint16_t)WM_PMODE_PUPPET);
    assert(victim.attach_proc == &bam);
    assert(bam.attach_proc == &victim);
    assert(victim.getup_time == 0);
    /* REACT5.ASM:421's `#done calla wres_collis_off`, which the port
       used to leave out. */
    assert((victim.anim_mode & WM_MODE_CHECKHIT) == 0);

    /*
     * A dead one attaches and stays dead. MACROS.H's SETMODE opens with
     * `cmpi MODE_DEAD,a0 / jreq done?`, so MODE_DEAD is immutable, and
     * "even if dead" never meant otherwise.
     */
    memset(&bam, 0, sizeof(bam));
    memset(&victim, 0, sizeof(victim));
    bam.who_i_hit = &victim;
    victim.player_mode = (uint16_t)WM_PMODE_DEAD;
    victim.anim_mode = (uint16_t)WM_MODE_CHECKHIT;
    assert(wm_anim_code_run(&bam, NULL, "#attach_victim", "BAMSEQ2.ASM", 1224));
    assert(victim.player_mode == (uint16_t)WM_PMODE_DEAD);
    assert(victim.attach_proc == &bam);
    assert(bam.attach_proc == &victim);
    assert((victim.anim_mode & WM_MODE_CHECKHIT) == 0);

    /* Nothing to attach to. */
    memset(&bam, 0, sizeof(bam));
    assert(wm_anim_code_run(&bam, NULL, "#attach_victim", "BAMSEQ2.ASM", 1224));
    assert(bam.attach_proc == NULL);
}

int main(void)
{
    test_dead_or_dying();
    test_attach_victim_keeps_mode_dead();
    printf("#dead_or_dying and hit_puppet_even_if_dead: all checks passed\n");
    return 0;
}
