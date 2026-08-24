#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import re
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("apply_fix39_v12", ROOT / "tools" / "apply_fix39.py")
mod = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(mod)


def test_makefile() -> None:
    with tempfile.TemporaryDirectory() as td:
        repo = Path(td)
        (repo / "src/core/arcade").mkdir(parents=True)
        make = repo / "Makefile"
        make.write_text(
            "CFLAGS += -I$(CURDIR)/include\n"
            "CORE_C := src/core/app.c\n"
            "ASSET_C := src/generated/base.c\n"
            "N64_C := src/platform/n64/main.c\n"
            "C_FILES := $(CORE_C) $(ASSET_C) $(N64_C)\n"
            "$(BUILD_DIR)/$(ROMNAME).elf: $(OBJS)\n"
            "\t$(N64_LD) -o $@ $(OBJS)\n"
        )
        mod.patch_makefile(make, ["wm_fix39_runtime.c", "wmania_attract_live.c"])
        once = make.read_text()
        assert "src/generated/fix39_attract_text_generated.c" in once
        assert "src/generated/fix39_attract_assets.c" in once
        assert "# BEGIN FIX39 ATTRACT SOURCE ASSETS" in once
        assert "# BEGIN FIX39 ATTRACT GENERATED HEADER ORDER" in once
        assert "$(BUILD_DIR)/src/platform/n64/main.o: $(FIX39_ATTRACT_TEXT_H) $(FIX39_ATTRACT_ASSET_H)" in once
        mod.patch_makefile(make, ["wm_fix39_runtime.c", "wmania_attract_live.c"])
        assert make.read_text() == once


def test_frontend_script() -> None:
    with tempfile.TemporaryDirectory() as td:
        p = Path(td) / "prepare_frontend_assets.sh"
        p.write_text(
            "python3 tools/bmod_source.py --module NTITLESCBMOD \\\n"
            "    --module SPORTBKBMOD \\\n"
            "    --module LADDERBMOD \\\n"
            "    --module choiceBMOD \\\n"
        )
        mod.patch_frontend_assets_script(p)
        once = p.read_text()
        assert "--module LADDERBMOD \\\n    --module slateBMOD \\\n" in once
        mod.patch_frontend_assets_script(p)
        assert p.read_text() == once


def test_n64_main() -> None:
    with tempfile.TemporaryDirectory() as td:
        p = Path(td) / "main.c"
        p.write_text(
            '#include "wm/app.h"\n'
            "\n"
            "static const char *progress_factory_recent_champ(const wm_app *app) {\n"
            "    (void)app;\n"
            '    return "OLD";\n'
            "}\n"
            "\n"
            "static void render_character_select(const wm_app *app) { (void)app; }\n"
            "\n"
            "static void render_frame(const wm_app *app) {\n"
            "    switch (app->attract.call) {\n"
            "        case WM_ATTRACT_SHOW_TITLE:\n"
            "            render_title_screen(app);\n"
            "            break;\n"
            "        default: break;\n"
            "    }\n"
            "}\n"
            "\n"
            "int main(void) {\n"
            "    wm_app app;\n"
            "    wm_app_init(&app);\n"
            "    return 0;\n"
            "}\n"
        )
        mod.patch_n64_main(p)
        once = p.read_text()
        for inc in (
            'wm_fix39_runtime.h', 'wmania_attract_data.h', 'wmania_hiscore_present.h',
            'fix39_attract_text_generated.h', 'fix39_attract_assets_generated.h',
        ):
            assert inc in once
        assert "/* BEGIN FIX39 V12 ATTRACT RENDERERS */" in once
        assert "case WM_ATTRACT_SHOW_HSTD:" in once
        assert "fix39_render_hiscores();" in once
        assert "case WM_ATTRACT_DO_HINTS:" in once
        assert "case WM_ATTRACT_SHOW_GEN_TIPS:" in once
        assert "case WM_ATTRACT_SHOW_COPYRIGHT:" in once
        assert "case WM_ATTRACT_AAMA_MESSAGE:" in once
        assert "WM_FIX39_ATTRACT_CAP_HISCORES" in once
        assert "WM_FIX39_ATTRACT_CAP_DESIGNER_HINT" in once
        assert "live->page >= 2u" in once
        mod.patch_n64_main(p)
        assert p.read_text() == once


