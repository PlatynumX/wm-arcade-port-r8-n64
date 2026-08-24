/* BEGIN FIX39 V12 ATTRACT RENDERERS */
typedef enum {
    FIX39_FONT_OSGEMD,
    FIX39_FONT_WSF10,
    FIX39_FONT_OGMD10,
    FIX39_FONT_WSF14,
    FIX39_FONT_WGSF18,
    FIX39_FONT_RD7
} fix39_attract_font;

static const wm_source_sprite *fix39_attract_sprite(const char *name) {
    const wm_source_sprite *spr = wm_fix39_attract_sprite_find(name);
    return spr ? spr : wm_select_sprite_find(name);
}

static const wm_source_sprite *fix39_attract_glyph(fix39_attract_font font,
                                                    char c, char name[24]) {
    name[0] = '\0';
    if (font == FIX39_FONT_OSGEMD) {
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
            snprintf(name, 24, "OSGEMD_%c", c);
        else switch (c) {
            case '%': strcpy(name,"OSGEMD_PCT"); break;
            case '&': strcpy(name,"OSGEMD_AND"); break;
            case '!': strcpy(name,"OSGEMD_EXP"); break;
            case '-': strcpy(name,"OSGEMD_DAS"); break;
            case '.': strcpy(name,"OSGEMD_DOT"); break;
            case '$': strcpy(name,"OSGEMD_DOL"); break;
            case '/': strcpy(name,"OSGEMD_SLS"); break;
            case '\'': strcpy(name,"OSGEMD_APO"); break;
            case '?': strcpy(name,"OSGEMD_QUE"); break;
            case ':': strcpy(name,"OSGEMD_COL"); break;
            case '(': strcpy(name,"OSGEMD_OBR"); break;
            case ')': strcpy(name,"OSGEMD_CBR"); break;
            case '#': strcpy(name,"OSGEMD_NUM"); break;
            default: break;
        }
    } else if (font == FIX39_FONT_WSF10) {
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
            snprintf(name, 24, "WSF10_%c", c);
        else switch (c) {
            case '%': strcpy(name,"WSF10PCT"); break;
            case '!': strcpy(name,"WSF10EXC"); break;
            case ',': strcpy(name,"WSF10COM"); break;
            case '.': strcpy(name,"WSF10PER"); break;
            case '-': strcpy(name,"WSF10DSH"); break;
            case '#': strcpy(name,"WSF10NUM"); break;
            case '\'': strcpy(name,"WSF10APO"); break;
            default: break;
        }
    } else if (font == FIX39_FONT_OGMD10) {
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
            snprintf(name, 24, "osgmd10_%c", c);
        else switch (c) {
            case '%': strcpy(name,"osgmd10_PCT"); break;
            case '!': strcpy(name,"osgmd10_EXP"); break;
            case '(': strcpy(name,"osgmd10_OBR"); break;
            case ')': strcpy(name,"osgmd10_CBR"); break;
            case ',': strcpy(name,"osgmd10_COM"); break;
            case '.': strcpy(name,"osgmd10_DOT"); break;
            case ':': strcpy(name,"osgmd10_COL"); break;
            case '-': strcpy(name,"osgmd10_DAS"); break;
            case '#': strcpy(name,"osgmd10_NUM"); break;
            case '&': strcpy(name,"osgmd10_AND"); break;
            case '/': strcpy(name,"osgmd10_SLS"); break;
            case '$': strcpy(name,"osgmd10_DOL"); break;
            case '\'': strcpy(name,"osgmd10_APO"); break;
            default: break;
        }
    } else if (font == FIX39_FONT_WSF14) {
        if (c >= '0' && c <= '9')
            snprintf(name, 24, "WSF14_%c", c);
        else switch (c) {
            case '#': strcpy(name,"WSF14NUM"); break;
            default: break;
        }
    } else if (font == FIX39_FONT_WGSF18) {
        if (c >= '0' && c <= '9')
            snprintf(name, 24, "WGSF18_%c", c);
        else switch (c) {
            case '.': strcpy(name,"WGSF18PER"); break;
            default: break;
        }
    } else {
        if (c >= 'A' && c <= 'Z') snprintf(name, 24, "FONT7%c", c);
        else if (c >= '0' && c <= '9') snprintf(name, 24, "FONT7%c", c);
        else switch (c) {
            case '!': strcpy(name,"FONT7excla"); break;
            case '"': strcpy(name,"FONT7quote"); break;
            case '#': strcpy(name,"FONT7pound"); break;
            case '$': strcpy(name,"FONT7doll"); break;
            case '%': strcpy(name,"FONT7percen"); break;
            case '&': strcpy(name,"FONT7and"); break;
            case '\'': strcpy(name,"FONT7apost"); break;
            case '(': strcpy(name,"FONT7parenl"); break;
            case ')': strcpy(name,"FONT7parenr"); break;
            case '*': strcpy(name,"FONT7ast"); break;
            case '+': strcpy(name,"FONT7plus"); break;
            case ',': strcpy(name,"FONT7comma"); break;
            case '-': strcpy(name,"FONT7dash"); break;
            case '.': strcpy(name,"FONT7period"); break;
            case '/': strcpy(name,"FONT7forsp"); break;
            case ':': strcpy(name,"FONT7colon"); break;
            case ';': strcpy(name,"FONT7semicol"); break;
            case '<': strcpy(name,"FONT7less"); break;
            case '=': strcpy(name,"FONT7equal"); break;
            case '>': strcpy(name,"FONT7more"); break;
            case '?': strcpy(name,"FONT7quest"); break;
            case '[': strcpy(name,"FONT7bracl"); break;
            case ']': strcpy(name,"FONT7bracr"); break;
            default: break;
        }
    }
    return name[0] ? fix39_attract_sprite(name) : NULL;
}

