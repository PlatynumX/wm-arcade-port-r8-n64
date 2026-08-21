#ifndef WM_ARCADE_DAMAGE_H
#define WM_ARCADE_DAMAGE_H

/*
 * DAMAGE.EQU translated as integer C expressions.  Keeping the arithmetic in
 * expression form preserves the original integer truncation.
 */
#define WM_D_PUNCH         8
#define WM_RD_PUNCH        (WM_D_PUNCH*2/3)
#define WM_D_HDBUTT        12
#define WM_RD_HDBUTT       (WM_D_HDBUTT*2/3)
#define WM_D_HDBUTT2       9
#define WM_RD_HDBUTT2      (WM_D_HDBUTT2*2/3)
#define WM_D_KICK          13
#define WM_RD_KICK         (WM_D_KICK*2/3)
#define WM_D_FLYKICK       28
#define WM_RD_FLYKICK      (WM_D_FLYKICK*2/3)
#define WM_D_GRABTHROW     0
#define WM_RD_GRABTHROW    (WM_D_GRABTHROW*2/3)
#define WM_D_UPRCUT        20
#define WM_RD_UPRCUT       (WM_D_UPRCUT*2/3)
#define WM_D_LBDROP        17
#define WM_RD_LBDROP       (WM_D_LBDROP*2/3)
#define WM_D_LBDROP2       9
#define WM_RD_LBDROP2      (WM_D_LBDROP2*2/3)
#define WM_D_GRBHOLD       0
#define WM_RD_GRBHOLD      (WM_D_GRBHOLD*2/3)
#define WM_D_GRBFLNG       0
#define WM_RD_GRBFLNG      (WM_D_GRBFLNG*2/3)
#define WM_D_PUSH          1
#define WM_RD_PUSH         (WM_D_PUSH*2/3)
#define WM_D_BIGBOOT       18
#define WM_RD_BIGBOOT      (WM_D_BIGBOOT*2/3)
#define WM_D_KNEE          12
#define WM_RD_KNEE         (WM_D_KNEE*2/3)
#define WM_D_GRAPPLE       0
#define WM_RD_GRAPPLE      (WM_D_GRAPPLE*2/3)
#define WM_D_BOXPUNCH      20
#define WM_RD_BOXPUNCH     (WM_D_BOXPUNCH*2/3)
#define WM_D_STOMP         8
#define WM_RD_STOMP        (WM_D_STOMP*2/3)
#define WM_D_STOMP2        6
#define WM_RD_STOMP2       (WM_D_STOMP2*2/3)
#define WM_D_SPINKIK       17
#define WM_RD_SPINKIK      (WM_D_SPINKIK*2/3)
#define WM_D_CLINE         20
#define WM_RD_CLINE        (WM_D_CLINE*2/3)
#define WM_D_HEDHOLD       0
#define WM_RD_HEDHOLD      (WM_D_HEDHOLD*2/3)
#define WM_D_JUMPKICK      20
#define WM_RD_JUMPKICK     (WM_D_JUMPKICK*2/3)
#define WM_D_RUN           0
#define WM_RD_RUN          (WM_D_RUN*2/3)
#define WM_D_PUPPET        0
#define WM_RD_PUPPET       (WM_D_PUPPET*2/3)
#define WM_D_BCKHAND       19
#define WM_RD_BCKHAND      (WM_D_BCKHAND*2/3)
#define WM_D_BUZZ          25
#define WM_RD_BUZZ         (WM_D_BUZZ*2/3)
#define WM_D_HAYMAKER      23
#define WM_RD_HAYMAKER     (WM_D_HAYMAKER*2/3)
#define WM_D_BLBDROP       20
#define WM_RD_BLBDROP      (WM_D_BLBDROP*2/3)
#define WM_D_BSTOMP        17
#define WM_RD_BSTOMP       (WM_D_BSTOMP*2/3)
#define WM_D_HDKNEES       8
#define WM_RD_HDKNEES      (WM_D_HDKNEES*2/3)
#define WM_D_EARSLAP1      20
#define WM_RD_EARSLAP1     (WM_D_EARSLAP1*2/3)
#define WM_D_EARSLAP2      8
#define WM_RD_EARSLAP2     (WM_D_EARSLAP2*2/3)
#define WM_D_HAMMER1       12
#define WM_RD_HAMMER1      (WM_D_HAMMER1*2/3)
#define WM_D_HAMMER2       5
#define WM_RD_HAMMER2      (WM_D_HAMMER2*2/3)
#define WM_D_BUTTSTOMP     28
#define WM_RD_BUTTSTOMP    (WM_D_BUTTSTOMP*2/3)
#define WM_D_ATT31         0
#define WM_RD_ATT31        (WM_D_ATT31*2/3)
#define WM_D_ATT32         0
#define WM_RD_ATT32        (WM_D_ATT32*2/3)
#define WM_D_ATT33         0
#define WM_RD_ATT33        (WM_D_ATT33*2/3)
#define WM_D_TOMB          8
#define WM_RD_TOMB         (WM_D_TOMB*2/3)
#define WM_D_BIGKNEE       17
#define WM_RD_BIGKNEE      (WM_D_BIGKNEE*2/3)
#define WM_D_FLPKIK        0
#define WM_RD_FLPKIK       (WM_D_FLPKIK*2/3)
#define WM_D_SPDKIK        10
#define WM_RD_SPDKIK       (WM_D_SPDKIK*1/3)
#define WM_D_SPDKIK2       9
#define WM_RD_SPDKIK2      (WM_D_SPDKIK2*2/3)
#define WM_D_HITCK         0
#define WM_RD_HITCK        (WM_D_HITCK*2/3)
#define WM_D_ARMBRK        17
#define WM_RD_ARMBRK       (WM_D_ARMBRK*2/3)
#define WM_D_RSLASH        14
#define WM_RD_RSLASH       (WM_D_RSLASH*2/3)
#define WM_D_HEADDSLASH    8
#define WM_RD_HEADDSLASH   (WM_D_HEADDSLASH*2/3)
#define WM_D_HEADUSLASH    8
#define WM_RD_HEADUSLASH   (WM_D_HEADUSLASH*2/3)
#define WM_D_RSLASH2       6
#define WM_RD_RSLASH2      (WM_D_RSLASH2*2/3)
#define WM_D_HDBUTT_STAY   5
#define WM_RD_HDBUTT_STAY  (WM_D_HDBUTT_STAY*2/3)
#define WM_D_FIRE_PUNCH    10
#define WM_RD_FIRE_PUNCH   (WM_D_FIRE_PUNCH*2/3)
#define WM_D_SALT          15
#define WM_RD_SALT         (WM_D_SALT*2/3)
#define WM_D_BSTOMP2       20
#define WM_RD_BSTOMP2      (WM_D_BSTOMP2*2/3)
#define WM_D_GUTPUSH       15
#define WM_RD_GUTPUSH      (WM_D_GUTPUSH*2/3)
#define WM_D_PUNCH2        6
#define WM_RD_PUNCH2       (WM_D_PUNCH2*2/3)
#define WM_D_GATE_CRASH    20
#define WM_RD_GATE_CRASH   (WM_D_GATE_CRASH*2/3)
#define WM_D_SHOOTER       4
#define WM_RD_SHOOTER      (WM_D_SHOOTER*2/3)
#define WM_D_NAPALM        20
#define WM_RD_NAPALM       (WM_D_NAPALM*2/3)

