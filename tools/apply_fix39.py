#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import shutil
from pathlib import Path

BEGIN = "# BEGIN FIX39 SOURCE-DIRECT MERGE"
END = "# END FIX39 SOURCE-DIRECT MERGE"

# Combat2DN dependency-closed selective ownership policy: do NOT let basename overlap decide ownership.
# The live N64 project has newer core/arcade frontend, attract, high-score,
# ring/rope and RNG modules that must remain authoritative.  Only the translated
# combat stack below supersedes a same-named core/arcade implementation.
FIX39_COMBAT_OWNERS = {
    "wm_arcade_anim_combat.c", "wm_arcade_attach_anim.c",
    "wm_arcade_bam.c", "wm_arcade_bret.c", "wm_arcade_bret_tables.c",
    "wm_arcade_combat.c", "wm_arcade_doink.c", "wm_arcade_drone.c",
    "wm_arcade_lex.c", "wm_arcade_move_dispatch.c",
    "wm_arcade_razor.c", "wm_arcade_razor_tables.c",
    "wm_arcade_react.c",
    *(f"wm_arcade_react{i}_core.c" for i in range(1, 10)),
    "wm_arcade_shawn.c", "wm_arcade_special.c", "wm_arcade_taker.c",
    "wm_arcade_wrestler_port.c", "wm_arcade_yoko.c",
    # Combat2DN dependency-closed shared providers: source-backed implementations
    # required by the Fix39 combat runtime while frontend adapters remain core-owned.
    "wmania_ring_geometry.c", "wmania_rng.c",
    # Combat2DQ: ATTRACT is one ABI family; do not mix core/Fix39 layouts.
    "wmania_attract_adapter.c", "wmania_attract_core.c", "wmania_attract_data.c",
    "wmania_attract_live.c", "wmania_attract_operator.c", "wmania_attract_secret.c",
    "wmania_attract_time.c", "wmania_attract_visuals.c",
}

# These overlapping modules intentionally remain owned by src/core/arcade.
# Keeping this explicit makes ownership reviewable and prevents DI's accidental
# replacement of the already-working eight-wrestler attract/presenter path.
CORE_PRESERVE_PREFIXES = ("wmania_attract_", "wmania_hiscore_",
                          "wmania_ring_", "wmania_rope_")
CORE_PRESERVE_NAMES = {"wm_arcade_roster.c"}

def fix39_owns_overlap(name: str) -> bool:
    return name in FIX39_COMBAT_OWNERS

def core_owns_overlap(name: str) -> bool:
    return name in CORE_PRESERVE_NAMES or name.startswith(CORE_PRESERVE_PREFIXES)


def fail(msg: str) -> None:
    raise SystemExit(f"Fix39 apply error: {msg}")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        fail(f"expected exactly one {label} anchor, found {count}")
    return text.replace(old, new, 1)


def find_matching_brace(text: str, open_pos: int) -> int:
    """Return the index of the brace matching text[open_pos].

    This small scanner understands strings/chars/comments so source comments or
    format strings containing braces do not confuse the patcher.
    """
    if open_pos < 0 or open_pos >= len(text) or text[open_pos] != "{":
        fail("internal brace scanner called without an opening brace")

    depth = 0
    i = open_pos
    state = "code"
    while i < len(text):
        c = text[i]
        n = text[i + 1] if i + 1 < len(text) else ""

        if state == "code":
            if c == '"':
                state = "string"
            elif c == "'":
                state = "char"
            elif c == "/" and n == "/":
                state = "line_comment"
                i += 1
            elif c == "/" and n == "*":
                state = "block_comment"
                i += 1
            elif c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    return i
        elif state == "string":
            if c == "\\":
                i += 1
            elif c == '"':
                state = "code"
        elif state == "char":
            if c == "\\":
                i += 1
            elif c == "'":
                state = "code"
        elif state == "line_comment":
            if c == "\n":
                state = "code"
        elif state == "block_comment":
            if c == "*" and n == "/":
                state = "code"
                i += 1
        i += 1

    fail("unbalanced braces while patching C source")


def function_span(text: str, name: str) -> tuple[int, int]:
    # Locate a real C function definition by name, allowing arbitrary return
    # type formatting and wrapped parameter lines.
    m = re.search(rf"(?m)^[ \t]*(?:static[ \t]+)?[^\n;]*\b{re.escape(name)}[ \t]*\(", text)
    if not m:
        fail(f"could not find function {name}")
    open_pos = text.find("{", m.start())
    semi_pos = text.find(";", m.start(), open_pos if open_pos >= 0 else len(text))
    if open_pos < 0 or (semi_pos >= 0 and semi_pos < open_pos):
        fail(f"found declaration but not definition for {name}")
    close_pos = find_matching_brace(text, open_pos)
    end = close_pos + 1
    if end < len(text) and text[end] == "\r":
        end += 1
    if end < len(text) and text[end] == "\n":
        end += 1
    return m.start(), end


def replace_function(text: str, name: str, replacement: str) -> str:
    start, end = function_span(text, name)
    if not replacement.endswith("\n"):
        replacement += "\n"
    return text[:start] + replacement + text[end:]


def replace_attract_call_switch(text: str) -> str:
    """Wire Fix39 into the live attract dispatch without relying on
    wm_app_tick function-span discovery.

    The baseline has exactly one switch whose controlling expression is
    app->attract.call. Search the complete translation unit for that C
    construct, then replace its balanced-brace body.
    """
    if "wm_fix39_attract_screen_tick(" in text:
        return text

    pat = re.compile(
        r"\bswitch[ \t\r\n]*\([ \t\r\n]*app->attract\.call"
        r"[ \t\r\n]*\)[ \t\r\n]*\{"
    )
    matches = list(pat.finditer(text))
    if len(matches) != 1:
        nearby = []
        for ln, line in enumerate(text.splitlines(), 1):
            if "attract.call" in line or ("switch" in line and "attract" in line):
                nearby.append(f"{ln}:{line.strip()}")
        fail(
            "expected one translation-unit attract call switch, found "
            + str(len(matches)) + "; nearby=" + " | ".join(nearby)
        )

    sm = matches[0]
    open_pos = text.find("{", sm.start(), sm.end() + 1)
    if open_pos < 0:
        fail("attract call switch matched without opening brace")
    close_pos = find_matching_brace(text, open_pos)

    old_body = text[sm.start():close_pos + 1]
    for required in (
        "WM_ATTRACT_DCS_LOGO",
        "WM_ATTRACT_SHOW_TITLE",
    ):
        if required not in old_body:
            fail(f"attract call switch missing expected case {required}")

    new_switch = '''switch (app->attract.call) {
        case WM_ATTRACT_DCS_LOGO: done = tick_dcs_logo(app, input); break;
        case WM_ATTRACT_SHOW_SPORTS_LOGO: done = tick_sports_logo(app, input); break;
        case WM_ATTRACT_SHOW_TITLE: done = tick_title(app, input); break;
        default:
            done = wm_fix39_attract_screen_tick(
                wm_app_any_attract_button(input));
            break;
    }'''

    line_start = text.rfind("\n", 0, sm.start()) + 1
    indent = text[line_start:sm.start()]
    indented = new_switch.replace("\n", "\n" + indent)
    return text[:sm.start()] + indented + text[close_pos + 1:]


def patch_public_attract_data_abi(path: Path) -> None:
    if not path.is_file():
        fail(f"Combat2DQ public ATTRACT data header missing: {path}")
    text = path.read_text()
    text = re.sub(r"#define WM_ATTRACT_ACTIVE_HINTS\s+\d+u", "#define WM_ATTRACT_ACTIVE_HINTS 10u", text, count=1)
    pat = re.compile(r"typedef struct \{\n\s*const char \*title_label;\n\s*const char \*body_label;\n\s*const char \*tip_image_symbol;\n\s*const char \*mug_image_symbol;\n\s*uint8_t number_image_index;\n\} WmAttractHint;")
    repl = "typedef struct {\n    const char *title_label;\n    const char *body_label;\n    const char *body_line_labels[6];\n    const char *tip_image_symbol;\n    const char *mug_image_symbol;\n    const char *number_image_symbol;\n    uint8_t number_image_index;\n    uint8_t body_line_count;\n} WmAttractHint;"
    text, n = pat.subn(repl, text, count=1)
    if n != 1 and ("const char *body_line_labels[6];" not in text or "uint8_t body_line_count;" not in text):
        fail("Combat2DQ could not converge public WmAttractHint ABI")
    path.write_text(text)

def patch_cmake(path: Path, sources: list[str]) -> None:
    text = path.read_text()

    # Combat2DM: remove a baseline owner only when the explicit subsystem
    # policy says Fix39 owns that overlap.  Frontend/attract/high-score/ring
    # overlaps remain on their newer core implementation.
    for src in sources:
        if not fix39_owns_overlap(src):
            continue
        text = re.sub(
            rf"(?m)^[ \t]*src/core/arcade/{re.escape(src)}[ \t]*\n",
            "",
            text,
        )

    # Demo3 collision binding calls wm_bret_sprite_find(), whose implementation
    # is generated as src/generated/bret_sprites.c by prepare_bret_sprites.sh.
    # The stock host CMake list only carries the visual/attack tables, so make
    # the generated sprite lookup implementation a real wmcore source too.
    if "src/generated/bret_sprites.c" not in text:
        anchor = "    src/generated/bret_attacks.c\n"
        if anchor not in text:
            anchor = "src/generated/bret_attacks.c\n"
        if anchor not in text:
            fail("could not find bret_attacks.c CMake anchor for bret_sprites.c")
        text = text.replace(anchor, anchor + "    src/generated/bret_sprites.c\n", 1)

    # Do not compile a Fix39 copy when a preserved core overlap exists.  The
    # core tree is known only in the cloned worktree, so discover it here.
    arcade_dir = path.parent / "src" / "core" / "arcade"
    arcade_names = {p.name for p in arcade_dir.glob("*.c")}
    cmake_sources = [src for src in sources
                     if src not in arcade_names or fix39_owns_overlap(src)]
    block = "\n" + BEGIN + "\n"
    block += "".join(f"    src/fix39/{src}\n" for src in cmake_sources)
    block += END
    if BEGIN in text:
        b = text.index(BEGIN)
        e = text.find(END, b)
        if e < 0:
            fail("CMake has Fix39 begin marker without end marker")
        e += len(END)
        # Preserve the newline before BEGIN so repeated integration is stable.
        if b > 0 and text[b - 1] == "\n":
            b -= 1
        text = text[:b] + block + text[e:]
    else:
        m = re.search(r"add_library\s*\(\s*wmcore\s+STATIC\b", text)
        if not m:
            fail("could not find add_library(wmcore STATIC ...) in CMakeLists.txt")
        close = text.find("\n)", m.end())
        if close < 0:
            fail("could not find end of wmcore source list in CMakeLists.txt")
        text = text[:close] + block + text[close:]

    inc_line = "target_include_directories(wmcore PUBLIC include)"
    if "target_include_directories(wmcore PUBLIC include src/fix39)" not in text:
        if inc_line not in text:
            fail("could not find wmcore include-directories line")
        text = text.replace(
            inc_line,
            "target_include_directories(wmcore PUBLIC include src/fix39)",
            1,
        )

    if "add_executable(wm_fix39_tests" not in text:
        anchor = "    add_test(NAME wm_core COMMAND wm_tests)\n"
        if anchor not in text:
            fail("could not find wm_core test anchor in CMakeLists.txt")
        test_block = anchor + (
            "    add_executable(wm_fix39_tests tests/fix39_smoke.c)\n"
            "    target_link_libraries(wm_fix39_tests PRIVATE wmcore)\n"
            "    target_compile_options(wm_fix39_tests PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)\n"
            "    add_test(NAME wm_fix39 COMMAND wm_fix39_tests)\n"
        )
        text = text.replace(anchor, test_block, 1)

    # Upgrade an already-integrated tree too: Release builds define NDEBUG,
    # which would compile assert()-based smoke checks away.
    old_opts = "target_compile_options(wm_fix39_tests PRIVATE -Wall -Wextra -Wpedantic)"
    new_opts = "target_compile_options(wm_fix39_tests PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)"
    if old_opts in text:
        text = text.replace(old_opts, new_opts, 1)

    path.write_text(text)