static void fix39_draw_sprite(const wm_source_sprite *spr, int x, int y,
                              const wm_fix39_attract_palette *pal) {
    if (!spr) return;
    wm_source_sprite proxy = *spr;
    const wm_source_sprite *palette_source = spr;
    if (pal && pal->rgba5551 && pal->color_count) {
        proxy.palette_rgba5551 = pal->rgba5551;
        proxy.palette_colors = pal->color_count;
        palette_source = &proxy;
    }
    draw_source_sprite_scaled(spr,
                              (float)x * WM_FRONTEND_SCALE_X,
                              (float)y * WM_FRONTEND_SCALE_Y,
                              spr->xani, spr->yani, false, palette_source,
                              WM_FRONTEND_SCALE_X, WM_FRONTEND_SCALE_Y);
}

static int fix39_text_width(const char *text, fix39_attract_font font,
                            int space_width, int spacing) {
    int width = 0;
    bool have_glyph = false;
    if (!text) return 0;
    for (const char *p = text; *p; ++p) {
        if (*p == ' ') { width += space_width; continue; }
        char name[24];
        const wm_source_sprite *g = fix39_attract_glyph(font, *p, name);
        if (!g) continue;
        width += (int)g->width + spacing;
        have_glyph = true;
    }
    if (have_glyph && spacing > 0) width -= spacing;
    return width;
}

static void fix39_draw_text(const char *text, fix39_attract_font font,
                            int x, int y, bool centered,
                            int space_width, int spacing,
                            const wm_fix39_attract_palette *pal,
                            const wm_source_sprite *sprite_pal) {
    if (!text) return;
    if (centered) x -= fix39_text_width(text, font, space_width, spacing) / 2;
    for (const char *p = text; *p; ++p) {
        if (*p == ' ') { x += space_width; continue; }
        char name[24];
        const wm_source_sprite *g = fix39_attract_glyph(font, *p, name);
        if (!g) continue;
        if (sprite_pal && sprite_pal->palette_rgba5551) {
            wm_source_sprite proxy = *g;
            proxy.palette_rgba5551 = sprite_pal->palette_rgba5551;
            proxy.palette_colors = sprite_pal->palette_colors;
            draw_source_sprite_scaled(g,
                                      (float)x * WM_FRONTEND_SCALE_X,
                                      (float)y * WM_FRONTEND_SCALE_Y,
                                      g->xani, g->yani, false, &proxy,
                                      WM_FRONTEND_SCALE_X, WM_FRONTEND_SCALE_Y);
        } else {
            fix39_draw_sprite(g, x, y, pal);
        }
        x += (int)g->width + spacing;
    }
}