/* Puppet/slam sequence damage. */
#define WM_D_BSLAM         (20*135/100)
#define WM_RD_BSLAM        (WM_D_BSLAM*2/3)
#define WM_D_GSUPLEX       (22*135/100)
#define WM_RD_GSUPLEX      (WM_D_GSUPLEX*2/3)
#define WM_D_FSTEIN        (25*135/100)
#define WM_RD_FSTEIN       (WM_D_FSTEIN*2/3)
#define WM_D_HIPTOSS       (20*135/100)
#define WM_RD_HIPTOSS      (WM_D_HIPTOSS*2/3)
#define WM_D_PILEDRIVER    (25*135/100)
#define WM_RD_PILEDRIVER   (WM_D_PILEDRIVER*2/3)
#define WM_D_FACESLAM1     (20*135/100)
#define WM_RD_FACESLAM1    (WM_D_FACESLAM1*2/3)
#define WM_D_FACESLAM2     (6*135/100)
#define WM_RD_FACESLAM2    (WM_D_FACESLAM2*2/3)
#define WM_D_BACKBRKR      (25*135/100)
#define WM_RD_BACKBRKR     (WM_D_BACKBRKR*2/3)
#define WM_D_POGO1         (19*135/100)
#define WM_RD_POGO1        (WM_D_POGO1*2/3)
#define WM_D_POGO2         (4*130/100)
#define WM_RD_POGO2        (WM_D_POGO2*2/3)
#define WM_D_SCISSOR       (24*135/100)
#define WM_RD_SCISSOR      (WM_D_SCISSOR*2/3)
#define WM_D_KICKTOSS      (22*135/100)
#define WM_RD_KICKTOSS     (WM_D_KICKTOSS*2/3)
#define WM_D_NECKBRKR      (22*135/100)
#define WM_RD_NECKBRKR     (WM_D_NECKBRKR*2/3)
#define WM_D_NECKSLAM      (22*135/100)
#define WM_RD_NECKSLAM     (WM_D_NECKSLAM*2/3)
#define WM_D_RUGSLAM       (7*135/100)
#define WM_RD_RUGSLAM      (WM_D_RUGSLAM*2/3)
#define WM_D_HITCONCRETE   (12*135/100)
#define WM_RD_HITCONCRETE  (WM_D_HITCONCRETE*2/3)
#define WM_D_FLIPSLAM      (26*135/100)
#define WM_RD_FLIPSLAM     (WM_D_FLIPSLAM*2/3)
#define WM_D_FACERAKE      (16*135/100)
#define WM_RD_FACERAKE     (WM_D_FACERAKE*2/3)
#define WM_D_FACEDRIVER    (14*135/100)
#define WM_RD_FACEDRIVER   (WM_D_FACEDRIVER*2/3)

