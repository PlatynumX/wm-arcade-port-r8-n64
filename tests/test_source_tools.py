#!/usr/bin/env python3
from __future__ import annotations
import importlib.util
import pathlib
import struct
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]


def load(name: str, path: pathlib.Path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    assert spec and spec.loader
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module

wlanim = load("wlanim", ROOT / "tools" / "wlanim.py")
manifest = load("bret_manifest", ROOT / "tools" / "bret_manifest.py")
wimp = load("wimpimg", ROOT / "tools" / "wimpimg.py")
bundle = load("bret_bundle", ROOT / "tools" / "bret_bundle.py")
frontend_bundle = load("frontend_bundle", ROOT / "tools" / "frontend_bundle.py")
sparkle_bundle = load("sparkle_bundle", ROOT / "tools" / "sparkle_bundle.py")
dcs_bundle = load("dcs_bundle", ROOT / "tools" / "dcs_bundle.py")
inventory_mod = load("source_inventory", ROOT / "tools" / "source_inventory.py")
attract_sequence = load("attract_sequence", ROOT / "tools" / "attract_sequence.py")
port_manifest = load("port_manifest", ROOT / "tools" / "port_manifest.py")
source_text_bundle = load("source_text_bundle", ROOT / "tools" / "source_text_bundle.py")
source_text_verify = load("verify_source_text_bundle", ROOT / "tools" / "verify_source_text_bundle.py")
bdd_bundle = load("bdd_bundle", ROOT / "tools" / "bdd_bundle.py")
bmod_source = load("bmod_source", ROOT / "tools" / "bmod_source.py")
source_ir = load("source_ir", ROOT / "tools" / "source_ir.py")
animation_ir = load("animation_ir", ROOT / "tools" / "animation_ir.py")
select_source = load("select_source", ROOT / "tools" / "select_source.py")


def test_wlanim() -> None:
    p = ROOT / "tests" / "fixtures" / "HRTSEQ1_MIN.ASM"
    stand2 = wlanim.extract(p, "hrt_stand2_anim")
    stand4 = wlanim.extract(p, "hrt_stand4_anim")
    torso2 = wlanim.extract(p, "hrt_torso2_anim")
    torso4 = wlanim.extract(p, "hrt_torso4_anim")
    walk2 = wlanim.extract(p, "hrt_walk2_f2_anim")
    walk8 = wlanim.extract(p, "hrt_walk8_f2_anim")
    walk4 = wlanim.extract(p, "hrt_walk4_f4_anim")
    walk6 = wlanim.extract(p, "hrt_walk6_f4_anim")
    run = wlanim.extract_visual_slice(p, "hrt_run_anim", True)

    assert stand2.repeat and len(stand2.frames) == 14
    assert stand2.frames[0].name == "H2ST2A05" and stand2.frames[3].ticks == 6
    assert stand4.repeat and len(stand4.frames) == 14
    assert stand4.frames[0].name == "H4ST4A02" and stand4.frames[6].ticks == 6
    assert torso2.repeat and [f.name for f in torso2.frames] == [
        "H2TW2A01", "H2TW2A02", "H2TW2A03", "H2TW2A04", "H2TW2A03", "H2TW2A02"
    ]
    assert torso4.repeat and torso4.frames[0].name == "H4TW4A01" and len(torso4.frames) == 6
    assert walk2.repeat and len(walk2.frames) == 16
    assert [f.ticks for f in walk2.frames[0:4]] == [2, 2, 2, 3]
    assert walk8.frames[0].name == "H2WL8A16" and walk8.frames[-1].name == "H2WL8A01"
    assert walk4.frames[-1].name == "H4WL4A16"
    assert len(walk6.frames) == 15 and walk6.frames[0].name == "H4WL2A15"
    assert run.repeat and len(run.frames) == 12
    assert run.frames[0].name == "H3RN3A01" and run.frames[-1].name == "H3RN3A12"

    p2 = ROOT / "tests" / "fixtures" / "HRTSEQ2_MIN.ASM"
    punch2 = wlanim.extract_visual_slice(p2, "hrt_2_punch_anim", False)
    punch4 = wlanim.extract_visual_slice(p2, "hrt_4_punch_anim", False)
    super_punch = wlanim.extract_visual_slice(p2, "hrt_4_super_punch_anim", False)
    kick2 = wlanim.extract_visual_slice(p2, "hrt_2_kick_anim", False)
    kick4 = wlanim.extract_visual_slice(p2, "hrt_4_kick_anim", False)
    super_kick = wlanim.extract_visual_slice(p2, "hrt_2_super_kick_anim", False)
    assert len(punch2.frames) == 11 and punch2.frames[5].name == "H2PL3B04"
    assert len(punch4.frames) == 11 and punch4.frames[-1].name == "H4PL3X08"
    assert len(super_punch.frames) == 10 and super_punch.frames[5].name == "H4UP3C06"
    assert len(kick2.frames) == 12 and kick2.frames[-1].name == "H2KM3A11"
    assert len(kick4.frames) == 12 and kick4.frames[-1].name == "H4KM3B10"
    assert len(super_kick.frames) == 11 and super_kick.frames[4].name == "H4KM3C04"
    assert super_kick.frames[-1].name == "H4KM3C09"


def test_manifest() -> None:
    m = manifest.parse_lod(ROOT / "tests" / "fixtures" / "BRET_MIN.LOD")
    assert m["H4ST4A01"] == "hrt_wlk.img"
    assert m["H2WL1A16"] == "hrt_wlk.img"


def synthetic_wimp(image_name: str = "H4ST4A01", palette_name: str = "HARTPAL") -> bytes:
    # 1 image + 1 palette. Image is 3x2 CI8, padded to a 4-byte row stride.
    # Palette payload at 0x1C; image payload at 0x40; directories start at 0x80.
    assert len(image_name) <= 8 and len(palette_name) <= 8
    data = bytearray(0x100)
    struct.pack_into("<HHIHHHH", data, 0,
                     1, 0, 0x80, 0x063F, 0, 0, 0)
    # transparent black, red, green, blue in Midway RGB555
    struct.pack_into("<HHHH", data, 0x1C, 0x0000, 0x7C00, 0x03E0, 0x001F)
    data[0x40:0x48] = bytes([1, 2, 3, 0xEE, 3, 2, 1, 0xEE])

    off = 0x80
    raw_name = image_name.encode("ascii")
    data[off:off+len(raw_name)] = raw_name
    struct.pack_into("<hhHHHI", data, off + 18,
                     -1, 2, 3, 2, 5, 0x40)
    # 18-byte WIMP tail.  First words are deliberately nonsense to guard
    # against the r6h3 assumption that +32/+34 are runtime attachment coords.
    struct.pack_into("<9h", data, off + 32, 30000, 25000, 0, 7, 11, 2, -1, -1, -1)

    poff = off + wimp.IMAGE_ENTRY_SIZE
    raw_pal = palette_name.encode("ascii")
    data[poff:poff+len(raw_pal)] = raw_pal
    struct.pack_into("<HI", data, poff + 12, 4, 0x1C)
    return bytes(data)


def test_wimp_probe() -> None:
    data = synthetic_wimp()
    h = wimp.parse_header(data)
    entries = wimp.parse_images(data, h)
    pals = wimp.parse_palettes(data, h, entries)
    assert h.image_count == 1 and h.directory_offset == 0x80
    assert entries[0].name == "H4ST4A01"
    assert entries[0].width == 3 and entries[0].height == 2
    assert entries[0].xani == -1 and entries[0].yani == 2
    assert entries[0].tail_words == (30000, 25000, 0, 7, 11, 2, -1, -1, -1)
    assert pals[0].name == "HARTPAL" and pals[0].color_count == 4
    assert wimp.read_palette_words(data, pals[0]) == [0, 0x7C00, 0x03E0, 0x001F]
    assert wimp.read_ci8(data, entries[0]) == bytes([1, 2, 3, 3, 2, 1])
    assert wimp.palette_for_image(entries[0], entries, pals) == pals[0]
    assert wimp.rgb555_to_rgba5551(0x7C00, 1) == 0xF801
    assert wimp.rgb555_to_rgba5551(0x0000, 0) == 0


def test_wimp_emit_c() -> None:
    data = synthetic_wimp()
    h = wimp.parse_header(data)
    entries = wimp.parse_images(data, h)
    pals = wimp.parse_palettes(data, h, entries)
    with tempfile.TemporaryDirectory() as td:
        out = pathlib.Path(td) / "bret_sprites.c"
        wimp.emit_c(out, data, entries, pals, ["H4ST4A01"], "hrt_wlk.img")
        text = out.read_text()
        assert '"H4ST4A01", "hrt_wlk.img", 3, 2, -1, 2, {30000, 25000, 0, 7, 11, 2, -1, -1, -1}' in text
        assert "0xF801" in text
        assert "__attribute__((aligned(8)))" in text
        assert "px_h4st4a01" in text and "px_{c_ident" not in text
        assert "0x01, 0x02, 0x03, 0x03, 0x02, 0x01" in text


def test_bundle_multi_container() -> None:
    with tempfile.TemporaryDirectory() as td_s:
        td = pathlib.Path(td_s)
        img_dir = td / "IMG"
        img_dir.mkdir()
        (img_dir / "HRT_WLK.IMG").write_bytes(synthetic_wimp("H4ST4A01", "WLKPAL"))
        (img_dir / "HRT_PNC.IMG").write_bytes(synthetic_wimp("H4PL3X01", "PNCPAL"))
        lod = img_dir / "BRET.LOD"
        lod.write_text("hrt_wlk.img\n---> H4ST4A01\nhrt_pnc.img\n---> H4PL3X01\n")
        visual = td / "visual.c"
        visual.write_text('static x a[]={{"H4ST4A01",4},{"H4PL3X01",1}};\n')
        out = td / "bundle.c"
        count, pixels = bundle.emit(out, lod, img_dir, [visual])
        text = out.read_text()
        assert count == 2 and pixels == 12
        assert '"H4ST4A01", "hrt_wlk.img"' in text
        assert '"H4PL3X01", "hrt_pnc.img"' in text
        assert text.count("static uint16_t pal_") == 2




def test_frontend_bundle() -> None:
    with tempfile.TemporaryDirectory() as td_s:
        td = pathlib.Path(td_s)
        source = td / "SPORTLO8.IMG"
        source.write_bytes(synthetic_wimp("SPRTLG01", "SPORTPAL"))
        out = td / "sports_logo.c"
        count, pixels = frontend_bundle.emit(source, out, ["SPRTLG01"])
        text = out.read_text()
        assert count == 1 and pixels == 6
        assert '#include "wm/sports_logo.h"' in text
        assert '"SPRTLG01", "SPORTLO8.IMG", 3, 2, -1, 2' in text
        assert "wm_sports_logo_sprite_find" in text
        assert "wm_sports_logo_sprite_at" in text
        assert "wm_sports_logo_sprite_count" in text




def test_sparkle_bundle() -> None:
    assert len(sparkle_bundle.SPARKLE_NAMES) == 69
    assert sparkle_bundle.SPARKLE_NAMES[0] == "BSPRKA01"
    assert sparkle_bundle.SPARKLE_NAMES[14] == "BSPRKA15"
    assert sparkle_bundle.SPARKLE_NAMES[15] == "BSPRKB01"
    assert sparkle_bundle.SPARKLE_NAMES[29] == "BSPRKB15"
    assert sparkle_bundle.SPARKLE_NAMES[30] == "SPRKLA01"
    assert sparkle_bundle.SPARKLE_NAMES[-1] == "SPRKLC13"

    with tempfile.TemporaryDirectory() as td_s:
        td = pathlib.Path(td_s)
        source = td / "SPARKLE.IMG"
        source.write_bytes(synthetic_wimp("BSPRKA01", "SPARKPAL"))
        out = td / "title_sparkle.c"
        count, pixels = sparkle_bundle.emit(source, out, ["BSPRKA01"])
        text = out.read_text()
        assert count == 1 and pixels == 6
        assert '#include "wm/title_sparkle.h"' in text
        assert '"BSPRKA01", "SPARKLE.IMG", 3, 2, -1, 2' in text
        assert "wm_title_sparkle_sprite_find" in text
        assert "wm_title_sparkle_sprite_at" in text
        assert "wm_title_sparkle_sprite_count" in text
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            f"-I{ROOT / 'include'}", "-c", str(out), "-o", str(td / "sparkle.o")
        ], check=True)