static void fix39_render_slate(void) {
    fill_rect(0, 0, 320, 240, RGBA32(0,0,0,255));
    render_select_module("slateBMOD", true, 0, 0);
}

/* HSTD.ASM table renderer.  Coordinates below are direct translations of
   print_inter/print_beaten/print_tag/print_hscores/print_winstreaks and their
   draw_*_table_entry helpers.  Persistence is intentionally separate: V12i
   presents the source factory/runtime tables already owned by Fix39. */
/* Keep lookup keys inline rather than as a ROM pointer table.  This mirrors
   the V12f hint hardening and avoids adding another dynamic pointer chain to
   an ATTRACT path that repeats indefinitely on hardware. */
static const char fix39_hs_crouton[8][8] = {
    "CRUT_BH", "CRUT_RR", "CRUT_UN", "CRUT_YK",
    "CRUT_SM", "CRUT_BM", "CRUT_DK", "CRUT_LX"
};

static const char fix39_hs_wrestler_name[9][8] = {
    "HART", "RAZOR", "UNDER", "YOKO", "SHAWN",
    "BAMBAM", "DOINK", "", "LEX"
};

static void fix39_draw_text_right(const char *text, fix39_attract_font font,
                                  int x, int y, int space_width, int spacing,
                                  const wm_fix39_attract_palette *pal) {
    x -= fix39_text_width(text, font, space_width, spacing);
    fix39_draw_text(text, font, x, y, false, space_width, spacing, pal, NULL);
}

static void fix39_hs_initials3(const uint8_t in[WM_HS_NUM_INITIALS + 1u], char out[4]) {
    out[0] = (char)in[0];
    out[1] = (char)in[1];
    out[2] = (char)in[2];
    out[3] = '\0';
}

static void fix39_hs_trim5(const uint8_t in[WM_HS_NUM_INITIALS + 1u],
                           char out[WM_HS_NUM_INITIALS + 1u]) {
    unsigned first = 0u;
    unsigned last = WM_HS_NUM_INITIALS;
    while (first < WM_HS_NUM_INITIALS && in[first] == (uint8_t)' ') ++first;
    while (last > first && in[last - 1u] == (uint8_t)' ') --last;
    unsigned n = 0u;
    for (unsigned i = first; i < last; ++i) out[n++] = (char)in[i];
    out[n] = '\0';
}

static const wm_fix39_attract_palette *fix39_hs_row_palette(const WmHsDisplayRow *row) {
    return wm_fix39_attract_palette_find(row && row->highlighted ? "BLUE" : "GOLD");
}

static void fix39_hs_rank(uint16_t rank, int x, int y, bool with_hash,
                          bool streak_decor) {
    char buf[8];
    const wm_fix39_attract_palette *rank_pal = wm_fix39_attract_palette_find("WSF_W_P");
    const wm_fix39_attract_palette *bar_pal = wm_fix39_attract_palette_find("DPLT_R_P");
    snprintf(buf, sizeof(buf), with_hash ? "#%u" : "%u", (unsigned)rank);
    fix39_draw_sprite(fix39_attract_sprite("SPEAR"), x - 8, y + 2, NULL);
    if (streak_decor)
        fix39_draw_sprite(fix39_attract_sprite("BARBUTT"), x - 10, y + 2, bar_pal);
    else
        fix39_draw_sprite(fix39_attract_sprite("SPEAR"), x + 13, y + 2, NULL);
    fix39_draw_text(buf, FIX39_FONT_WSF14, streak_decor ? x - 4 : x, y,
                    false, 7, 0, rank_pal, NULL);
}

