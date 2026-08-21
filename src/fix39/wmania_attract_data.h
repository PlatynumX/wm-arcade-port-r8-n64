#ifndef WMANIA_ATTRACT_DATA_H
#define WMANIA_ATTRACT_DATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ATTRACT.ASM: NUM_HINTS .EQU 10. */
#define WM_ATTRACT_ACTIVE_HINTS 10u
#define WM_ATTRACT_WRESTLERS 8u
#define WM_FIX39_ATTRACT_BIO_ATTRIBUTES 4u
#define WM_FIX39_ATTRACT_COPYRIGHT_PAGE1_LINES 9u
#define WM_FIX39_ATTRACT_COPYRIGHT_PAGE2_LINES 10u
#define WM_FIX39_ATTRACT_AAMA_LINES 6u
#define WM_FIX39_ATTRACT_GENERAL_TIP_ROWS 11u

/*
 * Source strings stay identified by their original labels.  V11 corrects the
 * executable WHICH_HINT/NUM_HINTS data and carries source line counts and art
 * symbols, but does not replace unresolved source-text extraction with typed
 * substitute copy.
 */
typedef struct {
    const char *title_label;
    const char *body_label;
    const char *body_line_labels[6];
    const char *tip_image_symbol;
    const char *mug_image_symbol;
    const char *number_image_symbol;
    uint8_t number_image_index;
    uint8_t body_line_count;
} WmAttractHint;

typedef struct {
    const char *name;
    uint16_t halfwidth;
    const char *from_label;
    uint16_t weight_lbs;
    uint8_t height_ft;
    uint8_t height_in;
    const char *quote_label;
    const char *logo_symbol;
    uint8_t logo_width;
    uint8_t logo_height;
    uint16_t tune_id;
    const char *tips_table_label;
} WmAttractBio;

typedef struct {
    const char *background_symbol;
    const char *title_text_label;
    int16_t title_x;
    int16_t title_y;
    int16_t body_x;
    int16_t body_y;
    int16_t body_line_step;
    uint16_t initial_sleep_ticks;
    uint16_t wait_tsec_num;
    uint16_t wait_tsec_den;
} WmAttractScreenLayout;

extern const WmAttractHint wm_attract_hints[WM_ATTRACT_ACTIVE_HINTS];
extern const WmAttractBio wm_attract_bios[WM_ATTRACT_WRESTLERS];

/* Source-layout constants. */
extern const WmAttractScreenLayout wm_attract_general_tips_layout;
extern const WmAttractScreenLayout wm_attract_operator_layout;
extern const WmAttractScreenLayout wm_attract_aama_layout;

/* General fixed tip table source labels, exactly 11 visible rows + terminator. */
extern const char *const wm_attract_general_tip_labels[
    WM_FIX39_ATTRACT_GENERAL_TIP_ROWS + 1u];

/* Copyright uses two sequential pages of 9 and 10 source labels. */
extern const char *const wm_attract_copyright_page1_labels[
    WM_FIX39_ATTRACT_COPYRIGHT_PAGE1_LINES];
extern const char *const wm_attract_copyright_page2_labels[
    WM_FIX39_ATTRACT_COPYRIGHT_PAGE2_LINES];

/* AAMA source labels. */
extern const char *const wm_attract_aama_labels[WM_FIX39_ATTRACT_AAMA_LINES];

/* Bio source-render geometry. */
#define WM_FIX39_ATTRACT_BIO_BACKGROUND_SYMBOL "biopageBMOD"
#define WM_FIX39_ATTRACT_BIO_STORY_BACKGROUND_SYMBOL "story_bgnd"
#define WM_FIX39_ATTRACT_BIO_MUG_TABLE_SYMBOL "wrestler_mugs2"
#define WM_FIX39_ATTRACT_BIO_ATTRIBUTE_TABLE_SYMBOL "wrestler_attributes"
#define WM_FIX39_ATTRACT_BIO_ATTRIBUTE_BARS_SYMBOL "attbars"
#define WM_FIX39_ATTRACT_BIO_ATTRIBUTE_LABEL_SYMBOL "ATT_TXT"
#define WM_FIX39_ATTRACT_BIO_LOGO_Y 24
#define WM_FIX39_ATTRACT_BIO_LOGO_X 11
#define WM_FIX39_ATTRACT_BIO_MUG_Y 0x17a
#define WM_FIX39_ATTRACT_BIO_MUG_X 0
#define WM_FIX39_ATTRACT_BIO_MUG_Z 0x0af
#define WM_FIX39_ATTRACT_BIO_ATTRIBUTE_Y 0xff
#define WM_FIX39_ATTRACT_BIO_ATTRIBUTE_X 0xae
#define WM_FIX39_ATTRACT_BIO_ATTRIBUTE_BAR_Y (0xff + 61)
#define WM_FIX39_ATTRACT_BIO_ATTRIBUTE_BAR_X (0xae + 12)
#define WM_FIX39_ATTRACT_BIO_ATTRIBUTE_BAR_STEP 8
#define WM_FIX39_ATTRACT_BIO_TEXT_CENTER_X 100
#define WM_FIX39_ATTRACT_BIO_TEXT_Y 106
#define WM_FIX39_ATTRACT_BIO_NORMAL_PREWAIT_TSEC 2
#define WM_FIX39_ATTRACT_BIO_NORMAL_WAIT_TSEC 6