/* DAMAGE.EQU attack types used by drone/AI logic. */
enum wm_arcade_attack_type {
    WM_AT_PUNCH=0, WM_AT_HDBUTT=1, WM_AT_KICK=2, WM_AT_FLYKICK=3,
    WM_AT_GRABTHROW=4, WM_AT_UPRCUT=5, WM_AT_LBDROP=6, WM_AT_GRBHOLD=7,
    WM_AT_GRBFLNG=8, WM_AT_PUSH=9, WM_AT_BIGBOOT=10, WM_AT_KNEE=11,
    WM_AT_GRAPPLE=12, WM_AT_BOXPUNCH=13, WM_AT_STOMP=14, WM_AT_SPINKIK=15,
    WM_AT_CLINE=16, WM_AT_HEDHOLD=17, WM_AT_JUMPKICK=18, WM_AT_RUN=19,
    WM_AT_PUPPET=20, WM_AT_BCKHAND=21, WM_AT_BUZZ=22, WM_AT_HAYMAKER=23,
    WM_AT_BLBDROP=24, WM_AT_BSTOMP=25, WM_AT_HDKNEES=26, WM_AT_EARSLAP1=27,
    WM_AT_EARSLAP2=28, WM_AT_HAMMER1=29, WM_AT_HAMMER2=30,
    WM_AT_BUTTSTOMP=31,
    WM_AT_TOMB=35, WM_AT_BIGKNEE=36, WM_AT_FLPKIK=37, WM_AT_SPDKIK=38,
    WM_AT_SPDKIK2=39, WM_AT_HITCK=40, WM_AT_ARMBRK=41,
    WM_AT_HDBUTT_STAY=42, WM_AT_HAIR_PICKUP=43, WM_AT_BSLAM=44,
    WM_AT_GSUPLEX=45, WM_AT_FSTEIN=46, WM_AT_HIPTOSS=47,
    WM_AT_PILEDRIVER=48, WM_AT_FACESLAM1=49, WM_AT_FACESLAM2=50,
    WM_AT_BACKBRKR=51, WM_AT_POGO1=52, WM_AT_POGO2=53, WM_AT_KICKTOSS=54,
    WM_AT_NECKBRKR=55, WM_AT_NECKSLAM=56, WM_AT_LEAPING=57,
    WM_AT_MSL=58, WM_AT_NUM=59
};

#endif