static void fix39_hs_backdrop(void) {
    fix39_render_slate();
}

/* HSTD.ASM gives MVEBAR_R/SHADOW01 OZPOS 0x1799 while scrolling rows use
   0x1000..0x1003. MOVE_ALL_OBJS_UP explicitly leaves objects above 0x1798
   stationary, so the fixed header is also in front of every scrolling rank,
   initial and defeated-wrestler object. Draw this overlay last to reproduce
   the arcade object's Z ordering instead of letting N64 submission order put
   scrolling rows through the banner. */
static void fix39_hs_header_overlay(const WmHsPresentDescriptor *desc) {
    const wm_fix39_attract_palette *blue = wm_fix39_attract_palette_find("BLUE");
    fix39_draw_sprite(fix39_attract_sprite(WM_HS_PRESENT_BAR_SOURCE_SYMBOL),
                      WM_HS_PRESENT_BAR_X, WM_HS_PRESENT_BAR_Y, NULL);
    fix39_draw_sprite(fix39_attract_sprite(WM_HS_PRESENT_SHADOW_SOURCE_SYMBOL),
                      WM_HS_PRESENT_SHADOW_X, WM_HS_PRESENT_SHADOW_Y, NULL);
    fix39_draw_text(desc->title, FIX39_FONT_OSGEMD,
                    desc->layout.title_x, desc->layout.title_y, true,
                    desc->layout.title_space_width, 0, blue, NULL);
}

static void fix39_hs_draw_beaten(const WmHsSystem *sys,
                                 const WmAttractLive *live,
                                 WmHsPresentScreen screen) {
    const WmHsPresentDescriptor *desc = &wm_hs_present_sequence[screen];
    WmHsDisplayRow rows[4];
    size_t count = wm_hs_present_rows(sys, screen, live->hiscore_start_rank, rows, 3u);
    const int scroll = live->phase == WM_ATTRACT_LIVE_HSTD_SCROLL ?
                       (int)live->hiscore_scroll_px : 0;
    if (scroll && live->hiscore_start_rank + 3u <= desc->total_rows) {
        WmHsDisplayRow next;
        if (wm_hs_present_rows(sys, screen,
                               (uint16_t)(live->hiscore_start_rank + 3u),
                               &next, 1u) == 1u) {
            rows[count++] = next;
        }
    }

    for (size_t i = 0; i < count; ++i) {
        const WmHsDisplayRow *row = &rows[i];
        const int iy = desc->layout.first_initials_y + (int)i * desc->layout.row_y_step - scroll;
        const int sy = desc->layout.first_score_y + (int)i * desc->layout.row_y_step - scroll;
        const wm_fix39_attract_palette *row_pal = fix39_hs_row_palette(row);
        int icon_x = desc->layout.first_score_x;
        unsigned icons = 0u;
        char initials[WM_HS_NUM_INITIALS + 1u];
        memcpy(initials, row->initials, sizeof(initials));

        if (iy < -40 || iy > 260) continue;
        for (unsigned n = 0; n < 8u; ++n) {
            if (row->defeated_wrestler_bits & (1u << n)) {
                fix39_draw_sprite(fix39_attract_sprite(fix39_hs_crouton[n]),
                                  icon_x, sy + 2, NULL);
                icon_x += 0x30;
                ++icons;
            }
        }
        if (icons < 8u) {
            icon_x += 10;
            for (unsigned n = icons; n < 8u; ++n) {
                fix39_draw_sprite(fix39_attract_sprite("OSGEMD_DOT"), icon_x, sy + 2, NULL);
                icon_x += 0x30;
            }
        }
        /* draw_beaten_table_entry: a9.x + 50, uncentred, OSGEMD. */
        fix39_draw_text(initials, FIX39_FONT_OSGEMD,
                        desc->layout.first_initials_x + 50, iy, false,
                        10, 0, row_pal, NULL);
        fix39_hs_rank(row->rank, desc->layout.first_initials_x, iy, true, false);
    }
}