def patch_github_workflow(path: Path) -> None:
    text = path.read_text()
    marker = "# FIX39 DEMO3 HOST GENERATED SPRITES"
    anchor = "      - name: Configure portable verifier\n        run: cmake -S . -B build-host -DCMAKE_BUILD_TYPE=Release\n"
    if anchor not in text:
        fail("could not find host-verification configure step in .github/workflows/build.yml")

    if marker not in text:
        prep = (
            "      - name: Prepare generated Bret sprites for host verifier\n"
            "        # FIX39 DEMO3 HOST GENERATED SPRITES\n"
            "        run: |\n"
            "          set -euo pipefail\n"
            "          sh ./scripts/prepare_bret_sprites.sh\n"
            "          test -s src/generated/bret_sprites.c\n"
        )
        text = text.replace(anchor, prep + anchor, 1)

    marker5 = "# FIX39 DEMO5 SOURCE ATTACK FRAMES"
    if marker5 not in text:
        prep5 = (
            "      - name: Generate Bret attack frames from historical source\n"
            "        # FIX39 DEMO5 SOURCE ATTACK FRAMES\n"
            "        run: |\n"
            "          set -euo pipefail\n"
            "          test -f original/wwf-wrestlemania/HRTSEQ2.ASM || sh ./scripts/fetch_original.sh\n"
            "          python3 tools/fix39_bret_attack_frames.py --source original/wwf-wrestlemania/HRTSEQ2.ASM --out src/fix39/wm_arcade_bret_attack_frames_generated.h\n"
            "          grep -q 'WM_FIX39_BRET_ATTACK_FRAMES_GENERATED 1' src/fix39/wm_arcade_bret_attack_frames_generated.h\n"
        )
        text = text.replace(anchor, prep5 + anchor, 1)
    graph_marker = "# COMBAT2DI BUILD GRAPH AUTHORITY"
    if graph_marker not in text:
        graph_step = (
            "      - name: Verify Fix39 host/N64 source ownership\n"
            "        # COMBAT2DI BUILD GRAPH AUTHORITY\n"
            "        run: python3 tools/fix39_build_graph_audit.py .\n"
        )
        text = text.replace(anchor, graph_step + anchor, 1)

    path.write_text(text)

def patch_makefile(path: Path, sources: list[str]) -> tuple[list[str], list[str]]:
    text = path.read_text()

    if "-I$(CURDIR)/src/fix39" not in text:
        include_anchor = "CFLAGS += -I$(CURDIR)/include"
        if include_anchor not in text:
            fail("could not find Makefile CFLAGS include line")
        text = text.replace(
            include_anchor,
            include_anchor + " -I$(CURDIR)/src/fix39 -I$(CURDIR)/src/generated",
            1,
        )
    elif "-I$(CURDIR)/src/generated" not in text:
        text = text.replace(
            "-I$(CURDIR)/src/fix39",
            "-I$(CURDIR)/src/fix39 -I$(CURDIR)/src/generated",
            1,
        )

    # Combat2DN dependency-closed selective convergence: only the translated combat stack
    # supersedes same-named core objects.  Preserve the newer frontend/attract,
    # high-score, ring/rope, roster and RNG owners.
    arcade_dir = path.parent / "src" / "core" / "arcade"
    arcade_is_built = "src/core/arcade" in text
    arcade_names = {p.name for p in arcade_dir.glob("*.c")} if arcade_is_built else set()
    overlaps = sorted(set(sources) & arcade_names)
    fix39_overlaps = [src for src in overlaps if fix39_owns_overlap(src)]
    preserved_overlaps = [src for src in overlaps if not fix39_owns_overlap(src)]

    for src in fix39_overlaps:
        text = re.sub(
            rf"(?m)^[ \t]*src/core/arcade/{re.escape(src)}[ \t]*\\?[ \t]*\n",
            "",
            text,
        )

    make_sources = [src for src in sources if src not in preserved_overlaps]
    deduped = fix39_overlaps

    if not make_sources:
        fail("Makefile Fix39 source list unexpectedly empty")
    if "wm_fix39_runtime.c" not in make_sources:
        fail("Fix39 runtime missing from authoritative N64 source list")

    block = BEGIN + "\nFIX39_C := \\\n"
    for i, src in enumerate(make_sources):
        tail = " \\\n" if i != len(make_sources) - 1 else "\n"
        block += f"    src/fix39/{src}{tail}"
    block += END

    if BEGIN in text:
        b = text.index(BEGIN)
        e = text.find(END, b)
        if e < 0:
            fail("Makefile has Fix39 begin marker without end marker")
        e += len(END)
        text = text[:b] + block + text[e:]
    else:
        asset_pos = text.find("ASSET_C :=")
        if asset_pos < 0:
            fail("could not find ASSET_C in Makefile")
        text = text[:asset_pos] + block + "\n" + text[asset_pos:]

    # Keep non-overlapping baseline CORE_C/ARCADE_C and assets; Fix39 owns overlaps.
    m = re.search(r"(?m)^C_FILES\s*:=\s*(.*)$", text)
    if not m:
        fail("could not find C_FILES assignment in Makefile")
    line = m.group(0)
    if "$(FIX39_C)" not in line:
        newline = line.replace("C_FILES :=", "C_FILES := $(FIX39_C)", 1)
        text = text[:m.start()] + newline + text[m.end():]

    # Combat2CG: the full ANIM.ASM command stream is too large to reside in
    # the 4 MiB N64 ELF.  Keep only the compact index/loader resident and make
    # the per-program/per-table bytecode part of DragonFS.
    anim_dfs_marker = "# BEGIN FIX39 STREAMED ANIM VM"
    if anim_dfs_marker not in text:
        anim_rule = (
            "\n# BEGIN FIX39 STREAMED ANIM VM\n"
            "FIX39_ANIM_DFS_FILES := $(wildcard filesystem/fix39_anim/programs/*.bin) $(wildcard filesystem/fix39_anim/tables/*.bin)\n"
            "$(BUILD_DIR)/$(ROMNAME).dfs: $(FIX39_ANIM_DFS_FILES)\n"
            "$(ROMNAME).z64: $(BUILD_DIR)/$(ROMNAME).dfs\n"
            "# END FIX39 STREAMED ANIM VM\n"
        )
        dfs_anchor = "$(ROMNAME).z64: $(BUILD_DIR)/$(ROMNAME).dfs\n"
        if dfs_anchor in text:
            text = text.replace(dfs_anchor, dfs_anchor + anim_rule, 1)
        else:
            text += anim_rule

    # V11 source-exact text bridge. GitHub's N64 job has already fetched the
    # historical source before make runs. Generate the visible ATTRACT strings
    # from that checkout instead of carrying a hand-maintained duplicate table.
    gen_c = "src/generated/fix39_attract_text_generated.c"
    gen_h = "src/generated/fix39_attract_text_generated.h"
    m_asset = re.search(r"(?m)^ASSET_C\s*:=\s*(.*)$", text)
    if not m_asset:
        fail("could not find ASSET_C assignment for V11 attract text generator")
    if gen_c not in m_asset.group(0):
        newline = m_asset.group(0) + " " + gen_c
        text = text[:m_asset.start()] + newline + text[m_asset.end():]

    rule_marker = "# BEGIN FIX39 ATTRACT SOURCE TEXT"
    if rule_marker not in text:
        rule = (
            "\n# BEGIN FIX39 ATTRACT SOURCE TEXT\n"
            "FIX39_ATTRACT_ASM := original/wwf-wrestlemania/ATTRACT.ASM\n"
            f"FIX39_ATTRACT_TEXT_C := {gen_c}\n"
            f"FIX39_ATTRACT_TEXT_H := {gen_h}\n"
            "$(FIX39_ATTRACT_TEXT_C) $(FIX39_ATTRACT_TEXT_H): $(FIX39_ATTRACT_ASM) tools/fix39_attract_text.py\n"
            "\tpython3 tools/fix39_attract_text.py --source $(FIX39_ATTRACT_ASM) --out-c $(FIX39_ATTRACT_TEXT_C) --out-h $(FIX39_ATTRACT_TEXT_H)\n"
            "# END FIX39 ATTRACT SOURCE TEXT\n"
        )
        insert_at = text.find("$(BUILD_DIR)/$(ROMNAME).elf:")
        if insert_at < 0:
            fail("could not find N64 ELF dependency anchor")
        text = text[:insert_at] + rule + "\n" + text[insert_at:]

    # V12: generate exact WIMP font/hint/logo pixels and named palettes from
    # the historical source checkout fetched by the N64 CI build.
    asset_gen_c = "src/generated/fix39_attract_assets.c"
    asset_gen_h = "src/generated/fix39_attract_assets_generated.h"
    m_asset = re.search(r"(?m)^ASSET_C\s*:=\s*(.*)$", text)
    if not m_asset:
        fail("could not find ASSET_C assignment for V12 attract assets")
    if asset_gen_c not in m_asset.group(0):
        newline = m_asset.group(0) + " " + asset_gen_c
        text = text[:m_asset.start()] + newline + text[m_asset.end():]

    asset_rule_marker = "# BEGIN FIX39 ATTRACT SOURCE ASSETS"
    if asset_rule_marker not in text:
        rule = (
            "\n# BEGIN FIX39 ATTRACT SOURCE ASSETS\n"
            "FIX39_ATTRACT_IMG_DIR := original/wwf-wrestlemania/IMG\n"
            "FIX39_ATTRACT_IMGPAL := original/wwf-wrestlemania/IMGPAL.ASM\n"
            f"FIX39_ATTRACT_ASSET_C := {asset_gen_c}\n"
            f"FIX39_ATTRACT_ASSET_H := {asset_gen_h}\n"
            "$(FIX39_ATTRACT_ASSET_C) $(FIX39_ATTRACT_ASSET_H): tools/fix39_attract_assets.py tools/wimpimg.py\n"
            "\t@test -f $(FIX39_ATTRACT_IMGPAL) || sh scripts/fetch_original.sh\n"
            "\tpython3 tools/fix39_attract_assets.py --img-dir $(FIX39_ATTRACT_IMG_DIR) --imgpal $(FIX39_ATTRACT_IMGPAL) --wimpimg tools/wimpimg.py --out-c $(FIX39_ATTRACT_ASSET_C) --out-h $(FIX39_ATTRACT_ASSET_H)\n"
            "# END FIX39 ATTRACT SOURCE ASSETS\n"
        )
        insert_at = text.find("$(BUILD_DIR)/$(ROMNAME).elf:")
        if insert_at < 0:
            fail("could not find N64 ELF dependency anchor for V12 assets")
        text = text[:insert_at] + rule + "\n" + text[insert_at:]

    dep_marker = "# BEGIN FIX39 ATTRACT GENERATED HEADER ORDER"
    if dep_marker not in text:
        dep = (
            "\n# BEGIN FIX39 ATTRACT GENERATED HEADER ORDER\n"
            "$(BUILD_DIR)/src/platform/n64/main.o: $(FIX39_ATTRACT_TEXT_H) $(FIX39_ATTRACT_ASSET_H)\n"
            "# END FIX39 ATTRACT GENERATED HEADER ORDER\n"
        )
        insert_at = text.find("$(BUILD_DIR)/$(ROMNAME).elf:")
        if insert_at < 0:
            fail("could not find N64 ELF dependency anchor for V12 header ordering")
        text = text[:insert_at] + dep + "\n" + text[insert_at:]

    demo5_marker = "# BEGIN FIX39 BRET SOURCE ATTACK FRAMES"
    if demo5_marker not in text:
        rule = (
            "\n# BEGIN FIX39 BRET SOURCE ATTACK FRAMES\n"
            "FIX39_HRTSEQ2 := original/wwf-wrestlemania/HRTSEQ2.ASM\n"
            "FIX39_BRET_ATTACK_H := src/fix39/wm_arcade_bret_attack_frames_generated.h\n"
            "$(FIX39_BRET_ATTACK_H): $(FIX39_HRTSEQ2) tools/fix39_bret_attack_frames.py\n"
            "\tpython3 tools/fix39_bret_attack_frames.py --source $(FIX39_HRTSEQ2) --out $(FIX39_BRET_ATTACK_H)\n"
            "$(BUILD_DIR)/src/fix39/wm_arcade_source_attack_frames.o: $(FIX39_BRET_ATTACK_H)\n"
            "# END FIX39 BRET SOURCE ATTACK FRAMES\n"
        )
        insert_at = text.find("$(BUILD_DIR)/$(ROMNAME).elf:")
        if insert_at < 0:
            fail("could not find N64 ELF dependency anchor for Demo5 attack frames")
        text = text[:insert_at] + rule + "\n" + text[insert_at:]

    path.write_text(text)
    return make_sources, deduped