def test_dcs_bundle() -> None:
    with tempfile.TemporaryDirectory() as td_s:
        td = pathlib.Path(td_s)
        source = td / "DCSLOGO.IMG"
        source.write_bytes(synthetic_wimp("DCSLOGO", "DCSPAL"))
        out = td / "dcs_logo.c"
        name, pixels = dcs_bundle.emit(source, out)
        text = out.read_text()
        assert name == "DCSLOGO" and pixels == 6
        assert '#include "wm/dcs_logo.h"' in text
        assert '"DCSLOGO", "DCSLOGO.IMG", 3, 2, -1, 2' in text
        assert "wm_dcs_logo_sprite" in text


def test_bdd_bundle() -> None:
    with tempfile.TemporaryDirectory() as td_s:
        td = pathlib.Path(td_s)
        bdd = td / "TINY.BDD"
        raw = bytearray()
        raw += b"2\n"
        raw += b"A 2 2 1\n" + bytes([0, 1, 2, 3])
        raw += b"B 1 2 0\n" + bytes([0, 1])
        raw += b"P0 4\n" + struct.pack("<4H", 0x0000, 0x7C00, 0x03E0, 0x001F)
        raw += b"P1 2\n" + struct.pack("<2H", 0x0000, 0x7FFF)
        bdd.write_bytes(raw)

        bdb = td / "TINY.BDB"
        bdb.write_text(
            "TINY 100 100 255 1 2 2\n"
            "NTITLESC 10 13 20 22\n"
            "100 10 20 A 0\n"
            "100 12 20 B 1\n"
        )
        bg = td / "BGNDTBL.ASM"
        bg.write_text(
            "TINYBLKS:\n"
            "    .word 0140H,0,0,00H\n"
            "    .word 0141H,2,0,01H\n"
            "NTITLESCBMOD:\n"
            "    .word 3,2,2\n"
            "    .long TINYBLKS, TINYHDRS, TINYPALS\n"
        )

        images, palettes = bdd_bundle.parse_bdd(bdd)
        assert len(images) == 2 and len(palettes) == 2
        assert (images[0].source_id, images[0].width, images[0].height) == (0xA, 2, 2)
        assert images[0].pixels == bytes([0, 1, 2, 3])
        assert palettes[0].name == "P0" and len(palettes[0].words_rgb555) == 4

        region = bdd_bundle.parse_bdb(bdb, "NTITLESC")
        assert len(region.blocks) == 2 and region.blocks[1].source_id == 0xB
        validation = bdd_bundle.validate_sources(images, palettes, region, bg, "NTITLESCBMOD")
        assert validation["footprint"] == (3, 2)
        assert (validation["min_x"], validation["min_y"]) == (10, 20)

        out = td / "title_screen.c"
        bdd_bundle.emit(out, images, palettes, region, validation, bdd, bdb)
        text = out.read_text()
        assert "wm_title_background_image_count" in text
        assert "wm_title_background_palette_at" in text
        assert "0xF801" in text
        # Keyed form makes only palette index zero transparent.
        assert "0x0000, 0xF801" in text
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            f"-I{ROOT / 'include'}", "-c", str(out), "-o", str(td / "title.o")
        ], check=True)