static void fix39_hs_draw_tag(const WmHsSystem *sys) {
    const WmHsPresentDescriptor *desc = &wm_hs_present_sequence[WM_HS_PRESENT_TAG];
    WmHsDisplayRow rows[9];
    const wm_fix39_attract_palette *score_pal = wm_fix39_attract_palette_find("WGSF_W_P1");
    size_t count = wm_hs_present_rows(sys, WM_HS_PRESENT_TAG, 1u, rows, 9u);
    for (size_t i = 0; i < count; ++i) {
        const WmHsDisplayRow *row = &rows[i];
        const int y = desc->layout.first_score_y + (int)i * desc->layout.row_y_step;
        const int ry = desc->layout.first_initials_y + (int)i * desc->layout.row_y_step;
        const wm_fix39_attract_palette *row_pal = fix39_hs_row_palette(row);
        char score[24];
        char initials[WM_HS_NUM_INITIALS + 1u];
        char partner[WM_HS_NUM_INITIALS + 1u];
        fix39_hs_trim5(row->initials, initials);
        fix39_hs_trim5(row->partner_initials, partner);
        if (!wm_hs_format_time(row->score_bcd, score, sizeof(score))) strcpy(score, "0.0");
        /* draw_tag_table_entry source A6=0x110, a10.x=72. */
        fix39_draw_text_right(score, FIX39_FONT_WGSF18,
                              desc->layout.first_score_x + 0x110, y,
                              10, 2, score_pal);
        fix39_draw_text(initials, FIX39_FONT_OSGEMD,
                        desc->layout.first_score_x + 30, y, true,
                        10, 0, row_pal, NULL);
        fix39_draw_sprite(fix39_attract_sprite("OSGEMD_AND"),
                          desc->layout.first_score_x + 0x52, y, row_pal);
        fix39_draw_text(partner, FIX39_FONT_OSGEMD,
                        desc->layout.first_score_x + 155, y, true,
                        10, 0, row_pal, NULL);
        fix39_hs_rank(row->rank, desc->layout.first_initials_x, ry, true, false);
    }
}

static void fix39_hs_draw_pin(const WmHsSystem *sys) {
    const WmHsPresentDescriptor *desc = &wm_hs_present_sequence[WM_HS_PRESENT_PIN];
    WmHsDisplayRow rows[9];
    const wm_fix39_attract_palette *score_pal = wm_fix39_attract_palette_find("WGSF_W_P1");
    size_t count = wm_hs_present_rows(sys, WM_HS_PRESENT_PIN, 1u, rows, 9u);
    for (size_t i = 0; i < count; ++i) {
        const WmHsDisplayRow *row = &rows[i];
        const int y = desc->layout.first_score_y + (int)i * desc->layout.row_y_step;
        const int ry = desc->layout.first_initials_y + (int)i * desc->layout.row_y_step;
        const wm_fix39_attract_palette *row_pal = fix39_hs_row_palette(row);
        char score[24];
        char initials[4];
        fix39_hs_initials3(row->initials, initials);
        if (!wm_hs_format_time(row->score_bcd, score, sizeof(score))) strcpy(score, "0.0");
        /* draw_pinspeed_table_entry source A6=120, a10.x=72. */
        fix39_draw_text_right(score, FIX39_FONT_WGSF18,
                              desc->layout.first_score_x + 120, y,
                              10, 2, score_pal);
        fix39_draw_text(initials, FIX39_FONT_OSGEMD,
                        desc->layout.first_score_x + 0x17, y, true,
                        10, 0, row_pal, NULL);
        if (row->wrestler_index >= 0 && row->wrestler_index < 9 &&
            fix39_hs_wrestler_name[(unsigned)row->wrestler_index][0] != '\0') {
            fix39_draw_sprite(
                fix39_attract_sprite(fix39_hs_wrestler_name[(unsigned)row->wrestler_index]),
                /* HSTD.ASM adds [302,0] to A9 ([Y,Xleft]), not A10. */
                desc->layout.first_initials_x + 302, y + 7, NULL);
        }
        fix39_hs_rank(row->rank, desc->layout.first_initials_x, ry, true, false);
    }
}