/* Designer-hint source layout/timing. */
#define WM_ATTRACT_HINT_BACKGROUND_SYMBOL "hstd_mod"
#define WM_ATTRACT_HINT_BAR_SYMBOL "MVEBAR_R"
#define WM_ATTRACT_HINT_SHADOW_SYMBOL "SHADOW01"
#define WM_ATTRACT_HINT_MUG_BACK_SYMBOL "MUGBAK"
#define WM_ATTRACT_HINT_MUG_FRONT_SYMBOL "MUGFRNT"
#define WM_ATTRACT_HINT_TITLE_Y 62
#define WM_ATTRACT_HINT_TITLE_X 200
#define WM_ATTRACT_HINT_BODY_Y 108
#define WM_ATTRACT_HINT_BODY_X 200
#define WM_ATTRACT_HINT_BODY_LINE_STEP 15
#define WM_ATTRACT_HINT_OPEN_SCREEN_A 18
#define WM_ATTRACT_HINT_OPEN_SCREEN_B 6
#define WM_ATTRACT_HINT_PREWAIT_TICKS 80
#define WM_ATTRACT_HINT_WAIT_TSEC 15

/* General-tips exact render/timing structure. */
#define WM_FIX39_ATTRACT_GENERAL_TIPS_BACKGROUND_SYMBOL "hstd_mod"
#define WM_FIX39_ATTRACT_GENERAL_TIPS_TITLE_LABEL "gen_tip_mes"
#define WM_FIX39_ATTRACT_GENERAL_TIPS_BAR_SYMBOL "MVEBAR_R"
#define WM_FIX39_ATTRACT_GENERAL_TIPS_SHADOW_SYMBOL "SHADOW01"
#define WM_FIX39_ATTRACT_GENERAL_TIPS_TITLE_Y 10
#define WM_FIX39_ATTRACT_GENERAL_TIPS_TITLE_X 200
#define WM_FIX39_ATTRACT_GENERAL_TIPS_FIRST_Y 60
#define WM_FIX39_ATTRACT_GENERAL_TIPS_LINE_STEP 15
#define WM_FIX39_ATTRACT_GENERAL_TIPS_PREWAIT_TSEC 1
#define WM_FIX39_ATTRACT_GENERAL_TIPS_WAIT_TSEC 10

/* Copyright source presentation. */
#define WM_FIX39_ATTRACT_COPYRIGHT_FONT_SYMBOL "RD7FONT"
#define WM_FIX39_ATTRACT_COPYRIGHT_WWF_IMAGE_SYMBOL "SMWWF2"
#define WM_FIX39_ATTRACT_COPYRIGHT_FIRST_Y 110
#define WM_FIX39_ATTRACT_COPYRIGHT_X 200
#define WM_FIX39_ATTRACT_COPYRIGHT_LINE_STEP 12
#define WM_FIX39_ATTRACT_COPYRIGHT_INITIAL_SLEEP_TICKS 2
#define WM_FIX39_ATTRACT_COPYRIGHT_FADE_STEPS 8
#define WM_FIX39_ATTRACT_COPYRIGHT_FADE_SETTLE_TICKS 20
#define WM_FIX39_ATTRACT_COPYRIGHT_PAGE_WAIT_TSEC 3

/* AAMA source presentation. */
#define WM_FIX39_ATTRACT_AAMA_FONT_SYMBOL "RD7FONT"
#define WM_FIX39_ATTRACT_AAMA_GRADIENT_ROWS_TOP 31
#define WM_FIX39_ATTRACT_AAMA_GRADIENT_ROWS_BOTTOM 32
#define WM_FIX39_ATTRACT_AAMA_INITIAL_SLEEP_TICKS 2
#define WM_FIX39_ATTRACT_AAMA_GRADIENT_PREWAIT_TICKS 2
#define WM_FIX39_ATTRACT_AAMA_GRADIENT_POSTWAIT_TICKS 4
#define WM_FIX39_ATTRACT_AAMA_POST_FADE_SLEEP_TICKS 2
#define WM_FIX39_ATTRACT_AAMA_FADE_STEPS 8
#define WM_FIX39_ATTRACT_AAMA_FADE_SETTLE_TICKS 20
#define WM_FIX39_ATTRACT_AAMA_WAIT_TSEC 4

/* Operator-message source presentation / dan_test backdrop. */
#define WM_ATTRACT_OPERATOR_BACKGROUND_SYMBOL "SPORTBKBMOD"
#define WM_ATTRACT_OPERATOR_BALL_SYMBOL "BALLD05A"
#define WM_ATTRACT_OPERATOR_FONT_SYMBOL "osgfont_t"
#define WM_ATTRACT_OPERATOR_FIRST_Y 50
#define WM_ATTRACT_OPERATOR_X 200
#define WM_ATTRACT_OPERATOR_LINE_STEP 45
#define WM_FIX39_ATTRACT_OPERATOR_DAN_SETUP_TICKS (2u + 1u + 32u)
#define WM_ATTRACT_OPERATOR_PREWAIT_TICKS 120
#define WM_ATTRACT_OPERATOR_WAIT_TSEC 6
#define WM_ATTRACT_OPERATOR_BALL_COUNT 32
#define WM_ATTRACT_OPERATOR_BALL_X_LIMIT 400
#define WM_ATTRACT_OPERATOR_BALL_Y_LIMIT 255

#ifdef __cplusplus
}
#endif

#endif