def test_attract_sequence() -> None:
    source = ROOT / "tests" / "fixtures" / "ATTRACT_MIN.ASM"
    labels = attract_sequence.extract(source)
    assert labels == [
        "show_hstd", "DCS_LOGO", "show_sports_logo", "show_gameplay",
        "creditscreen", "show_title", "show_gameplay", "creditscreen",
        "DO_HINTS", "show_gen_tips", "show_bios", "show_bios_tips",
        "show_operatormsg",
    ]
    with tempfile.TemporaryDirectory() as td_s:
        out = pathlib.Path(td_s) / "attract_sequence.c"
        emitted = attract_sequence.emit(source, out)
        assert emitted == labels
        text = out.read_text()
        assert "WM_ATTRACT_DCS_LOGO" in text
        assert text.count("WM_ATTRACT_SHOW_GAMEPLAY") == 2
        assert "commented_out" not in text
        assert "WM_ATTRACT_SHOW_OPERATORMSG" in text


def test_bmod_source_generator() -> None:
    text = """
TESTBLKS:
    .word 0140h,0,133,0
    .word 0235h,-2,16,0a123h
TESTBMOD:
    .word 403,256,2
    .long TESTBLKS, TESTHDRS, TESTPALS
"""
    m = bmod_source.parse_module(text, "TESTBMOD")
    assert m["width"] == 403 and m["height"] == 256 and m["count"] == 2
    assert m["words"] == [0x0140,0,133,0,0x0235,0xfffe,16,0xa123]
    assert m["headers"] == "TESTHDRS" and m["palettes"] == "TESTPALS"
    with tempfile.TemporaryDirectory() as td_s:
        out = pathlib.Path(td_s) / "bmod_tables.c"
        bmod_source.emit([m], out)
        c = out.read_text()
        assert '"TESTBMOD", {403, 256, 2' in c
        assert '0x0140' in c and '0xA123' in c
        assert 'wm_source_bmod_find' in c