def patch_frontend_assets_script(path: Path) -> None:
    """Include ATTRACT.ASM's slateBMOD and bind the project-retained Sports
    replacement foreground to the existing source converter.

    The override is still SPORTLO8.IMG: frontend_bundle.py continues to decode
    the real SPRTLG01..SPRTLG17 WIMP objects. We only redirect the input file;
    timing, placement, object count and rendering code stay untouched.
    """
    if not path.is_file():
        fail(f"expected frontend asset script at {path}")
    text = path.read_text()

    if "--module slateBMOD" not in text:
        anchor = "    --module LADDERBMOD \\\n"
        if anchor not in text:
            fail("could not find LADDERBMOD anchor in prepare_frontend_assets.sh")
        text = text.replace(anchor, anchor + "    --module slateBMOD \\\n", 1)

    marker = "# FIX39 SPORTS FOREGROUND OVERRIDE"
    if marker not in text:
        anchor = 'SPORTS_SOURCE="$ORIG/IMG/SPORTLO8.IMG"\n'
        if anchor in text:
            override = anchor + (
            '# FIX39 SPORTS FOREGROUND OVERRIDE\n'
            'FIX39_SPORTS_SOURCE="$ROOT/assets/fix39_sports_override/SPORTLO8.IMG"\n'
            'if [ -f "$FIX39_SPORTS_SOURCE" ]; then\n'
            '  SPORTS_SOURCE="$FIX39_SPORTS_SOURCE"\n'
                'fi\n'
            )
            text = text.replace(anchor, override, 1)

    regen_marker = "# FIX39 SPORTS BACKGROUND REGEN"
    if regen_marker not in text:
        text = text.rstrip() + (
            "\n\n# FIX39 SPORTS BACKGROUND REGEN\n"
            'if [ -f "$ROOT/assets/fix39_sports_override/SPORTBK.IMG" ]; then\n'
            '  sh "$ROOT/scripts/prepare_sports_source_assets.sh"\n'
            'fi\n'
        )

    path.write_text(text)


def patch_sports_source_assets_script(path: Path) -> None:
    """Bind the replacement SPORTBK.IMG and its WIMP-authored palette.

    The stock converter correctly uses BGNDPAL.ASM for the untouched arcade
    asset, but the repurposed MIDWAY tile carries a changed 29-color palette
    inside the round-tripped WIMP container.  Route only the Fix39 override
    through fix39_sports_background_bundle.py; keep stock behavior untouched
    when the override is absent.
    """
    if not path.is_file():
        fail(f"expected sports source asset script at {path}")
    text = path.read_text()

    marker = "# FIX39 SPORTS BACKGROUND OVERRIDE"
    if marker not in text:
        anchor = 'SPORTS_BG_SOURCE="$ORIG/IMG/SPORTBK.IMG"\n'
        if anchor not in text:
            fail("could not find SPORTBK source anchor in prepare_sports_source_assets.sh")
        override = anchor + (
            '# FIX39 SPORTS BACKGROUND OVERRIDE\n'
            'FIX39_SPORTS_BG_SOURCE="$ROOT/assets/fix39_sports_override/SPORTBK.IMG"\n'
            'SPORTS_BG_TOOL="$ROOT/tools/sports_background_bundle.py"\n'
            'if [ -f "$FIX39_SPORTS_BG_SOURCE" ]; then\n'
            '  SPORTS_BG_SOURCE="$FIX39_SPORTS_BG_SOURCE"\n'
            '  SPORTS_BG_TOOL="$ROOT/tools/fix39_sports_background_bundle.py"\n'
            'fi\n'
        )
        text = text.replace(anchor, override, 1)

    stock_call = 'python3 "$ROOT/tools/sports_background_bundle.py" ' + "\\\n"
    fix_call = 'python3 "$SPORTS_BG_TOOL" ' + "\\\n"
    if stock_call in text:
        text = text.replace(stock_call, fix_call, 1)
    elif fix_call not in text:
        fail("could not find sports background converter call in prepare_sports_source_assets.sh")

    path.write_text(text)