static void fix39_hs_draw_streak(const WmHsSystem *sys) {
    const WmHsPresentDescriptor *desc = &wm_hs_present_sequence[WM_HS_PRESENT_STREAK];
    WmHsDisplayRow rows[18];
    const wm_fix39_attract_palette *score_pal = wm_fix39_attract_palette_find("WGSF_W_P1");
    size_t count = wm_hs_present_rows(sys, WM_HS_PRESENT_STREAK, 1u, rows, 18u);
    for (size_t i = 0; i < count; ++i) {
        const bool second = i >= 9u;
        const int slot = (int)(second ? i - 9u : i);
        const int ix = second ? desc->layout.second_initials_x : desc->layout.first_initials_x;
        const int iy0 = second ? desc->layout.second_initials_y : desc->layout.first_initials_y;
        const int sx = second ? desc->layout.second_score_x : desc->layout.first_score_x;
        const int sy0 = second ? desc->layout.second_score_y : desc->layout.first_score_y;
        const int y = sy0 + slot * desc->layout.row_y_step;
        const int ry = iy0 + slot * desc->layout.row_y_step;
        const WmHsDisplayRow *row = &rows[i];
        const wm_fix39_attract_palette *row_pal = fix39_hs_row_palette(row);
        char score[24];
        char initials[4];
        fix39_hs_initials3(row->initials, initials);
        snprintf(score, sizeof(score), "%u", (unsigned)row->score_binary);
        /* draw_winstreak_table_entry source A6=80, print_string_C. */
        fix39_draw_text(score, FIX39_FONT_WGSF18,
                        sx + 80, y, true, 10, 0, score_pal, NULL);
        fix39_draw_text(initials, FIX39_FONT_OSGEMD,
                        sx + 0x17, y, true, 10, 0, row_pal, NULL);
        fix39_hs_rank(row->rank, ix, ry, false, true);
    }
}

static void fix39_render_hiscores(void) {
    const WmAttractLive *live = wm_fix39_attract_live_state();
    const WmHsSystem *sys = wm_fix39_hiscore_system();
    if (!live || !sys || live->page >= (uint8_t)WM_HS_PRESENT_COUNT) return;
    WmHsPresentScreen screen = (WmHsPresentScreen)live->page;
    const WmHsPresentDescriptor *desc = &wm_hs_present_sequence[screen];
    fix39_hs_backdrop();
    switch (screen) {
        case WM_HS_PRESENT_INTER:
        case WM_HS_PRESENT_BEATEN:
            fix39_hs_draw_beaten(sys, live, screen);
            break;
        case WM_HS_PRESENT_TAG:
            fix39_hs_draw_tag(sys);
            break;
        case WM_HS_PRESENT_PIN:
            fix39_hs_draw_pin(sys);
            break;
        case WM_HS_PRESENT_STREAK:
            fix39_hs_draw_streak(sys);
            break;
        default:
            break;
    }
    fix39_hs_header_overlay(desc);
}

/* N64 runtime hardening for the source WHICH_HINT/WHICH_22_NUM tables.
   V12e hardware pinned the second-loop exception to the dynamic hint-art
   wm_fix39_attract_sprite_find() call.  The exception alone cannot prove
   whether the bad read was the WmAttractHint name pointer or the generated
   lookup table's source_frame pointer, so V12f removes the former from this
   N64 path while the asset generator removes the latter from lookup scans.
   Keep the exact source order with inline character matrices. */