def test_source_ir_graph() -> None:
    with tempfile.TemporaryDirectory() as td_s:
        root = pathlib.Path(td_s)
        (root / "A.ASM").write_text("""
SUBRP attract_mode
    JSRP #show_title
    CREATE CYCPID,#cycle_lava
    rets
SUBR show_title
    calla helper
    rets
SUBR helper
    rets
SUBR cycle_lava
    SLEEP 5
    rets
SUBR start_match
    CALLR helper
    CREATE0 X,#wrestler_proc
    rets
SUBR wrestler_proc
    GETPRC
    rets
""")
        data = source_ir.build(root)
        assert data["routine_count"] == 6
        a = source_ir.closure(data, "attract_mode")
        assert set(a["routines"]) == {"attract_mode","show_title","helper","cycle_lava"}
        m = source_ir.closure(data, "start_match")
        assert set(m["routines"]) == {"start_match","helper","wrestler_proc"}
        assert data["routines"]["wrestler_proc"]["dynamic"]
        md = source_ir.render_md(data, ["attract_mode","start_match"])
        assert "Source dependency frontier" in md and "Static closure" in md


def test_animation_ir() -> None:
    with tempfile.TemporaryDirectory() as td_s:
        root = pathlib.Path(td_s)
        (root / "SEQ.ASM").write_text("""
SUBR hrt_test_anim
    WL 2,H4ST4A01+FR1
    .word ANI_SET_XVEL
    .long 012345678h
    .word 1
    WWL ANI_SOUND,7,SND_HIT
    .word ANI_END
SUBR executable_routine
    move a0,a1
""")
        data = animation_ir.build(root)
        assert data["routine_count"] == 2
        assert data["animation_like_routine_count"] == 1
        r = data["animation_like_routines"][0]
        assert r["name"] == "hrt_test_anim"
        assert [x["type"] for x in r["items"]] == ["W","L","W","L","W","W","W","L","W"]
        assert r["items"][1]["expr"] == "H4ST4A01+FR1"
        assert "ANI_SET_XVEL" in r["ani_symbols"]
        assert data["typed_item_counts"]["L"] == 3
        md = animation_ir.render_md(data)
        assert "Animation translation frontier" in md
        assert "LONG items preserved" in md