def patch_core_tests(path: Path) -> None:
    """Update the one frontend test that intentionally tracked the old
    provisional title RNG state.  Fix39 replaces that bridge with the shared
    translated arcade RAND/RNDRNG0 service, so the legacy per-title state is
    no longer the thing that should advance.
    """
    if not path.is_file():
        fail(f"expected core tests at {path}")

    text = path.read_text()

    if '#include "wm_fix39_runtime.h"' not in text:
        anchor = '#include "wm/app.h"\n'
        if anchor not in text:
            fail("could not find wm/app.h include in tests/test_core.c")
        text = text.replace(anchor, anchor + '#include "wm_fix39_runtime.h"\n', 1)

    old = 'CHECK(app.attract.title_random_state != 0x57574631u);'
    new = ('/* Fix39 replaced the provisional per-title RNG with the shared '
           'source RAND/RNDRNG0 state. */\n'
           '    CHECK(wm_fix39_rng_state() != 0u);')
    if old in text:
        text = text.replace(old, new, 1)
    elif 'CHECK(wm_fix39_rng_state() != 0u);' not in text:
        fail("could not find provisional title RNG assertion in tests/test_core.c")

    # Attract Demo 1c: SHOW_GAMEPLAY is now a real timed attract call. Patch
    # the current upstream test structurally instead of depending on one exact
    # whitespace/comment snapshot. This is idempotent because the GitHub base
    # can already contain part of a previous test sync.
    demo_marker = 'for (unsigned i = 0; i < 60u; ++i)'
    if demo_marker not in text:
        flow_re = re.compile(
            r'(?P<indent>^[ \t]*)wm_app_tick\(&app,\s*&button\);\s*\n'
            r'(?P=indent)CHECK\(wm_process_find_id\(&app\.scheduler,\s*WM_PID_WATER\)\s*==\s*NULL\);\s*\n'
            r'(?P=indent)CHECK\(app\.attract\.call\s*==\s*WM_ATTRACT_SHOW_TITLE\);\s*\n'
            r'(?P=indent)unsigned initial_lava = app\.attract\.title_lava_step;',
            re.MULTILINE,
        )
        m = flow_re.search(text)
        if not m:
            # A prior sync may already expect SHOW_GAMEPLAY but use a different
            # loop spelling. Accept that state rather than failing the apply.
            nearby = re.search(
                r'WM_PID_WATER\)\s*==\s*NULL\).*?WM_ATTRACT_SHOW_GAMEPLAY.*?title_lava_step',
                text,
                re.DOTALL,
            )
            if not nearby:
                fail("could not locate Sports->gameplay attract test region in tests/test_core.c")
        else:
            ind = m.group('indent')
            replacement = (
                f"{ind}wm_app_tick(&app, &button);\n"
                f"{ind}CHECK(wm_process_find_id(&app.scheduler, WM_PID_WATER) == NULL);\n"
                f"{ind}CHECK(app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY);\n"
                f"{ind}/* Gameplay now owns a real attract window. With RUN held, it becomes\n"
                f"{ind}   skippable after 60 ticks and then advances past the pending credit\n"
                f"{ind}   screen to the translated title call. */\n"
                f"{ind}for (unsigned i = 0; i < 60u; ++i) {{\n"
                f"{ind}    wm_app_tick(&app, &button);\n"
                f"{ind}    CHECK(app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY);\n"
                f"{ind}}}\n"
                f"{ind}wm_app_tick(&app, &button);\n"
                f"{ind}CHECK(app.attract.call == WM_ATTRACT_SHOW_TITLE);\n"
                f"{ind}unsigned initial_lava = app.attract.title_lava_step;"
            )
            text = text[:m.start()] + replacement + text[m.end():]

    # Attract Demo 1d: the source loop contains a second SHOW_GAMEPLAY after
    # Title. The old core test expected Title to skip straight back to DCS
    # because gameplay used to be harness-only. Keep the test source-shaped:
    # enter demo #2, preserve the 60-tick unskippable window, then RUN skips
    # it and pending unsupported tail screens collapse to the next DCS loop.
    second_demo_marker = '/* Fix39: second source gameplay slot after Title. */'
    if second_demo_marker not in text:
        tail_re = re.compile(
            r'(?P<indent>^[ \t]*)wm_app_tick\(&app,\s*&button\);\s*\n'
            r'(?P=indent)CHECK\(wm_process_find_id\(&app\.scheduler,\s*WM_PID_CYCLE_LAVA\)\s*==\s*NULL\);\s*\n'
            r'(?P=indent)CHECK\(wm_process_find_id\(&app\.scheduler,\s*WM_PID_FLASH\)\s*==\s*NULL\);\s*\n'
            r'(?P=indent)CHECK\(wm_process_find_id\(&app\.scheduler,\s*WM_PID_ATTRACT_ANIM\)\s*==\s*NULL\);\s*\n'
            r'(?P=indent)CHECK\(app\.attract\.call\s*==\s*WM_ATTRACT_DCS_LOGO\);',
            re.MULTILINE,
        )
        m2 = tail_re.search(text)
        if not m2:
            # Accept an already-synced checkout rather than making the apply
            # depend on one exact upstream test snapshot.
            if second_demo_marker not in text and not re.search(
                r'WM_PID_ATTRACT_ANIM\)\s*==\s*NULL\).*?WM_ATTRACT_SHOW_GAMEPLAY.*?WM_ATTRACT_DCS_LOGO',
                text, re.DOTALL):
                fail('could not locate Title->second-gameplay attract test region in tests/test_core.c')
        else:
            ind = m2.group('indent')
            replacement = (
                f"{ind}wm_app_tick(&app, &button);\n"
                f"{ind}CHECK(wm_process_find_id(&app.scheduler, WM_PID_CYCLE_LAVA) == NULL);\n"
                f"{ind}CHECK(wm_process_find_id(&app.scheduler, WM_PID_FLASH) == NULL);\n"
                f"{ind}CHECK(wm_process_find_id(&app.scheduler, WM_PID_ATTRACT_ANIM) == NULL);\n"
                f"{ind}CHECK(app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY);\n"
                f"{ind}/* Fix39: second source gameplay slot after Title. */\n"
                f"{ind}for (unsigned i = 0; i < 60u; ++i) {{\n"
                f"{ind}    wm_app_tick(&app, &button);\n"
                f"{ind}    CHECK(app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY);\n"
                f"{ind}}}\n"
                f"{ind}wm_app_tick(&app, &button);\n"
                f"{ind}CHECK(app.attract.call == WM_ATTRACT_DCS_LOGO);"
            )
            text = text[:m2.start()] + replacement + text[m2.end():]

    # Attract Demo 1e: active gameplay demos intentionally mutate the legacy
    # presentation-demo combat counters. The old core-flow test asserted both
    # counters stayed zero because SHOW_GAMEPLAY was skipped entirely. That is
    # no longer a valid attract-flow invariant; combat behavior is covered by
    # the dedicated demo/runtime tests. Remove only those stale zero assertions.
    demo_counter_marker = '/* Fix39: live attract gameplay mutates demo combat counters. */'
    if demo_counter_marker not in text:
        counters_re = re.compile(
            r'(?P<indent>^[ \t]*)CHECK\(app\.demo\.total_hits\s*==\s*0\);\s*\n'
            r'(?P=indent)CHECK\(app\.demo\.total_blocks\s*==\s*0\);',
            re.MULTILINE,
        )
        mc = counters_re.search(text)
        if mc:
            ind = mc.group('indent')
            replacement = (
                f"{ind}{demo_counter_marker}\n"
                f"{ind}/* Counter values are outcome-dependent once CPU-vs-CPU gameplay runs. */"
            )
            text = text[:mc.start()] + replacement + text[mc.end():]
        elif ('CHECK(app.demo.total_hits == 0);' in text or
              'CHECK(app.demo.total_blocks == 0);' in text):
            fail('could not structurally remove stale attract demo combat-counter assertions')

    # Combat2AE: audit the entire movement/presentation portion of the demo
    # core test in one pass.  The upstream test was written when wm_demo was
    # Bret-only and therefore hard-coded four Bret movement sequences.  Run
    # the same movement assertions with roster 4 selected from the beginning
    # and compare every presenter sequence through the character backend.
    if '#include "wm/character_assets.h"' not in text:
        inc = '#include "wm/app.h"\n'
        if inc not in text:
            fail("Combat2AE: wm/app.h include missing while patching roster-aware demo tests")
        text = text.replace(inc, inc + '#include "wm/character_assets.h"\n', 1)

    demo_fn_start = text.find('static void test_demo_four_way_and_run(void) {')
    demo_fn_end = text.find('\nstatic bool is_attack_action', demo_fn_start)
    if demo_fn_start < 0 or demo_fn_end < 0:
        fail("Combat2AE: could not locate test_demo_four_way_and_run")
    demo_fn = text[demo_fn_start:demo_fn_end]
    roster_anchor = '    d.ai_enabled = false;\n'
    if 'wm_demo_set_roster(&d, 4u, 0u);' not in demo_fn:
        if roster_anchor not in demo_fn:
            fail("Combat2AE: demo roster insertion anchor missing")
        demo_fn = demo_fn.replace(roster_anchor, roster_anchor + '    wm_demo_set_roster(&d, 4u, 0u);\n', 1)

    movement = {
        '&wm_bret_walk6_f4_anim': 'wm_character_visual(4u, WM_CV_WALK6)',
        '&wm_bret_walk4_f4_anim': 'wm_character_visual(4u, WM_CV_WALK4)',
        '&wm_bret_run_anim': 'wm_character_visual(4u, WM_CV_RUN)',
        '&wm_bret_walk2_f2_anim': 'wm_character_visual(4u, WM_CV_WALK2)',
    }
    for old, generic in movement.items():
        demo_fn = demo_fn.replace(old, generic)

    # Explicitly make a Bret fallback a test failure, not merely a different
    # pointer expectation.  One check is enough because all four movement
    # assertions above already validate the selected wrestler's slots.
    marker = '    CHECK(d.p1.visual.sequence == wm_character_visual(4u, WM_CV_WALK6));\n'
    anti = '    CHECK(d.p1.visual.sequence != wm_character_visual(0u, WM_CV_WALK6));\n'
    if anti not in demo_fn:
        if marker not in demo_fn:
            fail("Combat2AE: walk6 generic assertion missing after audit")
        demo_fn = demo_fn.replace(marker, marker + anti, 1)

    # Fail closed if any old Bret movement expectation survived this function.
    stale_demo = [x for x in movement if x in demo_fn]
    if stale_demo:
        fail("Combat2AE: stale Bret movement assertions survived: " + ", ".join(stale_demo))
    text = text[:demo_fn_start] + demo_fn + text[demo_fn_end:]

    # Combat2AG: the attack tests had the same stale pointer-identity assumption
    # as the movement test.  The generic character backend intentionally emits
    # its own sequence objects, so roster 0 (Bret) no longer pointer-equals the
    # legacy wm_bret_* sequence globals even though it owns the same source art.
    # Keep the attack/action/damage assertions, but expect the canonical
    # character-backend slots used by sequence_for().
    attack_expect = {
        '&wm_bret_light_punch4_anim': 'wm_character_visual(0u, WM_CV_LP4)',
        '&wm_bret_power_punch_anim': 'wm_character_visual(0u, WM_CV_PP)',
        '&wm_bret_light_kick4_anim': 'wm_character_visual(0u, WM_CV_LK4)',
        '&wm_bret_power_kick_anim': 'wm_character_visual(0u, WM_CV_PK)',
    }
    attack_start = text.find('static void test_four_attack_buttons(void) {')
    attack_end = text.find('\nstatic void test_block', attack_start)
    if attack_start < 0 or attack_end < 0:
        fail("Combat2AG: could not locate test_four_attack_buttons")
    attack_fn = text[attack_start:attack_end]
    for old, generic in attack_expect.items():
        attack_fn = attack_fn.replace(old, generic)
    stale_attack = [x for x in attack_expect if x in attack_fn]
    if stale_attack:
        fail("Combat2AG: stale Bret attack expectations survived: " + ", ".join(stale_attack))
    text = text[:attack_start] + attack_fn + text[attack_end:]

    # Fail closed on any remaining legacy Bret pointer expectation in demo
    # behavior tests. Dedicated test_visual_sequences() is intentionally left
    # alone because it validates the legacy Bret tables themselves.
    behavior_start = text.find('static void test_demo_four_way_and_run(void) {')
    behavior_end = text.find('static void test_source_attract_sequence(void) {', behavior_start)
    behavior = text[behavior_start:behavior_end]
    stale = re.findall(r'CHECK\([^\n]*visual\.sequence[^\n]*wm_bret_[^\n]*\);', behavior)
    if stale:
        fail("Combat2AG: stale Bret visual.sequence behavior assertions remain: " + " | ".join(stale))

    path.write_text(text)