def test_renderer_contract() -> None:
    s = (ROOT / "patches/fix39_v12_n64_attract.inc").read_text()
    assert 'fix39_attract_sprite("SMWWF2")' in s
    assert "const bool page2 = live->page >= 2u;" in s
    assert "if (!page2)" in s
    assert "wm_fix39_attract_screen_signal_external_complete" in s
    assert "case '\\'': strcpy(name,\"OSGEMD_APO\")" in s
    assert "FONT7period" in s
    assert "fix39_render_aama_gradient" in s
    assert "static color_t fix39_aama_blue(unsigned rgb555)" in s
    assert "static uint32_t fix39_aama_blue" not in s
    assert "31u - index" in s
    # V12f hardware regression: N64 must not dereference WmAttractHint's
    # pointer-bearing art names.  Keep exact source names in inline char grids.
    assert "fix39_hint_tip_symbols[WM_ATTRACT_ACTIVE_HINTS][8]" in s
    assert "fix39_hint_mug_symbols[WM_ATTRACT_ACTIVE_HINTS][8]" in s
    assert "fix39_hint_number_symbols[WM_ATTRACT_ACTIVE_HINTS][10]" in s
    assert "hint->tip_image_symbol" not in s
    assert "hint->number_image_symbol" not in s
    assert "hint->mug_image_symbol" not in s
    assert '"JMSTIP", "MIKTIP", "MJTTIP", "JOSTIP", "EUGTIP"' in s
    assert '"WGSF22_6", "WGSF22_7", "WGSF22_8", "WGSF22_9", "WGSF22_0"' in s

    # V12g: ATTRACT.ASM PUT_UP_TIP_NAME anchors the number to the width of
    # the designer-name/TIP# art, never to the number glyph itself.
    assert "200 + (int)tip->width / 2" in s
    assert "200 + (int)num->width / 2" not in s

    # V12g+: all five HSTD pages are rendered from the existing source-backed
    # high-score system using HSTD.ASM's exact table order and font families.
    assert "static void fix39_render_hiscores(void)" in s
    assert "WM_HS_PRESENT_INTER" in s and "WM_HS_PRESENT_BEATEN" in s
    assert "WM_HS_PRESENT_TAG" in s and "WM_HS_PRESENT_PIN" in s
    assert "WM_HS_PRESENT_STREAK" in s
    assert "FIX39_FONT_WSF14" in s
    assert "FIX39_FONT_WGSF18" in s
    assert '"CRUT_BH", "CRUT_RR", "CRUT_UN", "CRUT_YK"' in s
    assert '"HART", "RAZOR", "UNDER", "YOKO", "SHAWN"' in s
    assert "static const char fix39_hs_crouton[8][8]" in s
    assert "static const char fix39_hs_wrestler_name[9][8]" in s
    assert "static const char *fix39_hs_crouton" not in s
    assert "static const char *fix39_hs_wrestler_name" not in s
    assert "wm_fix39_hiscore_system()" in s
    assert "fix39_hs_initials3" in s
    assert "fix39_hs_trim5" in s

    # V12h hardware regression: HSTD.ASM places the pindown wrestler-name
    # object at A9.x + 302 (left/rank origin), not A10.x + 302 (score origin).
    assert "desc->layout.first_initials_x + 302" in s
    assert "desc->layout.first_score_x + 302" not in s

    # V12i hardware regression: HSTD.ASM assigns the fixed MVEBAR_R/SHADOW01
    # objects Z=0x1799 while scrolling row objects live at 0x1000..0x1003.
    # The N64 immediate renderer must therefore submit the header after rows.
    assert "static void fix39_hs_backdrop(void)" in s
    assert "static void fix39_hs_header_overlay" in s
    render = s[s.index("static void fix39_render_hiscores(void)"):]
    assert render.index("fix39_hs_backdrop();") < render.index("switch (screen)")
    assert render.index("switch (screen)") < render.index("fix39_hs_header_overlay(desc);")
    assert "fix39_hs_backdrop_and_title" not in s
    assert "0x1799" in s and "0x1000..0x1003" in s

    # HSTD.ASM shared object/layout constants: keep the source [Y,X]
    # interpretation from drifting back to the earlier transposed values.
    present = (ROOT / "src/fix39/wmania_hiscore_present.h").read_text()
    assert "uint8_t title_space_width;" in present
    assert "#define WM_HS_PRESENT_BAR_X 10" in present
    assert "#define WM_HS_PRESENT_BAR_Y 21" in present
    assert "#define WM_HS_PRESENT_SHADOW_X 13" in present
    assert "#define WM_HS_PRESENT_SHADOW_Y 30" in present
    assert "#define WM_HS_PRESENT_SCROLL_STEP_TICKS_A 36u" in present
    assert "#define WM_HS_PRESENT_SCROLL_STEP_TICKS_B 34u" in present
    assert "#define WM_HS_PRESENT_SCROLL_LAST_OFF_TICKS 0x15u" in present
    assert "#define WM_HS_PRESENT_SCROLL_GROUP_HOLD_TICKS 85u" in present

    # Harden the other side of the failing comparison too: generated sprite
    # lookup keys are inline character arrays, never sprites[i].source_frame
    # pointer dereferences while scanning the large generated asset table.
    gen = (ROOT / "tools/fix39_attract_assets.py").read_text()
    gen_emit = gen.split("def self_test", 1)[0]
    assert "wm_fix39_attract_sprite_lookup" in gen_emit
    assert "sprite_lookup[i].source_frame" in gen_emit
    assert "sprites[i].source_frame, source_frame" not in gen_emit

    # V12h hardware regression: external IMGPAL palettes used with DMAWNZ
    # must make CI8 index 0 transparent even when its RGB555 value is nonzero.
    assert "def rgba5551(v: int, palette_index: int)" in gen
    assert "if palette_index == 0:" in gen
    assert "rgba5551(v, i) for i, v in enumerate(named_pals[name])" in gen
    assert "transparent_zero" not in gen

    # The inline N64 arrays must remain byte-for-byte aligned with the source
    # WHICH_HINT / WHICH_22_NUM translation carried by the portable core.
    data = (ROOT / "src/fix39/wmania_attract_data.c").read_text()
    triples = re.findall(
        r'\},\s*"([A-Z0-9_]+)",\s*"([A-Z0-9_]+)",\s*"([A-Z0-9_]+)",\s*\d+u,\s*\d+u\s*\}',
        data,
    )
    assert triples[:10] == [
        ("JMSTIP", "JASMUG", "WGSF22_1"),
        ("MIKTIP", "MIKMUG", "WGSF22_2"),
        ("MJTTIP", "MRKMUG", "WGSF22_3"),
        ("EUGTIP", "EUGMUG", "WGSF22_4"),
        ("SHNTIP", "SHNMUG", "WGSF22_5"),
        ("JAKTIP", "JAKMUG", "WGSF22_6"),
        ("SALTIP", "SALMUG", "WGSF22_7"),
        ("TONTIP", "TONMUG", "WGSF22_8"),
        ("JOSTIP", "JSHMUG", "WGSF22_9"),
        ("JOSTIP", "JSHMUG", "WGSF22_0"),
    ]


def main() -> None:
    test_makefile()
    test_frontend_script()
    test_n64_main()
    test_renderer_contract()
    print("Fix39 V12i HSTD/renderer regression: PASS")


if __name__ == "__main__":
    main()