def test_select_source_generator() -> None:
    src = r"""
#plyrsel_mod
    .long wwfselbkBMOD
    .word -40,0
    .long choiceBMOD
    .word 3,256
    .long 0
#crutplt_z equ 1
crouton_pos_table
    .word 164,45
    .word 204,45
    .word 164,90
    .word 204,90
    .word 164,135
    .word 204,135
    .word 164,180
    .word 204,180
    .word 0
SUBRP player_cursor
wrestler_attributes
    .word 4,9,9,3
    .word 7,6,2,5
    .word 8,4,7,6
    .word 9,2,4,6
    .word 3,9,8,7
    .word 8,6,5,3
    .word 4,8,7,8
    .word 9,5,4,7
    .word 9,5,4,7
scramble_table
    .word 6
    .word 1,2,3,4,5
    .word 0
    .word 8
attbars
    .long A,B
#p1info
    .long X,Y
    .word 0
    .word 0+18+2,175
    .word CTRL
    .long X,Y
    .long INDEX
    .word 0c8h,0cbh
#p2info
    .long X,Y
    .word 1
    .word 400-18,175
    .word CTRL|M_FLIPH
    .long X,Y
    .long INDEX
    .word 0c7h,0cch
#plt_b .word 0
"""
    data = select_source.parse(src)
    assert data["croutons"] == [(164,45),(204,45),(164,90),(204,90),(164,135),(204,135),(164,180),(204,180)]
    assert data["scramble"] == [6,1,2,3,4,5,0,8]
    assert data["attributes"][0] == (4,9,9,3)
    assert data["attributes"][8] == (9,5,4,7)
    assert data["bmods"] == [("wwfselbkBMOD",-40,0),("choiceBMOD",3,256)]
    assert data["players"][0]["start"] == 0 and data["players"][0]["mug"] == (20,175)
    assert data["players"][0]["move_sound"] == 0xc8 and data["players"][0]["select_sound"] == 0xcb
    assert data["players"][1]["start"] == 1 and data["players"][1]["mug"] == (382,175)
    assert data["players"][1]["flip"]
    with tempfile.TemporaryDirectory() as td_s:
        out = pathlib.Path(td_s) / "select_tables.c"
        select_source.emit(data, out)
        text = out.read_text()
        assert '"wwfselbkBMOD", -40, 0' in text
        assert '6, 1, 2, 3, 4, 5, 0, 8' in text
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            f"-I{ROOT / 'include'}", "-c", str(out), "-o", str(pathlib.Path(td_s) / "select.o")
        ], check=True)