def patch_n64_main(path: Path) -> None:
    """Bind V12 source-backed ATTRACT screens to the live N64 renderer."""
    if not path.is_file():
        fail(f"expected N64 platform source at {path}")

    text = path.read_text()
    include_anchor = '#include "wm/app.h"\n'
    if include_anchor not in text:
        fail("could not find wm/app.h include in N64 main.c")
    needed_includes = (
        '#include "wm_fix39_runtime.h"\n',
        '#include "wmania_attract_data.h"\n',
        '#include "wmania_hiscore_present.h"\n',
        '#include "fix39_attract_text_generated.h"\n',
        '#include "fix39_attract_assets_generated.h"\n',
    )
    insert = "".join(x for x in needed_includes if x not in text)
    if insert:
        text = text.replace(include_anchor, include_anchor + insert, 1)

    repl = (
        'static const char *progress_factory_recent_champ(const wm_app *app) {\n'
        '    if (!app) return "";\n'
        '    /* HSTD.ASM source factory/live table owns the recent champion string. */\n'
        '    return wm_fix39_hiscore_recent_initials(\n'
        '        app->pregame.belt_type == WM_PREGAME_BELT_WWF);\n'
        '}\n'
    )
    text = replace_function(text, "progress_factory_recent_champ", repl)

    marker = "/* BEGIN FIX39 V12 ATTRACT RENDERERS */"
    if marker not in text:
        snippet_path = Path(__file__).resolve().parents[1] / "patches" / "fix39_v12_n64_attract.inc"
        if not snippet_path.is_file():
            fail("V12 N64 attract renderer snippet missing from bundle")
        snippet = snippet_path.read_text().rstrip() + "\n\n"
        anchor = "static void render_character_select(const wm_app *app) {"
        if anchor not in text:
            fail("could not find render_character_select anchor in N64 main.c")
        text = text.replace(anchor, snippet + anchor, 1)

    if "case WM_ATTRACT_DO_HINTS:" not in text:
        anchor = (
            "        case WM_ATTRACT_SHOW_TITLE:\n"
            "            render_title_screen(app);\n"
            "            break;\n"
        )
        if anchor not in text:
            fail("could not find SHOW_TITLE render switch anchor")
        cases = anchor + (
            "        case WM_ATTRACT_SHOW_HSTD:\n"
            "            fix39_render_hiscores();\n"
            "            break;\n"
            "        case WM_ATTRACT_DO_HINTS:\n"
            "            fix39_render_hint();\n"
            "            break;\n"
            "        case WM_ATTRACT_SHOW_GEN_TIPS:\n"
            "            fix39_render_general_tips();\n"
            "            break;\n"
            "        case WM_ATTRACT_SHOW_COPYRIGHT:\n"
            "            fix39_render_copyright();\n"
            "            break;\n"
            "        case WM_ATTRACT_AAMA_MESSAGE:\n"
            "            fix39_render_aama();\n"
            "            break;\n"
        )
        text = text.replace(anchor, cases, 1)

    caps_anchor = "    wm_app_init(&app);\n"
    caps_marker = "WM_FIX39_ATTRACT_CAP_HISCORES |"
    if caps_marker not in text:
        if caps_anchor not in text:
            fail("could not find wm_app_init(&app) anchor in N64 main.c")
        caps = caps_anchor + (
            "    wm_fix39_attract_set_platform_capabilities(\n"
            "        WM_FIX39_ATTRACT_CAP_HISCORES |\n"
            "        WM_FIX39_ATTRACT_CAP_DESIGNER_HINT |\n"
            "        WM_FIX39_ATTRACT_CAP_GENERAL_TIPS |\n"
            "        WM_FIX39_ATTRACT_CAP_COPYRIGHT |\n"
            "        WM_FIX39_ATTRACT_CAP_AAMA);\n"
        )
        text = text.replace(caps_anchor, caps, 1)

    gameplay_black = (
        "        case WM_ATTRACT_SHOW_GAMEPLAY:\n"
        "            /* The existing combat renderer is a development harness only.\n"
        "               Normal product rendering can never present it as start_match. */\n"
        "            fill_rect(0, 0, 320, 240, RGBA32(0, 0, 0, 255));\n"
        "            break;\n"
    )
    gameplay_live = (
        "        case WM_ATTRACT_SHOW_GAMEPLAY:\n"
        "            /* Fix39: normal attract SHOW_GAMEPLAY presentation. */\n"
        "            render_match(app);\n"
        "            break;\n"
    )
    if gameplay_live not in text and gameplay_black in text:
        text = text.replace(gameplay_black, gameplay_live, 1)

    # Combat2i: show live COLLIS gate telemetry in the temporary match HUD so
    # one hardware run identifies the exact stage that is failing. Minimal
    # patcher fixtures may omit draw_match_hud; the real N64 main contains it.
    if "COLLIS ch:" not in text:
        hud_anchor = "    text_line(8, 44, line);\n"
        if hud_anchor in text:
            hud = hud_anchor + (
                "    {\n"
                "        const WmFix39Status *cs = wm_fix39_status();\n"
                "        if (cs) {\n"
                "            snprintf(line, sizeof(line), \"COLLIS ch:%u ab:%u x:%u y:%u z:%u o:%u r:%u h:%u\",\n"
                "                     (unsigned)cs->combat_checkhit_ticks, (unsigned)cs->combat_attack_boxes_built,\n"
                "                     (unsigned)cs->combat_x_overlap_ticks, (unsigned)cs->combat_y_overlap_ticks,\n"
                "                     (unsigned)cs->combat_z_overlap_ticks, (unsigned)cs->combat_full_overlap_ticks,\n"
                "                     (unsigned)cs->combat_full_overlap_rejected, (unsigned)cs->combat_accepted_hits);\n"
                "            text_line(8, 56, line);\n"
                "            snprintf(line, sizeof(line), \"DEMO VIS:B/B SEL:%u/%u\",\n"
                "                     (unsigned)app->p1_choice, (unsigned)app->p2_choice);\n"
                "            text_line(8, 68, line);\n"
                "        }\n"
                "    }\n"
            )
            text = text.replace(hud_anchor, hud, 1)

    path.write_text(text)