static const char fix39_hint_tip_symbols[WM_ATTRACT_ACTIVE_HINTS][8] = {
    "JMSTIP", "MIKTIP", "MJTTIP", "JOSTIP", "EUGTIP",
    "SHNTIP", "JAKTIP", "SALTIP", "TONTIP", "EUGTIP"
};
static const char fix39_hint_mug_symbols[WM_ATTRACT_ACTIVE_HINTS][8] = {
    "JASMUG", "MIKMUG", "MRKMUG", "JSHMUG", "EUGMUG",
    "SHNMUG", "JAKMUG", "SALMUG", "TONMUG", "JSHMUG"
};
static const char fix39_hint_number_symbols[WM_ATTRACT_ACTIVE_HINTS][10] = {
    "WGSF22_1", "WGSF22_2", "WGSF22_3", "WGSF22_4", "WGSF22_5",
    "WGSF22_6", "WGSF22_7", "WGSF22_8", "WGSF22_9", "WGSF22_0"
};

static void fix39_render_hint(void) {
    const WmAttractLive *live = wm_fix39_attract_live_state();
    if (!live) return;
    unsigned h = live->hint_index % WM_ATTRACT_ACTIVE_HINTS;
    const wm_fix39_attract_palette *ruby = wm_fix39_attract_palette_find("RUBYPAL");
    const wm_fix39_attract_palette *yellow = wm_fix39_attract_palette_find("WSF_Y_P");
    const wm_fix39_attract_palette *white = wm_fix39_attract_palette_find("WGFS_W_P");
    fix39_render_slate();
    fix39_draw_sprite(fix39_attract_sprite("MVEBAR_R"), 10, 21, NULL);
    fix39_draw_sprite(fix39_attract_sprite("SHADOW01"), 13, 30, NULL);
    const wm_source_sprite *tip = fix39_attract_sprite(fix39_hint_tip_symbols[h]);
    const wm_source_sprite *num = fix39_attract_sprite(fix39_hint_number_symbols[h]);
    fix39_draw_sprite(tip, 190, 21, white);
    /* ATTRACT.ASM PUT_UP_TIP_NAME uses OSIZEX of the designer-name/TIP#
       object, not the number glyph, then adds 200 for the number X. */
    if (tip && num) fix39_draw_sprite(num, 200 + (int)tip->width / 2, 9, white);
    fix39_draw_sprite(fix39_attract_sprite("MUGBAK"), 400, 254, NULL);
    fix39_draw_sprite(fix39_attract_sprite("MUGFRNT"), 400, 254, NULL);
    fix39_draw_sprite(fix39_attract_sprite(fix39_hint_mug_symbols[h]), 400, 254, NULL);
    fix39_draw_text(wm_fix39_source_hint_title(h), FIX39_FONT_OSGEMD,
                    200, 62, true, 10, 0, ruby, NULL);
    size_t lines = wm_fix39_source_hint_line_count(h);
    for (size_t i = 0; i < lines; ++i)
        fix39_draw_text(wm_fix39_source_hint_line(h, i), FIX39_FONT_WSF10,
                        200, 108 + (int)i * 15, true, 10, 1, yellow, NULL);
    if (live->waiting_external)
        (void)wm_fix39_attract_screen_signal_external_complete();
}

static void fix39_render_general_tips(void) {
    const WmAttractLive *live = wm_fix39_attract_live_state();
    const wm_fix39_attract_palette *yellow = wm_fix39_attract_palette_find("SGMD8YEL");
    const wm_source_sprite *blue = wm_select_sprite_find("OSGEMD_C");
    fix39_render_slate();
    fix39_draw_sprite(fix39_attract_sprite("MVEBAR_R"), 10, 21, NULL);
    fix39_draw_sprite(fix39_attract_sprite("SHADOW01"), 13, 30, NULL);
    fix39_draw_text(wm_fix39_source_general_title(), FIX39_FONT_OSGEMD,
                    200, 10, true, 10, 0, NULL, blue);
    for (size_t i = 0; i < WM_FIX39_ATTRACT_GENERAL_TIP_ROWS; ++i)
        fix39_draw_text(wm_fix39_source_general_row(i), FIX39_FONT_OGMD10,
                        200, 60 + (int)i * 15, true, 6, 0, yellow, NULL);
    if (live && live->waiting_external)
        (void)wm_fix39_attract_screen_signal_external_complete();
}