def test_source_inventory() -> None:
    with tempfile.TemporaryDirectory() as td_s:
        td = pathlib.Path(td_s)
        (td / "IMG").mkdir()
        for name, module, prefix in inventory_mod.ROSTER:
            (td / module).write_text(f"SUBR {prefix}_main\nWL 2,{prefix.upper()}FRAME+FR1\n")
        (td / "HRTSEQ1.ASM").write_text("SUBR hrt_stand4_anim\nWL 4,H4ST4A+FR1\n.word ANI_END\n")
        (td / "FINISEQ.ASM").write_text("SUBR hrt_finish1_move\nSUBR bam_finish1_move\n")
        (td / "IMG" / "BRET.LOD").write_text("hrt_wlk.img\n---> H4ST4A01\n")
        (td / "IMG" / "HRT_WLK.IMG").write_bytes(b"x")
        data = inventory_mod.inventory(td)
        assert data["asm_files"] >= 10
        assert data["sequence_files"] >= 2
        assert data["subroutines"] >= 10
        assert data["unique_frame_refs"] >= 9
        assert all(r["module_present"] for r in data["roster"])
        md = inventory_mod.render_md(data)
        assert "Bret Hart" in md and "Undertaker" in md

def test_port_manifest() -> None:
    data = port_manifest.load(ROOT / "port" / "translation_manifest.json")
    assert data["attract"]["show_gameplay"]["status"] == "partial-source"
    assert data["attract"]["show_sports_logo"]["status"] == "partial-source"
    assert data["attract"]["show_title"]["status"] == "partial-source"
    with tempfile.TemporaryDirectory() as td_s:
        td = pathlib.Path(td_s)
        out_c = td / "port_status.c"
        out_md = td / "coverage.md"
        port_manifest.emit_c(data, out_c)
        port_manifest.emit_md(data, out_md)
        text = out_c.read_text()
        assert "WM_ATTRACT_SHOW_GAMEPLAY: return WM_PORT_PARTIAL_SOURCE" in text
        assert "WM_ATTRACT_SHOW_SPORTS_LOGO: return WM_PORT_PARTIAL_SOURCE" in text
        assert "WM_ATTRACT_SHOW_TITLE: return WM_PORT_PARTIAL_SOURCE" in text
        assert "harness-only" in out_md.read_text()