def patch_app(path: Path) -> None:
    text = path.read_text()

    if '#include "wm_fix39_runtime.h"' not in text:
        inc = '#include "wm/app.h"\n'
        if inc not in text:
            fail("could not find wm/app.h include in app.c")
        insert = '#include "wm/app.h"\n#include "wm_fix39_runtime.h"\n'
        text = text.replace(inc, insert, 1)
    if "#include <stdint.h>" not in text:
        runtime_inc = '#include "wm_fix39_runtime.h"\n'
        text = text.replace(runtime_inc, runtime_inc + "#include <stdint.h>\n", 1)
    if '#include "wm_arcade_wimp_frame.h"' not in text:
        runtime_inc = '#include "wm_fix39_runtime.h"\n'
        text = text.replace(runtime_inc, runtime_inc + '#include "wm_arcade_wimp_frame.h"\n#include "wm/bret_sprites.h"\n#include "wm/visual.h"\n', 1)

    rng_repl = '''static uint32_t title_rndrng0(wm_app *app, const wm_process *proc,
                               uint32_t maximum) {
    /* Exact shared RNDRNG0 translation. The source scheduler phase supplies
       HCOUNT; a live stack address supplies the N64 SP entropy input. */
    uint32_t hcount = (app->scheduler.tick * 8u) & 0x1ffu;
    uint32_t sp_value = (uint32_t)(uintptr_t)&maximum;
    (void)proc;
    wm_fix39_rng_set_entropy(hcount, sp_value);
    return wm_fix39_rndrng0(maximum);
}
'''
    # v5 keeps the structural function replacement introduced in v4 and avoids
    # regex-match its old provisional implementation byte-for-byte.
    text = replace_function(text, "title_rndrng0", rng_repl)

    # The old helper only existed for the provisional title RNG. Remove it if
    # it remains and no call sites use it after the RNDRNG0 replacement.
    if "title_rotl32" in text:
        start, end = function_span(text, "title_rotl32")
        outside = text[:start] + text[end:]
        if "title_rotl32(" not in outside:
            text = outside

    if "wm_fix39_runtime_init();" not in text:
        start, end = function_span(text, "wm_app_init")
        fn = text[start:end]
        anchor = "    memset(app, 0, sizeof(*app));\n"
        if anchor not in fn:
            fail("could not find memset anchor in wm_app_init")
        fn = fn.replace(anchor, anchor + "    wm_fix39_runtime_init();\n", 1)
        text = text[:start] + fn + text[end:]

    # Make the translated ZIP attract cycle the owner of top-level screen
    # order. Existing N64 renderers still execute only source routines whose
    # platform adapters are actually present; no substitute screens are made.
    attract_begin = '''static bool fix39_attract_frontend_call(const WmAttractStep *step,
                                          wm_attract_call *out) {
    if (!step || !out) return false;
    switch (step->screen) {
        case WM_FIX39_ATTRACT_HISCORES: *out = WM_ATTRACT_SHOW_HSTD; return true;
        case WM_FIX39_ATTRACT_DCS_LOGO: *out = WM_ATTRACT_DCS_LOGO; return true;
        case WM_FIX39_ATTRACT_SPORTS_LOGO: *out = WM_ATTRACT_SHOW_SPORTS_LOGO; return true;
        case WM_FIX39_ATTRACT_GAMEPLAY_DEMO_1:
        case WM_FIX39_ATTRACT_GAMEPLAY_DEMO_2: *out = WM_ATTRACT_SHOW_GAMEPLAY; return true;
        case WM_FIX39_ATTRACT_CREDITS_1:
        case WM_FIX39_ATTRACT_CREDITS_2:
        case WM_FIX39_ATTRACT_EVEN_LOOP_CREDITS: *out = WM_ATTRACT_CREDITSCREEN; return true;
        case WM_FIX39_ATTRACT_TITLE: *out = WM_ATTRACT_SHOW_TITLE; return true;
        case WM_FIX39_ATTRACT_DESIGNER_HINT: *out = WM_ATTRACT_DO_HINTS; return true;
        case WM_FIX39_ATTRACT_GENERAL_TIPS: *out = WM_ATTRACT_SHOW_GEN_TIPS; return true;
        case WM_FIX39_ATTRACT_BIO: *out = WM_ATTRACT_SHOW_BIOS; return true;
        case WM_FIX39_ATTRACT_BIO_TIPS: *out = WM_ATTRACT_SHOW_BIOS_TIPS; return true;
        case WM_FIX39_ATTRACT_OPERATOR_MESSAGE: *out = WM_ATTRACT_SHOW_OPERATORMSG; return true;
        case WM_FIX39_ATTRACT_TIME_DATE: *out = WM_ATTRACT_SHOW_TIME_DATE; return true;
        case WM_FIX39_ATTRACT_COPYRIGHT: *out = WM_ATTRACT_SHOW_COPYRIGHT; return true;
        case WM_FIX39_ATTRACT_AAMA: *out = WM_ATTRACT_AAMA_MESSAGE; return true;
    }
    return false;
}
static bool begin_fix39_attract_step(wm_app *app, size_t index) {
    const WmAttractStep *step = wm_fix39_attract_step(index);
    wm_attract_call call;
    if (!step || !fix39_attract_frontend_call(step, &call)) return false;
    app->attract.source_index = (uint8_t)index;
    app->attract.amode_loops = step->source_amode_loops;
    begin_call(app, call);
    /* V11 source ownership: this begins the translated screen runner only
       when the platform has explicitly bound its exact presentation. */
    (void)wm_fix39_attract_screen_begin(index);
    return true;
}
static void begin_base_loop(wm_app *app) {
    app->attract.flow = WM_ATTRACT_FLOW_BASE;
    if (wm_fix39_attract_cycle_begin() == 0u) return;
    (void)begin_fix39_attract_step(app, 0u);
}
'''
    if "fix39_attract_frontend_call(" not in text:
        text = replace_function(text, "begin_base_loop", attract_begin)

    # V8 removes the parallel hand-written conditional tail. The ZIP cycle
    # already carries AMODE_LOOPS, even-credit/time-date, copyright and AAMA.
    try:
        fs, fe = function_span(text, "finish_base_loop")
        text = text[:fs] + text[fe:]
    except SystemExit:
        pass

    attract_advance = '''static void advance_call(wm_app *app) {
    wm_attract_state *a = &app->attract;
    size_t next_index;
    if (a->call == WM_ATTRACT_DCS_LOGO) {
        /* ATTR.ASM DCS screen stop/reset boundary. */
        (void)wm_audio_send_command(&app->audio, 0);
    }
    kill_call_processes(app, a->call);
    next_index = (size_t)a->source_index + 1u;
    if (wm_fix39_attract_step(next_index) != NULL) {
        (void)begin_fix39_attract_step(app, next_index);
        return;
    }
    begin_base_loop(app);
}
'''
    text = replace_function(text, "advance_call", attract_advance)

    # V11 removes the old frontend-port-status gate. ATTRACT.ASM/Fix39 is now
    # the sole owner of whether a source step can execute. Newly translated
    # screens opt in only after their exact N64 presentation adapter is bound.
    if "skip_fix39_pending_calls(" not in text:
        attract_skip = '''static void skip_fix39_pending_calls(wm_app *app) {
    for (unsigned guard = 0; guard < 64u; ++guard) {
        size_t index = (size_t)app->attract.source_index;
        if (wm_fix39_attract_step_runnable(index)) return;
        wm_fix39_attract_note_pending_skip(index);
        advance_call(app);
    }
}
'''
        text = replace_function(text, "skip_untranslated_calls", attract_skip)
    text = text.replace("skip_untranslated_calls(app);",
                        "skip_fix39_pending_calls(app);")

    # Attract demo activation: SHOW_GAMEPLAY runs the live Fix39 match runtime
    # in CPU-vs-CPU mode and uses the existing N64 source-art presenter.
    if "fix39_tick_gameplay_demo(" not in text:
        marker = "static bool tick_title(wm_app *app, const wm_input_state *input) {"
        pos = text.find(marker)
        if pos < 0:
            fail("could not find tick_title anchor for attract gameplay demo")
        helper = 'static bool fix39_tick_gameplay_demo(wm_app *app, const wm_input_state *input) {\n    wm_attract_state *a = &app->attract;\n    ++a->call_ticks;\n    if (a->call_ticks == 1u) {\n        /* ATTR.ASM SHOW_GAMEPLAY enters the real match path. No wm_demo simulation. */\n        wm_fix39_match_begin((unsigned)app->p1_choice, (unsigned)app->p2_choice);\n        wm_fix39_match_set_cpu_vs_cpu(true);\n    }\n    wm_fix39_match_tick(0, 0, false, false, false, false, false, false);\n    if (a->call_ticks > 60u && wm_app_any_attract_button(input)) { wm_fix39_match_set_cpu_vs_cpu(false); return true; }\n    if (a->call_ticks >= 600u) { wm_fix39_match_set_cpu_vs_cpu(false); return true; }\n    return false;\n}\n'
        text = text[:pos] + helper + text[pos:]

    # Run V11 live ATTRACT control/timing for source-owned calls. Pending
    # screens have already been skipped above, so this default cannot park on
    # an unimplemented state. DCS/Sports/Title retain their existing exact
    # frontend tickers.
    text = replace_attract_call_switch(text)
    old_switch = "case WM_ATTRACT_SHOW_TITLE: done = tick_title(app, input); break;"
    new_switch = old_switch + "\n        case WM_ATTRACT_SHOW_GAMEPLAY: done = fix39_tick_gameplay_demo(app, input); break;"
    if new_switch not in text:
        if old_switch not in text:
            fail("could not find attract call switch SHOW_TITLE case")
        text = text.replace(old_switch, new_switch, 1)

    if "wm_fix39_match_tick(" not in text:
        # Patch the mode branch in the complete translation unit instead of
        # first slicing wm_app_tick.  Earlier patchers could select the wrong
        # function span when C formatting changed.  There should be one live
        # MATCH_INIT if-branch; assignments to MATCH_INIT are ignored.
        pat = re.compile(
            r"if[ \t\r\n]*\([ \t\r\n]*app->mode[ \t\r\n]*==[ \t\r\n]*"
            r"WM_APP_MODE_MATCH_INIT[ \t\r\n]*\)[ \t\r\n]*\{"
        )
        matches = list(pat.finditer(text))
        if len(matches) != 1:
            # Diagnostic intentionally reports all nearby MATCH_INIT lines so
            # a future repo drift is obvious instead of another blind retry.
            nearby = []
            for ln, line in enumerate(text.splitlines(), 1):
                if "MATCH_INIT" in line:
                    nearby.append(f"{ln}:{line.strip()}")
            fail("expected one live MATCH_INIT if-branch, found " +
                 str(len(matches)) + "; occurrences=" + " | ".join(nearby))
        m = matches[0]
        open_pos = text.find("{", m.start(), m.end() + 1)
        if open_pos < 0:
            fail("MATCH_INIT branch matched without an opening brace")
        close_pos = find_matching_brace(text, open_pos)
        replacement = '''if (app->mode == WM_APP_MODE_MATCH_INIT) {
        if (!wm_fix39_match_started()) {
            uint32_t hs_remaining = 0u;
            wm_fix39_match_begin((unsigned)app->p1_choice,
                                 (unsigned)app->p2_choice);
            /* No guessed operator adjustment: this is a no-op until bound. */
            (void)wm_fix39_hiscore_player_start_or_continue(&hs_remaining);
        }
        wm_fix39_match_tick(input ? input->stick_x : 0,
                            input ? input->stick_y : 0,
                            input ? input->run : false,
                            input ? input->light_punch : false,
                            input ? input->power_punch : false,
                            input ? input->light_kick : false,
                            input ? input->power_kick : false,
                            input ? input->block : false);
        return;
    }'''
        text = text[:m.start()] + replacement + text[close_pos + 1:]

    if "wm_fix39_mainloop_step(" not in text:
        start, end = function_span(text, "wm_app_video_frame")
        fn = text[start:end]
        anchor = "    wm_app_tick(app, &tick_input);\n"
        if anchor not in fn:
            fail("could not find wm_app_tick call in wm_app_video_frame")
        hook = anchor + '''    /* WRESTLE.ASM mainpok randomizes RAND after process_dispatch. */
    {
        uint32_t hcount = (app->scheduler.tick * 8u) & 0x1ffu;
        uint32_t sp_value = (uint32_t)(uintptr_t)&tick_input;
        (void)wm_fix39_mainloop_step(hcount, sp_value);
    }
'''
        fn = fn.replace(anchor, hook, 1)
        text = text[:start] + fn + text[end:]

    path.write_text(text)



def patch_headless_main(path: Path) -> None:
    text = path.read_text()
    if '#include "wm_fix39_runtime.h"' not in text:
        app_inc = '#include "wm/app.h"\n'
        if app_inc in text:
            text = text.replace(app_inc, app_inc + '#include "wm_fix39_runtime.h"\n', 1)
        else:
            first_include = text.find('#include ')
            if first_include < 0:
                fail('could not locate headless include anchor')
            line_end = text.find('\n', first_include)
            text = text[:line_end + 1] + '#include "wm_fix39_runtime.h"\n' + text[line_end + 1:]
    # The headless smoke has existed in several revisions.  Patch it
    # structurally instead of depending on one exact historical comment/body.
    # This keeps Combat2 applicable to the current main branch and to branches
    # already carrying an earlier attract-demo update.
    live_block = '''    while (app.attract.call == WM_ATTRACT_SHOW_SPORTS_LOGO && guard++ < 4000)
        wm_app_tick(&app, &button);
    if (app.attract.call != WM_ATTRACT_SHOW_GAMEPLAY) {
        fprintf(stderr, "HEADLESS_ATTRACT_FAIL[3]: expected first SHOW_GAMEPLAY, call=%d guard=%u\\n", (int)app.attract.call, guard);
        return 3;
    }

    while (app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY && guard++ < 6000)
        wm_app_tick(&app, &button);
    if (app.attract.call != WM_ATTRACT_SHOW_TITLE) {
        fprintf(stderr, "HEADLESS_ATTRACT_FAIL[4]: first demo did not return to TITLE, call=%d guard=%u\\n", (int)app.attract.call, guard);
        return 4;
    }

    while (app.attract.call == WM_ATTRACT_SHOW_TITLE && guard++ < 8000)
        wm_app_tick(&app, &button);
    if (app.attract.call != WM_ATTRACT_SHOW_GAMEPLAY) {
        fprintf(stderr, "HEADLESS_ATTRACT_FAIL[5]: expected second SHOW_GAMEPLAY, call=%d guard=%u\\n", (int)app.attract.call, guard);
        return 5;
    }

    while (app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY && guard++ < 10000)
        wm_app_tick(&app, &button);
    if (app.attract.call != WM_ATTRACT_DCS_LOGO) {
        fprintf(stderr, "HEADLESS_ATTRACT_FAIL[6]: second demo did not loop to DCS_LOGO, call=%d guard=%u\\n", (int)app.attract.call, guard);
        return 6;
    }
    if (app.attract.amode_loops != 1) {
        fprintf(stderr, "HEADLESS_ATTRACT_FAIL[7]: amode_loops=%u expected=1\\n", app.attract.amode_loops);
        return 7;
    }
    /* SHOW_GAMEPLAY is an existing-frontend-owned source step. The legacy
       wm_attract_call_is_translated() table does not own it; V13e owner/runnable
       state is authoritative. */
    {
        WmAttractOwner owner = wm_fix39_attract_step_owner(2u);
        if (owner != WM_ATTRACT_OWNER_EXISTING_FRONTEND ||
            !wm_fix39_attract_step_runnable(2u)) {
            fprintf(stderr, "HEADLESS_ATTRACT_FAIL[8]: first gameplay step not runnable, owner=%d\\n", (int)owner);
            return 8;
        }
    }
'''

    sports_anchor = '    while (app.attract.call == WM_ATTRACT_SHOW_SPORTS_LOGO && guard++ < 4000)\n'
    print_anchor = '    printf("wm_arcade_port r9\\n");\n'
    if sports_anchor not in text or print_anchor not in text:
        fail('could not locate headless attract smoke anchors')
    a = text.index(sports_anchor)
    b = text.index(print_anchor, a)
    current = text[a:b]
    # Replace the whole attract assertion segment.  If it is already the live
    # form this is harmless/idempotent and avoids stale-text failures.
    text = text[:a] + live_block + text[b:]
    text = text.replace('frontend rule: harness-only code excluded from normal arcade execution',
                        'frontend rule: source gameplay demos execute in normal arcade attract')
    path.write_text(text)

