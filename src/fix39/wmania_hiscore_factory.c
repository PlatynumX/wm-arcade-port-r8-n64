#include "wmania_hiscore_factory.h"

#define F(score_, a,b,c,d,e) { (score_), { (a),(b),(c),(d),(e) } }
#define SP ' '

static const WmHsFactoryEntry streak_factory[] = {
    F(0x22122145u,'E','P','J','A','.'),
    F(0x00000011u,'M','J','T','A','.'),
    F(0x00000010u,'S','A','L','B','.'),
    F(0x00000009u,'J','M','S','C','.'),
    F(0x00000009u,'J','Y','T','D','.'),
    F(0x00000008u,'M','J','L','E','.'),
    F(0x00000008u,'J','A','K','F','.'),
    F(0x00000007u,'O','E','G','A','.'),
    F(0x00000007u,'S','L',SP,'A','.'),
    F(0x00000007u,'M','D','P','A','.'),
    F(0x00000006u,'G','B','S','A','.'),
    F(0x00000006u,'D','J','T','A','.'),
    F(0x00000006u,'E','P','J','A','.'),
    F(0x00000005u,'B','I','F','A','.'),
    F(0x00000005u,'U','N','K','A','.'),
    F(0x00000005u,'U','T','B','A','.'),
    F(0x00000004u,'C','R','L','A','.'),
    F(0x00000004u,'T','D','G','A','.'),
    F(0x00000004u,'A','S','B','A','.')
};

static const WmHsFactoryEntry pin_speed_factory[] = {
    F(0x00007000u,'E','P','J','A','.'),
    F(0x00006000u,'M','J','T','I','.'),
    F(0x00006100u,'S','A','L','I','.'),
    F(0x00006200u,'J','M','S','G','.'),
    F(0x00006300u,'J','Y','T','F','.'),
    F(0x00006400u,'J','A','K','E','.'),
    F(0x00006500u,'O','E','G','D','.'),
    F(0x00006600u,'M','J','L','C','.'),
    F(0x00006700u,'U','T','B','B','.'),
    F(0x00006800u,'A','S','B','A','.')
};

static const WmHsFactoryEntry beaten_factory[] = {
    F(0x22122145u,'E','P','J','A','B'),
    F(0x00011101u,'M','I','K','E',SP),
    F(0x00001001u,'J','A','M','I','T'),
    F(0x00000100u,'T','E','A','L',SP),
    F(0x00001000u,'D','I','N','K',SP),
    F(0x00010000u,'J','A','K','E',SP),
    F(0x00100000u,'D','R','J',SP,SP),
    F(0x01000000u,'C','H','I','C','K'),
    F(0x10000000u,'S','H','A','W','N'),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP)
};

static const WmHsFactoryEntry inter_factory[] = {
    F(0x22122145u,'E','P','J','A','B'),
    F(0x00000111u,'M','A','R','K',SP),
    F(0x00000101u,'S','A','L',SP,SP),
    F(0x00000100u,'J','A','S','O','N'),
    F(0x00001000u,'L','I','C','K',SP),
    F(0x00010000u,'J','A','K','E',SP),
    F(0x00100000u,'D','I','E','S','L'),
    F(0x01000000u,'F','U','J','I',SP),
    F(0x10000000u,'S','H','A','W','N'),
    F(0x01000000u,'B','I','F','F',SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP),
    F(0x00000000u,SP,SP,SP,SP,SP)
};

static const WmHsFactoryEntry tag_factory[] = {
    F(0x22122145u,'E','P','J','A','B'),
    F(0x00009000u,'B','O','O','N',SP),
    F(0x00009000u,'S','A','L',SP,SP),
    F(0x00009500u,'E','R','M','A','C'),
    F(0x00009500u,'K','A','N','O',SP),
    F(0x00010000u,'S','O','N','Y','A'),
    F(0x00010000u,'G','E','N','E',SP),
    F(0x00010500u,'M','I','K','E',SP),
    F(0x00010500u,'S','H','A','W','N'),
    F(0x00011000u,'J','A','S','O','N'),
    F(0x00011000u,'J','O','S','H',SP),
    F(0x00011500u,'J','O','H','N',SP),
    F(0x00011500u,'J','A','K','E',SP),
    F(0x00012000u,'J','O','E',SP,SP),
    F(0x00012000u,'E','D','W','I','N'),
    F(0x00012500u,'B','I','L','L',SP),
    F(0x00012500u,'M','A','R','K','P'),
    F(0x00013000u,'S','T','E','V','E'),
    F(0x00013000u,'T','O','N','Y',SP)
};

const WmHsTableTemplate wm_hs_streak_template = {
    "streak", streak_factory, WM_HS_STREAK_LAST_ENTRY, 18u, 3u,
    WM_HS_HIGHER_IS_BETTER, WM_HS_INSERT_NORMAL
};

const WmHsTableTemplate wm_hs_pin_speed_template = {
    "pin_speed", pin_speed_factory, WM_HS_PIN_SPEED_LAST_ENTRY, 9u, 2u,
    WM_HS_LOWER_IS_BETTER, WM_HS_INSERT_NORMAL
};

const WmHsTableTemplate wm_hs_beaten_template = {
    "beaten", beaten_factory, WM_HS_BEATEN_LAST_ENTRY, 30u, 6u,
    WM_HS_LOWER_IS_BETTER, WM_HS_INSERT_SPECIAL_BEATEN
};

const WmHsTableTemplate wm_hs_inter_template = {
    "inter", inter_factory, WM_HS_INTER_LAST_ENTRY, 30u, 6u,
    WM_HS_LOWER_IS_BETTER, WM_HS_INSERT_SPECIAL_BEATEN
};

const WmHsTableTemplate wm_hs_tag_template = {
    "tag", tag_factory, WM_HS_TAG_LAST_ENTRY, 18u, 3u,
    WM_HS_LOWER_IS_BETTER, WM_HS_INSERT_SPECIAL_TAG
};

#undef SP
#undef F