def test_source_text_bundle() -> None:
    import zipfile
    with tempfile.TemporaryDirectory() as td_s:
        td = pathlib.Path(td_s)
        src = td / "original"
        (src / "IMG").mkdir(parents=True)
        (src / "ATTRACT.ASM").write_text("SUBR attract_mode\n")
        (src / "ANIM.EQU").write_text("ANI_END EQU 0\n")
        (src / "IMG" / "BRET.LOD").write_text("HRT.IMG\n")
        (src / "IMG" / "HRT.IMG").write_bytes(b"binary-wimp-must-not-be-bundled")
        out = td / "source.zip"
        info = source_text_bundle.build(src, out)
        assert info["file_count"] == 3
        with zipfile.ZipFile(out) as z:
            names = set(z.namelist())
            assert "wwf-wrestlemania/ATTRACT.ASM" in names
            assert "wwf-wrestlemania/IMG/BRET.LOD" in names
            assert "wwf-wrestlemania/IMG/HRT.IMG" not in names
            assert "SOURCE_TEXT_MANIFEST.json" in names

        # Verify the strict checker itself using a synthetic full-enough source tree.
        for name in ["HSTD.ASM", "BRET.ASM", "HRTSEQ1.ASM", "FINISEQ.ASM"]:
            (src / name).write_text(f"; {name}\n" + ("X" * 12000))
        for i in range(50):
            (src / f"EXTRA{i:02d}.ASM").write_text(f"; extra {i}\n")
        source_text_bundle.build(src, out)
        verified = source_text_verify.verify(out, min_files=50, min_bytes=1024)
        assert verified["file_count"] >= 50
        assert verified["bytes"] >= 1024

        empty = td / "empty.zip"
        empty.write_bytes(b"")
        try:
            source_text_verify.verify(empty, min_files=1, min_bytes=1)
        except ValueError:
            pass
        else:
            raise AssertionError("zero-byte source bundle must be rejected")


def main() -> int:
    test_wlanim()
    test_manifest()
    test_wimp_probe()
    test_wimp_emit_c()
    test_bundle_multi_container()
    test_frontend_bundle()
    test_sparkle_bundle()
    test_dcs_bundle()
    test_bdd_bundle()
    test_bmod_source_generator()
    test_attract_sequence()
    test_source_ir_graph()
    test_source_inventory()
    test_port_manifest()
    test_source_text_bundle()
    print("source tool tests passed")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