def patch_character_presenter(repo: Path) -> None:
    """Combat2L: replace Bret-only attract presentation with roster-aware source art."""
    dh = repo / "include/wm/demo.h"
    text = dh.read_text()
    if "uint8_t roster_id;" not in text:
        # Upstream declares wm_demo_fighter with an anonymous typedef:
        #     typedef struct { ... } wm_demo_fighter;
        # Older Combat2L expected a tagged struct and therefore failed before
        # touching the fresh checkout. Anchor on the fighter's first field
        # instead; this matches the actual known-good repository layout while
        # remaining specific to wm_demo_fighter.
        anchor = "typedef struct {\n    wm_visual_state visual;\n"
        if anchor not in text:
            fail("Combat2P: wm_demo_fighter anonymous typedef anchor missing")
        text = text.replace(anchor, "typedef struct {\n    uint8_t roster_id;\n    wm_visual_state visual;\n", 1)
    if "wm_demo_set_roster" not in text:
        anchor = "void wm_demo_reset_match(wm_demo *d);\n"
        if anchor not in text: fail("Combat2L: demo reset declaration missing")
        text = text.replace(anchor, anchor + "void wm_demo_set_roster(wm_demo *d, uint8_t p1, uint8_t p2);\n", 1)
    dh.write_text(text)

    dc = repo / "src/core/demo.c"
    text = dc.read_text().replace('#include "wm/bret_visuals.h"\n', '#include "wm/character_assets.h"\n')
    start, end = function_span(text, "sequence_for")
    repl = '''static const wm_visual_sequence *sequence_for(const wm_demo_fighter *f) {
    wm_character_visual_slot slot = WM_CV_STAND2;
    switch (f->action) {
        case WM_DEMO_LIGHT_PUNCH: slot = horizontal_facing(f->facing) ? WM_CV_LP4 : WM_CV_LP2; break;
        case WM_DEMO_POWER_PUNCH: slot = WM_CV_PP; break;
        case WM_DEMO_LIGHT_KICK: slot = horizontal_facing(f->facing) ? WM_CV_LK4 : WM_CV_LK2; break;
        case WM_DEMO_POWER_KICK: slot = WM_CV_PK; break;
        case WM_DEMO_RUN: slot = WM_CV_RUN; break;
        case WM_DEMO_WALK:
            switch (f->facing) {
                case WM_DEMO_FACING_2: slot = WM_CV_WALK2; break;
                case WM_DEMO_FACING_8: slot = WM_CV_WALK8; break;
                case WM_DEMO_FACING_4: slot = WM_CV_WALK4; break;
                case WM_DEMO_FACING_6: slot = WM_CV_WALK6; break;
            }
            break;
        case WM_DEMO_BLOCK:
        case WM_DEMO_IDLE:
        default: slot = horizontal_facing(f->facing) ? WM_CV_STAND4 : WM_CV_STAND2; break;
    }
    return wm_character_visual(f->roster_id, slot);
}
'''
    text = text[:start] + repl + text[end:]
    start, end = function_span(text, "torso_sequence_for")
    repl = '''static const wm_visual_sequence *torso_sequence_for(const wm_demo_fighter *f) {
    return wm_character_visual(f->roster_id, horizontal_facing(f->facing) ? WM_CV_TORSO4 : WM_CV_TORSO2);
}
'''
    text = text[:start] + repl + text[end:]
    old = "void wm_demo_reset_match(wm_demo *d) {\n    memset(&d->p1, 0, sizeof(d->p1));\n    memset(&d->p2, 0, sizeof(d->p2));\n"
    new = "void wm_demo_reset_match(wm_demo *d) {\n    uint8_t p1_roster = d->p1.roster_id;\n    uint8_t p2_roster = d->p2.roster_id;\n    memset(&d->p1, 0, sizeof(d->p1));\n    memset(&d->p2, 0, sizeof(d->p2));\n    d->p1.roster_id = p1_roster;\n    d->p2.roster_id = p2_roster;\n"
    if old in text: text = text.replace(old, new, 1)
    if "void wm_demo_set_roster" not in text:
        anchor = "void wm_demo_init(wm_demo *d) {\n"
        setter = '''void wm_demo_set_roster(wm_demo *d, uint8_t p1, uint8_t p2) {
    if (!d) return;
    d->p1.roster_id = p1;
    d->p2.roster_id = p2;
    set_action(&d->p1, d->p1.action);
    set_action(&d->p2, d->p2.action);
}
'''
        if anchor not in text: fail("Combat2L: demo init anchor missing")
        text = text.replace(anchor, setter + anchor, 1)
    dc.write_text(text)

    nc = repo / "src/platform/n64/main.c"
    text = nc.read_text()
    if '#include "wm/character_assets.h"' not in text:
        text = text.replace('#include "wm/bret_sprites.h"\n', '#include "wm/bret_sprites.h"\n#include "wm/character_assets.h"\n', 1)
    text = text.replace('return frame ? wm_bret_sprite_find(frame->source_frame) : NULL;', 'return frame ? wm_character_sprite_find(f->roster_id, frame->source_frame) : NULL;')
    text = text.replace('const wm_source_sprite *object_pal = bret_object_palette(spr);', '/* Combat2DT: each source frame owns its exact WIMP palette.  Using the wrestler base-frame TLUT here can reinterpret a valid non-Bret CI8 frame with unrelated colors. */\n    const wm_source_sprite *object_pal = spr;\n    if (!object_pal || !object_pal->palette_rgba5551) object_pal = wm_character_base_sprite(f->roster_id);')
    text = text.replace('? wm_bret_sprite_find(torso_frame->source_frame) : NULL;', '? wm_character_sprite_find(f->roster_id, torso_frame->source_frame) : NULL;')
    text = text.replace('        text_line(8, 214, "SOURCE SLOT SELECTED - BRET ART BACKEND USED AS PLACEHOLDER");', '        text_line(8, 214, "SOURCE CHARACTER ART BACKEND ACTIVE");')
    nc.write_text(text)

    ac = repo / "src/core/app.c"
    text = ac.read_text()
    if '#include "wm/character_assets.h"' not in text:
        text = text.replace('#include "wm_arcade_wimp_frame.h"\n', '#include "wm_arcade_wimp_frame.h"\n#include "wm/character_assets.h"\n', 1)
    text = text.replace('const wm_source_sprite *spr = (vf[i] && vf[i]->source_frame) ? wm_bret_sprite_find(vf[i]->source_frame) : NULL;', 'const uint8_t rid = i ? app->demo.p2.roster_id : app->demo.p1.roster_id;\n        const wm_source_sprite *spr = (vf[i] && vf[i]->source_frame) ? wm_character_sprite_find(rid, vf[i]->source_frame) : NULL;')
    text = text.replace('wm_fix39_match_bind_bret_source_frame_attack(0, af0 ? af0->source_frame : NULL);\n        wm_fix39_match_bind_bret_source_frame_attack(1, af1 ? af1->source_frame : NULL);', 'wm_fix39_match_bind_source_frame_attack(0, app->demo.p1.roster_id, af0 ? af0->source_frame : NULL);\n        wm_fix39_match_bind_source_frame_attack(1, app->demo.p2.roster_id, af1 ? af1->source_frame : NULL);')
    ac.write_text(text)

    cm = repo / "CMakeLists.txt"
    text = cm.read_text()
    if "src/generated/character_assets.c" not in text:
        anchor = "    src/generated/bret_sprites.c\n"
        if anchor not in text: fail("Combat2L: CMake bret sprite anchor missing")
        text = text.replace(anchor, anchor + "    src/generated/character_assets.c\n", 1)
    cm.write_text(text)
    mk = repo / "Makefile"
    text = mk.read_text()
    if "src/generated/character_assets.c" not in text:
        text = text.replace("src/generated/bret_sprites.c", "src/generated/bret_sprites.c src/generated/character_assets.c", 1)
    if "FIX39_CHAR_DFS_FILES" not in text:
        dfs_anchor = "$(ROMNAME).z64: $(BUILD_DIR)/$(ROMNAME).dfs\n"
        dfs_block = "# BEGIN FIX39 STREAMED CHARACTER ART\nFIX39_CHAR_DFS_FILES := $(wildcard filesystem/fix39_chars/*/*.bin)\n$(BUILD_DIR)/$(ROMNAME).dfs: $(FIX39_CHAR_DFS_FILES)\n$(ROMNAME).z64: $(BUILD_DIR)/$(ROMNAME).dfs\n# END FIX39 STREAMED CHARACTER ART\n"
        if dfs_anchor in text:
            text = text.replace(dfs_anchor, dfs_block, 1)
        else:
            text += "\n" + dfs_block
    mk.write_text(text)

    wf = repo / ".github/workflows/build.yml"
    text = wf.read_text()
    marker = "# FIX39 COMBAT2L CHARACTER ASSETS"
    anchor = "      - name: Configure portable verifier\n        run: cmake -S . -B build-host -DCMAKE_BUILD_TYPE=Release\n"
    if marker not in text and anchor in text:
        step = (
            "      - name: Generate all wrestler source art and attack frames\n"
            "        # FIX39 COMBAT2L CHARACTER ASSETS\n"
            "        run: |\n"
            "          set -euo pipefail\n"
            "          test -d original/wwf-wrestlemania || sh ./scripts/fetch_original.sh\n"
            "          python3 tools/fix39_character_assets.py --root original/wwf-wrestlemania --out-c src/generated/character_assets.c --out-h include/wm/character_assets.h --out-fs filesystem/fix39_chars\n"
            "          python3 tools/fix39_character_attack_frames.py --root original/wwf-wrestlemania --out src/fix39/wm_arcade_character_attack_frames_generated.h\n"
            "          grep -q 'WM_FIX39_CHARACTER_ATTACK_FRAMES_GENERATED 1' src/fix39/wm_arcade_character_attack_frames_generated.h\n"
            "          test -s src/generated/character_assets.c\n"
        )
        text = text.replace(anchor, step + anchor, 1)
    # Combat2AL: N64 job must regenerate the streamed DragonFS character payloads too.
    n64_anchor = "          sh ./scripts/prepare_bret_sprites.sh\n          sh ./scripts/prepare_frontend_assets.sh\n"
    n64_extra = (
        "          python3 tools/fix39_character_assets.py --root original/wwf-wrestlemania --out-c src/generated/character_assets.c --out-h include/wm/character_assets.h --out-fs filesystem/fix39_chars\n"
        "          python3 tools/fix39_character_attack_frames.py --root original/wwf-wrestlemania --out src/fix39/wm_arcade_character_attack_frames_generated.h\n"
        "          test -n \"$(find filesystem/fix39_chars -type f -name '*.bin' -print -quit)\"\n"
    )
    # Prefer the N64 regeneration block; replacing the last occurrence avoids the host/source-conversion step.
    pos = text.rfind(n64_anchor)
    if pos >= 0:
        insert_at = pos + len(n64_anchor)
        if "fix39_character_assets.py" not in text[insert_at:insert_at + 1200]:
            text = text[:insert_at] + n64_extra + text[insert_at:]
    wf.write_text(text)