static void fix39_render_copyright(void) {
    const WmAttractLive *live = wm_fix39_attract_live_state();
    fill_rect(0, 0, 320, 240, RGBA32(0,0,0,255));
    if (!live) return;
    const bool page2 = live->page >= 2u;
    size_t first = page2 ? 9u : 0u;
    size_t count = page2 ? 10u : 9u;
    if (!page2)
        fix39_draw_sprite(fix39_attract_sprite("SMWWF2"), 200, 65, NULL);
    /* STRCNRMO_2 is an external Wolf-unit library routine. Published game
       source gives a10=1 and all glyph widths/anchors but not its private
       blank-space metric; keep source glyphs/native RD7 palette instead of an
       N64/debug-font replacement. */
    for (size_t i = 0; i < count; ++i)
        fix39_draw_text(wm_fix39_source_copyright_line(first + i), FIX39_FONT_RD7,
                        200, 110 + (int)i * 12, true, 1, 1, NULL, NULL);
}

static color_t fix39_aama_blue(unsigned rgb555) {
    /* ATTRACT.ASM::blue_grad_pal is literal RGB555: 31..0 (blue channel). */
    unsigned b5 = rgb555 & 31u;
    unsigned b8 = (b5 << 3) | (b5 >> 2);
    return RGBA32(0, 0, b8, 255);
}

static void fix39_render_aama_gradient(void) {
    /* Direct translation of do_the_grad_thang's two 2-pixel-high DMA runs.
       Top: palette indexes 0..30.  Then source advances Y by 0x84 (132).
       Bottom: palette indexes 31..0.  The final source rows clip at 256. */
    int sy = 0;
    for (unsigned index = 0; index < 31u; ++index, sy += 2) {
        unsigned rgb555 = 31u - index;
        int y0 = (sy * 240) / 256;
        int y1 = ((sy + 2) * 240 + 255) / 256;
        fill_rect(0, y0, 320, y1, fix39_aama_blue(rgb555));
    }
    sy += 132;
    for (int index = 31; index >= 0; --index, sy += 2) {
        if (sy >= 256) break;
        unsigned rgb555 = 31u - (unsigned)index;
        int end = sy + 2;
        if (end > 256) end = 256;
        int y0 = (sy * 240) / 256;
        int y1 = (end * 240 + 255) / 256;
        fill_rect(0, y0, 320, y1, fix39_aama_blue(rgb555));
    }
}

static void fix39_render_aama(void) {
    fill_rect(0, 0, 320, 240, RGBA32(0,0,0,255));
    fix39_render_aama_gradient();
    fix39_draw_text(wm_fix39_source_aama_line(0), FIX39_FONT_RD7, 200,94,true,1,1,NULL,NULL);
    fix39_draw_text(wm_fix39_source_aama_line(1), FIX39_FONT_RD7, 177,114,false,1,1,NULL,NULL);
    fix39_draw_text(wm_fix39_source_aama_line(2), FIX39_FONT_RD7, 260,114,false,1,1,NULL,NULL);
    fix39_draw_text(wm_fix39_source_aama_line(3), FIX39_FONT_RD7, 200,125,true,1,1,NULL,NULL);
    fix39_draw_text(wm_fix39_source_aama_line(4), FIX39_FONT_RD7, 200,136,true,1,1,NULL,NULL);
    fix39_draw_text(wm_fix39_source_aama_line(5), FIX39_FONT_RD7, 200,147,true,1,1,NULL,NULL);
}
/* END FIX39 V12 ATTRACT RENDERERS */