def main() -> None:
    parser = argparse.ArgumentParser(description="Apply source-direct Fix39 merge")
    parser.add_argument("repo", nargs="?", default=".")
    args = parser.parse_args()

    repo = Path(args.repo).resolve()
    bundle = Path(__file__).resolve().parents[1]
    src_bundle = bundle / "src" / "fix39"
    test_bundle = bundle / "tests" / "fix39_smoke.c"
    chunk6_test_bundle = bundle / "tests" / "test_v13e_chunk6.py"
    attract_text_tool = bundle / "tools" / "fix39_attract_text.py"
    attract_asset_tool = bundle / "tools" / "fix39_attract_assets.py"
    drone_table_tool = bundle / "tools" / "fix39_drone_tables.py"
    drone_range_tool = bundle / "tools" / "fix39_drone_ranges.py"
    drone_script_tool = bundle / "tools" / "fix39_drone_scripts.py"
    drone_services_tool = bundle / "tools" / "fix39_drone_services.py"
    drone_bodies_tool = bundle / "tools" / "fix39_drone_bodies.py"
    drone_translate_tool = bundle / "tools" / "fix39_drone_translate.py"
    bret_attack_tool = bundle / "tools" / "fix39_bret_attack_frames.py"
    character_asset_tool = bundle / "tools" / "fix39_character_assets.py"
    character_attack_tool = bundle / "tools" / "fix39_character_attack_frames.py"
    attract_renderer_patch = bundle / "patches" / "fix39_v12_n64_attract.inc"
    sports_override_bundle = bundle / "assets" / "fix39_sports_override"
    sports_background_tool = bundle / "tools" / "fix39_sports_background_bundle.py"
    build_graph_audit_tool = bundle / "tools" / "fix39_build_graph_audit.py"

    for required in (repo / "CMakeLists.txt", repo / "Makefile", repo / "src/core/app.c",
                     repo / "src/platform/n64/main.c", repo / "src/platform/headless/main.c",
                     repo / ".github/workflows/build.yml"):
        if not required.is_file():
            fail(f"not the expected wm-arcade repo; missing {required}")

    # Project-specific Sports repurpose. These are already round-tripped source
    # files, not flattened N64 substitutes. Keep them tracked in the branch so
    # both host/source-conversion CI and the libdragon ROM build consume the
    # exact same WIMP inputs.
    sports_required = ("SPORTLO8.IMG", "SPORTBK.IMG", "SPRTBK.BDD", "SPRTBK.BDB")
    for name in sports_required:
        if not (sports_override_bundle / name).is_file():
            fail(f"Sports override asset missing from integrator: {name}")
    sports_dest = repo / "assets" / "fix39_sports_override"
    sports_dest.mkdir(parents=True, exist_ok=True)
    for name in sports_required:
        shutil.copy2(sports_override_bundle / name, sports_dest / name)
    for extra in ("README.txt", "SHA256SUMS.txt"):
        src = sports_override_bundle / extra
        if src.is_file():
            shutil.copy2(src, sports_dest / extra)

    dest = repo / "src" / "fix39"
    dest.mkdir(parents=True, exist_ok=True)
    for p in src_bundle.iterdir():
        if p.is_file() and p.suffix in (".c", ".h"):
            shutil.copy2(p, dest / p.name)

    # V11 hard guard: the repo must never compile a stale pre-V9 direct-port
    # rope API.  C enumerators have translation-unit scope, so these old names
    # collide with include/wm/ropes.h as soon as app.c includes both APIs.
    stale_rope = re.compile(r"\bWM_ROPE_(?:FRONT|BACK|LEFT|RIGHT|TOP|MIDDLE|BOTTOM|Z_HIGH|Z_NORM|BOUNCE_UD|BOUNCE_IO|SIDE_SPRING|DOWN_SPRING|SIDE_SPRING_RELEASE|DOWN_SPRING_RELEASE|COMMAND_COUNT|CHANNEL_RED|CHANNEL_WHITE|CHANNEL_BLUE|CHANNEL_SHADOW|CHANNEL_COUNT|HALF_FIRST|HALF_SECOND)\b")
    stale_hits = []
    for copied in sorted(dest.glob("*.[ch]")):
        m = stale_rope.search(copied.read_text())
        if m:
            stale_hits.append(f"{copied.name}:{m.group(0)}")
    if stale_hits:
        fail("stale pre-V9 rope namespace after copy: " + ", ".join(stale_hits))

    rope_header = (dest / "wmania_rope_command.h").read_text()
    if "WM_FIX39_ROPE_COMMAND_COUNT = 6" not in rope_header:
        fail("namespaced V11 rope sentinel missing after copy")

    (repo / "tests").mkdir(exist_ok=True)
    shutil.copy2(test_bundle, repo / "tests" / "fix39_smoke.c")
    if not chunk6_test_bundle.is_file():
        fail("V13e-c6 activation regression missing from bundle")
    shutil.copy2(chunk6_test_bundle, repo / "tests" / "test_v13e_chunk6.py")
    (repo / "tools").mkdir(exist_ok=True)
    if not attract_text_tool.is_file():
        fail("V12 ATTRACT source-text generator missing from bundle")
    if not attract_asset_tool.is_file():
        fail("V12 ATTRACT source-asset generator missing from bundle")
    if not drone_table_tool.is_file():
        fail("V13e DRONE source-table generator missing from bundle")
    if not drone_range_tool.is_file():
        fail("V13e DRONE source-range generator missing from bundle")
    if not drone_script_tool.is_file():
        fail("V13e DRONE source-script generator missing from bundle")
    if not attract_renderer_patch.is_file():
        fail("V12 N64 ATTRACT renderer snippet missing from bundle")
    shutil.copy2(attract_text_tool, repo / "tools" / "fix39_attract_text.py")
    shutil.copy2(attract_asset_tool, repo / "tools" / "fix39_attract_assets.py")
    shutil.copy2(drone_table_tool, repo / "tools" / "fix39_drone_tables.py")
    shutil.copy2(drone_range_tool, repo / "tools" / "fix39_drone_ranges.py")
    shutil.copy2(drone_script_tool, repo / "tools" / "fix39_drone_scripts.py")
    shutil.copy2(drone_services_tool, repo / "tools" / "fix39_drone_services.py")
    shutil.copy2(drone_bodies_tool, repo / "tools" / "fix39_drone_bodies.py")
    shutil.copy2(drone_translate_tool, repo / "tools" / "fix39_drone_translate.py")
    if not bret_attack_tool.is_file():
        fail("Demo5 Bret attack-frame source generator missing from bundle")
    shutil.copy2(bret_attack_tool, repo / "tools" / "fix39_bret_attack_frames.py")
    if not character_asset_tool.is_file(): fail("Combat2L character asset generator missing")
    shutil.copy2(character_asset_tool, repo / "tools" / "fix39_character_assets.py")
    if not character_attack_tool.is_file(): fail("Combat2L character attack generator missing")
    shutil.copy2(character_attack_tool, repo / "tools" / "fix39_character_attack_frames.py")
    if not sports_background_tool.is_file():
        fail("Fix39 Sports background override converter missing from bundle")
    shutil.copy2(sports_background_tool, repo / "tools" / "fix39_sports_background_bundle.py")
    if not build_graph_audit_tool.is_file():
        fail("Combat2DM build-graph audit tool missing from bundle")
    shutil.copy2(build_graph_audit_tool, repo / "tools" / "fix39_build_graph_audit.py")

    patch_public_attract_data_abi(repo / "include/wm/arcade/wmania_attract_data.h")

    sources = sorted(p.name for p in dest.glob("*.c"))
    if "wm_fix39_runtime.c" not in sources:
        fail("runtime wrapper did not copy")

    patch_cmake(repo / "CMakeLists.txt", sources)
    patch_github_workflow(repo / ".github/workflows/build.yml")
    make_sources, deduped = patch_makefile(repo / "Makefile", sources)
    patch_app(repo / "src/core/app.c")
    patch_character_presenter(repo)
    patch_frontend_assets_script(repo / "scripts/prepare_frontend_assets.sh")
    patch_sports_source_assets_script(repo / "scripts/prepare_sports_source_assets.sh")
    if "# FIX39 SPORTS FOREGROUND OVERRIDE" not in (repo / "scripts/prepare_frontend_assets.sh").read_text():
        fail("actual repo prepare_frontend_assets.sh did not accept SPORTLO8 override")
    sports_script_text = (repo / "scripts/prepare_sports_source_assets.sh").read_text()
    if "# FIX39 SPORTS BACKGROUND OVERRIDE" not in sports_script_text:
        fail("actual repo prepare_sports_source_assets.sh did not accept SPORTBK override")
    if 'SPORTS_BG_TOOL="$ROOT/tools/fix39_sports_background_bundle.py"' not in sports_script_text:
        fail("actual repo Sports override did not bind the WIMP-palette converter")
    patch_n64_main(repo / "src/platform/n64/main.c")
    patch_headless_main(repo / "src/platform/headless/main.c")
    patch_core_tests(repo / "tests/test_core.c")

    print(f"Fix39 applied: {len(sources)} C modules available to host verification")
    print(f"N64 Makefile: {len(make_sources)} authoritative Fix39 module(s); retired {len(deduped)} overlapping src/core/arcade module(s)")
    if deduped:
        print("N64 stale baseline owners retired: " + ", ".join(deduped))

    # V13e-c5c generated executable-service binding registry.
    for _n in ("wm_arcade_drone_source_services.c", "wm_arcade_drone_source_services.h", "wm_arcade_drone_source_services_generated.h"):
        shutil.copy2(bundle / "src" / "fix39" / _n, repo / "src" / "fix39" / _n)
    print("Patched: CMakeLists.txt, .github/workflows/build.yml, Makefile, src/core/app.c, src/platform/n64/main.c, src/platform/headless/main.c, scripts/prepare_frontend_assets.sh, scripts/prepare_sports_source_assets.sh, assets/fix39_sports_override, tests/test_core.c, tests/fix39_smoke.c, tests/test_v13e_chunk6.py, tools/fix39_attract_text.py, tools/fix39_attract_assets.py, tools/fix39_drone_tables.py, tools/fix39_drone_ranges.py, tools/fix39_drone_scripts.py, tools/fix39_drone_services.py, tools/fix39_sports_background_bundle.py")


if __name__ == "__main__":
    main()
