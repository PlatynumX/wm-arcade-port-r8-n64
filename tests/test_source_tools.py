#!/usr/bin/env python3
from __future__ import annotations
import importlib.util
import pathlib
import struct
import subprocess
import os
import collections
import inspect
import re
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
bret_manifest = load("bret_manifest", ROOT / "tools" / "bret_manifest.py")
bret_bundle = load("bret_bundle", ROOT / "tools" / "bret_bundle.py")
asmseq = load("asmseq", ROOT / "tools" / "asmseq.py")
wlattack = load("wlattack", ROOT / "tools" / "wlattack.py")
wlcommands = load("wlcommands", ROOT / "tools" / "wlcommands.py")
wlpuppet = load("wlpuppet", ROOT / "tools" / "wlpuppet.py")
wltarget = load("wltarget", ROOT / "tools" / "wltarget.py")
ani_code_census = load("ani_code_census",
                       ROOT / "tools" / "ani_code_census.py")
wlroll = load("wlroll", ROOT / "tools" / "wlroll.py")
wlvoice = load("wlvoice", ROOT / "tools" / "wlvoice.py")
wlwrsnd = load("wlwrsnd", ROOT / "tools" / "wlwrsnd.py")
wlstring = load("wlstring", ROOT / "tools" / "wlstring.py")
wlpal = load("wlpal", ROOT / "tools" / "wlpal.py")
wlrostertbl = load("wlrostertbl", ROOT / "tools" / "wlrostertbl.py")
wlwrestlertbl = load("wlwrestlertbl", ROOT / "tools" / "wlwrestlertbl.py")
port_coverage = load("port_coverage", ROOT / "tools" / "port_coverage.py")
wlverify = load("wlverify", ROOT / "tools" / "wlverify.py")
wlprogram = load("wlprogram", ROOT / "tools" / "wlprogram.py")
manifest = load("bret_manifest", ROOT / "tools" / "bret_manifest.py")
wimp = load("wimpimg", ROOT / "tools" / "wimpimg.py")
bundle = load("bret_bundle", ROOT / "tools" / "bret_bundle.py")
geometry_bundle = load("bret_geometry_bundle",
                       ROOT / "tools" / "bret_geometry_bundle.py")
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


def test_wlprogram() -> None:
    """tools/wlprogram.py emits an animation as the program it really is.

    The flat model cannot represent a branch: a routine that plays
    different frames on a hit than on a miss gets linearised into one list
    no playthrough ever plays. hrt_2_punch_anim is the smallest real
    example -- its ANI_SLIDE_BACK skips a frame when the punch missed --
    and this checks the branch is present and resolved, not flattened away.
    """
    p = ROOT / "original" / "wwf-wrestlemania" / "HRTSEQ2.ASM"
    if not p.exists():
        return

    ops = wlprogram.program_for(p, "hrt_2_punch_anim")
    kinds = [o[0] for o in ops]

    # The header, the attack box with its real operands, and the fork.
    assert kinds[0] == "SETMODE"
    assert ("ATTACK_ON_Z", 0, 30, 91, -45, 50, 15, 45) in ops, ops
    assert "SLIDE_BACK" in kinds
    assert kinds[-1] == "END"

    # The branch target is an op index inside the program, and it really
    # skips forward over the connected-hit frames.
    slide = next(o for o in ops if o[0] == "SLIDE_BACK")
    target = slide[1]
    assert 0 <= target < len(ops), slide
    assert target > kinds.index("SLIDE_BACK"), "slide-back must jump forward"
    skipped = kinds[kinds.index("SLIDE_BACK") + 1:target]
    assert "FRAME" in skipped, skipped

    # Every wired animation emits, branches and all. Three of them branch
    # into shared tail code that sits past their own ANI_END (#common_4,
    # the #missed blocks), which is why the body grows to cover targets.
    for src, label in (("HRTSEQ2", "hrt_4_knee_to_head_anim"),
                       ("HRTSEQ3", "hrt_3_fake_hold_anim"),
                       ("HRTSEQ4", "hrt_faceup_getup_anim")):
        f = ROOT / "original" / "wwf-wrestlemania" / f"{src}.ASM"
        prog = wlprogram.program_for(f, label)
        assert any(o[0] == "FRAME" for o in prog), label
        for o in prog:
            if o[0] in ("IFSTATUS", "IFNOTSTATUS", "IFBLOCKED", "GOTO",
                        "IF_RPTCOUNT", "SLIDE_BACK"):
                assert 0 <= o[1] < len(prog), (label, o)


def test_wlprogram_roster_wide() -> None:
    """The emitter reads the whole roster, not just the wrestler it was
    written against.

    Every animation in the port comes out of one of these nine sequence
    files, so a parser gap is not a one-animation problem -- it multiplies
    by eight. This walks every SUBR in all of them and asserts the playable
    roster still emits essentially completely, which is what makes bringing
    the other seven wrestlers up a data job rather than a translation job.

    A SUBR holding no frames is a helper the animations CALL (HRTSEQ3's
    `set_zvel`, `rope_check`), not an animation, so it is counted apart
    rather than scored as a failure.
    """
    base = ROOT / "original" / "wwf-wrestlemania"
    if not (base / "HRTSEQ2.ASM").exists():
        return

    playable = ["HRT", "RZR", "UND", "YOK", "SHN", "BAM", "DNK", "LEX"]
    emitted = refused = helpers = 0
    per_wrestler = {}
    for who in playable:
        got = 0
        for path in sorted(q for q in base.glob(who + "SEQ*.ASM")
                           if "'" not in q.name):
            labels = []
            for line in path.read_text(errors="replace").splitlines():
                m = wlanim.SUBR_RE.match(line)
                if m and m.group(1) not in labels:
                    labels.append(m.group(1))
            for label in labels:
                try:
                    ops = wlprogram.program_for(path, label)
                except ValueError as exc:
                    if str(exc).endswith("no frames"):
                        helpers += 1
                    else:
                        refused += 1
                    continue
                assert ops, f"{label} emitted an empty program"
                emitted += 1
                got += 1
        per_wrestler[who] = got

    total = emitted + refused
    assert total > 1400, f"only found {total} animations -- did the walk break?"
    assert emitted / total > 0.99, (
        f"{emitted}/{total} emitted; refusals: {refused}")
    assert helpers, "no frameless helper SUBRs seen -- classification broke"
    # No wrestler may be left behind: the point is roster-wide coverage,
    # which an average could hide.
    for who, got in per_wrestler.items():
        assert got > 170, f"{who} only emitted {got} animations"


def test_wlprogram_tick_expressions() -> None:
    """Tick counts are evaluated, including the ones written as products.

    Every wrestler's `*_zip_anim` opens on a one-minute hold spelled either
    `60*60` or `TSEC*60`; TSEC is DISPLAY.EQU:46. Refusing either dropped
    eight real animations, one per wrestler.
    """
    assert wlanim.eval_ticks("60*60") == 3600
    assert wlanim.GLOBAL_EQU["TSEC"] == 53
    assert wlanim.eval_ticks("TSEC*60") == 53 * 60
    assert wlanim.eval_ticks("0Ah") == 10
    for bad in ("SOME_UNDEFINED_NAME", "0", "99999"):
        try:
            wlanim.eval_ticks(bad)
        except ValueError:
            continue
        raise AssertionError(f"eval_ticks accepted {bad!r}")


def test_wlanim_label_def() -> None:
    """A branch target may be a file-scope label, not only a `#local`.

    SHNSEQ2.ASM:1744 defines `getup_in_4` in column 0 and four routines
    branch to it; to the assembler that resolves exactly like a local does.
    Treating only `#names` as labels made those four animations unemittable.
    """
    assert wlanim.label_def("#cont") == "#cont"
    assert wlanim.label_def("getup_in_4") == "getup_in_4"
    assert wlanim.label_def("\tWL\t5,H4SL4C+FR1") is None
    assert wlanim.label_def(" SUBR\thrt_2_punch_anim") is None
    assert wlanim.label_def("#RUN_SPD\tequ\t2") == "#RUN_SPD"


def test_wlprogram_is_deterministic() -> None:
    """The same source must emit the same program, run after run.

    It did not. The body-growth loop iterated a SET of missing branch
    targets, and growing the body for one target can satisfy or move
    others -- so which came first decided where the body ended up, and set
    iteration order depends on PYTHONHASHSEED. yok_heldheadbutt_rpt_anim
    emitted 94 ops under one seed and 203 under another, which made the
    generated file change from run to run for no reason visible in the
    source. It cost two wrong diagnoses ("the checked-in file is stale")
    before the real cause turned up.

    This runs the emitter in subprocesses under different hash seeds, since
    the seed is fixed for the life of a process and cannot be changed from
    inside one.
    """
    base = ROOT / "original" / "wwf-wrestlemania"
    if not (base / "YOKSEQ3.ASM").exists():
        return

    script = (
        "import pathlib, sys\n"
        f"sys.path.insert(0, {str(ROOT / 'tools')!r})\n"
        "import wlprogram\n"
        "out = []\n"
        "for f, l in ("
        "    ('YOKSEQ3.ASM', 'yok_heldheadbutt_rpt_anim'),"
        "    ('HRTSEQ2.ASM', 'hrt_2_punch_anim'),"
        "    ('HRTSEQ2.ASM', 'hrt_knees_to_head_anim'),"
        "    ('RZRSEQ2.ASM', 'rzr_4_uprcut_anim')):\n"
        f"    p = pathlib.Path({str(base)!r}) / f\n"
        "    out.append('%s=%d' % (l, len(wlprogram.program_for(p, l))))\n"
        "print(' '.join(out))\n"
    )
    results = set()
    for seed in ("0", "1", "2", "7"):
        env = dict(os.environ, PYTHONHASHSEED=seed)
        proc = subprocess.run([sys.executable, "-c", script],
                              capture_output=True, text=True, env=env)
        assert proc.returncode == 0, proc.stderr
        results.add(proc.stdout.strip())
    assert len(results) == 1, (
        "emission depends on PYTHONHASHSEED: " + " | ".join(sorted(results)))


def test_body_stop_ends_a_frameless_routine() -> None:
    """A routine with no frames anywhere must not swallow the file.

    _body_stop treats a SUBR reached before any frame as an ALIAS for the
    routine after it, which is real (HRTSEQ2.ASM:1334-1335
    hrt_2/4_super_kick_anim). But a routine with no frames AT ALL makes
    every following SUBR look like an alias, so the body runs on forever.
    FINISEQ.ASM's finish moves are exactly that -- eight lines of commands
    ending in ANI_END, behind a `.if NUM_xxx_FINISHES` guard, with no
    artwork behind them -- and rzr_finish1_move came out reporting ten
    frames belonging to routines further down the file.

    The discriminator is whether the routine terminates before the next
    SUBR: an alias does not, a finished routine does.

    Once the span is right, a frameless routine is emitted rather than
    refused -- it is a real op stream that happens to draw nothing, and
    FINISEQ.ASM:259-268 is exactly the seven commands below. What the
    runaway looked like was ten FRAMEs belonging to routines further down
    the file, so that is what this checks for.
    """
    base = ROOT / "original" / "wwf-wrestlemania"
    if not (base / "FINISEQ.ASM").exists():
        return

    expect = ["SETMODE", "ZEROVELS", "SETSPEED", "SETFACING",
              "SET_WRESTLER_XFLIP", "SETMODE", "END"]
    for label in ("rzr_finish1_move", "rzr_finish2_move"):
        ops = wlprogram.program_for(base / "FINISEQ.ASM", label)
        assert [o[0] for o in ops] == expect, (label, [o[0] for o in ops])

    # ...while the alias case still runs on into the routine it names.
    if (base / "HRTSEQ2.ASM").exists():
        ops = wlprogram.program_for(base / "HRTSEQ2.ASM", "hrt_4_super_kick_anim")
        assert any(o[0] == "FRAME" for o in ops), "the SUBR alias rule broke"


def test_roster_dispatcher_labels_all_emit() -> None:
    """Every animation the six label-based dispatchers can select emits.

    Undertaker, Yokozuna, Shawn, Bam Bam, Doink and Lex select animations by
    the source's OWN routine name -- their modules carry tables of
    "und_2_punch_anim" and the like -- and the generated programs are keyed
    on exactly that name. So this is the join that makes those six animate,
    and a label that stops emitting silently un-animates whatever selects
    it. Walking the module sources rather than a copied list means a label
    added to a dispatcher is covered the moment it appears.
    """
    base = ROOT / "original" / "wwf-wrestlemania"
    if not (base / "UNDSEQ2.ASM").exists():
        return

    modules = {
        "und": "wm_arcade_taker.c",
        "yok": "wm_arcade_yoko.c",
        "shn": "wm_arcade_shawn.c",
        "bam": "wm_arcade_bam.c",
        "dnk": "wm_arcade_doink.c",
        "lex": "wm_arcade_lex.c",
    }
    prefix_dir = {"und": "UND", "yok": "YOK", "shn": "SHN",
                  "bam": "BAM", "dnk": "DNK", "lex": "LEX"}
    label_re = re.compile(r'"([a-z]{3}_[A-Za-z0-9_]*anim)"')

    total = 0
    for prefix, filename in modules.items():
        src = ROOT / "src" / "core" / "arcade" / filename
        assert src.exists(), f"{filename} is gone -- did a wrestler get renamed?"
        labels = sorted(set(label_re.findall(src.read_text())))
        labels = [l for l in labels if l.startswith(prefix + "_")]
        assert len(labels) > 25, f"{prefix}: only found {len(labels)} labels"
        seq_files = sorted(q for q in base.glob(prefix_dir[prefix] + "SEQ*.ASM")
                           if "'" not in q.name)
        for label in labels:
            path = next(
                (q for q in seq_files
                 if wlanim._routine_span(
                     [wlanim.strip_comment(r)
                      for r in q.read_text(errors="replace").splitlines()],
                     label)),
                None)
            assert path is not None, f"{label}: no routine in any {prefix_dir[prefix]}SEQ file"
            ops = wlprogram.program_for(path, label)
            assert ops, f"{label} emitted an empty program"
            total += 1
    assert total > 200, f"only {total} roster labels checked"


def test_wlattack_audit() -> None:
    """tools/wlattack.py --audit's job is to answer "would a flat
    wlanim.py --slice of this routine be faithful to any single real
    playthrough?" -- the question that decides whether an animation can
    honestly be wired at all. Checked against real HRTSEQ2.ASM routines
    whose control flow is known by reading them:

      hrt_2_punch_anim   straight line plus forward skips -> sliceable
      hrt_2_butts_anim   ANI_SET_RPTCOUNT repeat loop and a terminal
                         ANI_CHANGEANIM -> not sliceable
      hrt_2_raise_arm_anim  no ANI_END; ANI_GOTO,#cont into the middle of
                         hrt_4_raise_arm_anim -> not sliceable

    The first of those is load-bearing in the other direction too: every
    already-wired attack has to keep passing, or the audit is calling
    shipped work broken.
    """
    p = ROOT / "original" / "wwf-wrestlemania" / "HRTSEQ2.ASM"
    if not p.exists():
        return  # original source not fetched in this checkout

    def verdict(label):
        findings, _term = wlattack.audit(p, label)
        return [why for sev, why in findings if sev == "blocking"]

    assert verdict("hrt_2_punch_anim") == []

    # hrt_2_butts_anim is fully representable now: its ANI_SET_RPTCOUNT,3
    # span is carried as loop fields, and its ANI_CHANGEANIM is recognised
    # as the terminator ANIM.ASM:1301 actually makes it rather than a
    # mid-stream command.
    butts = verdict("hrt_2_butts_anim")
    assert butts == [], butts

    bseq = wlanim.extract_visual_slice(p, "hrt_2_butts_anim", False)
    assert (bseq.loop_first, bseq.loop_last, bseq.loop_count) == (0, 7, 3)
    # Its ANI_CHANGEANIM is reached when the repeat loop runs out, but the
    # routine ALSO has a real ANI_END on its button-mash #ex path, so the
    # transition is one exit among several rather than the terminator --
    # the flat list keeps walking to that ANI_END, the same "longest real
    # path" rule every forward branch already gets. Treating it as
    # terminal truncated this to 8 frames and claimed a transition it does
    # not unconditionally take.
    assert bseq.next_label is None, bseq.next_label
    assert len(bseq.frames) == 9, len(bseq.frames)

    # hrt_facedown_getup_anim is the mirror image: its ANI_CHANGEANIM sits
    # inside an ANI_IFNOTSTATUS free-toss branch with the ordinary ending
    # below it, so it is not the terminator either.
    getup = wlanim.extract_visual_slice(
        ROOT / "original" / "wwf-wrestlemania" / "HRTSEQ4.ASM",
        "hrt_facedown_getup_anim", False)
    assert getup.next_label is None, getup.next_label

    # _ani_changeanim (ANIM.ASM:1301) overwrites OANIPC *and* OANIBASE and
    # never returns, and the source confirms it by commenting out the
    # `.word ANI_END` that follows it in 16 places. Treating it as an
    # ordinary command made routines read far longer and messier than they
    # are -- hrt_fall_back_anim as 55 frames with four transitions when it
    # is 12 frames with one, hrt_flying_kick_anim as 38 rather than 9.
    fb = wlanim.extract_visual_slice(
        ROOT / "original" / "wwf-wrestlemania" / "HRTSEQ4.ASM",
        "hrt_fall_back_anim", False)
    assert len(fb.frames) == 12, len(fb.frames)
    assert fb.next_label == "hrt_faceup_getup_anim", fb.next_label
    fk = wlanim.extract_visual_slice(p, "hrt_flying_kick_anim", False)
    assert len(fk.frames) == 9, len(fk.frames)
    assert fk.next_label == "hrt_facedown_getup_anim", fk.next_label

    # hrt_2_raise_arm_anim has no ANI_END of its own: it ends in
    # `ANI_GOTO,#cont`, a label inside hrt_4_raise_arm_anim. The extractor
    # follows that, so the GOTO is not the blocker -- what it lands in is.
    # hrt_4_raise_arm_anim's ANI_SET_RPTCOUNT is NEGATIVE (-4), i.e.
    # RNDRNG0(4) drawn at runtime (ANIM.ASM:3538), so its iteration count is
    # not fixed and no static table can carry it. Refusing that is the point:
    # baking in a number the source rolls for would be inventing data.
    raise_arm = verdict("hrt_2_raise_arm_anim")
    assert any("negative" in w or "RNDRNG0" in w for w in raise_arm), raise_arm
    assert not any("#cont" in w for w in raise_arm)

    # An ANI_IF_RPTCOUNT that branches FORWARD is a first pass plus a
    # separate repeated block sharing one RPT_COUNT, which a single loop
    # span cannot represent -- also refused rather than emitted inverted.
    ups = verdict("hrt_uppercuts_to_head_anim")
    assert any("FORWARD" in w for w in ups), ups

    # The three that the loop fields genuinely do make representable.
    for lab, span in (("hrt_2_pin_anim", (18, 27, 3)),
                      ("hrt_4_pin_anim", (16, 25, 3)),
                      ("hrt_knees_to_head_anim", (1, 5, 3))):
        assert verdict(lab) == [], (lab, verdict(lab))
        q = wlanim.extract_visual_slice(p, lab, False)
        assert (q.loop_first, q.loop_last, q.loop_count) == span, (lab, q)

    # Chaining, on a routine whose continuation is representable end to end:
    # hrt_4_knee_to_head_anim ends in ANI_GOTO,#cont with no ANI_END of its
    # own, so under its own label it is a single frame; followed, it is the
    # real 8-frame stream, and its ANI_ATTACK_ON lands at index 2 -- past the
    # end of the unchained fragment entirely.
    seq = wlanim.extract_visual_slice(p, "hrt_4_knee_to_head_anim", False)
    assert len(seq.frames) == 8, len(seq.frames)
    frames, events = wlattack.trace(p, "hrt_4_knee_to_head_anim")
    assert len(frames) == 8
    assert [(i, o) for i, c, o in events if c == "ANI_ATTACK_ON"] == [
        (2, "AMODE_KNEE,11,44,51,49")]

    # Local labels are reused across routines (#cont, #hit, #missed appear in
    # many), so a chain target must resolve forward from the routine itself.
    # Resolving from the top of the file picked up an unrelated earlier
    # #cont, which for hrt_2_raise_arm_anim produced frames from a different
    # animation entirely (H4NM3A*) instead of its own shared tail (H4SL4C*).
    lines = [wlanim.strip_comment(r)
             for r in p.read_text(errors="replace").splitlines()]
    order = wlanim.slice_line_order(lines, "hrt_2_raise_arm_anim")
    chained = [wlanim._frame_from_line(lines[i]) for i in order]
    names = [f.name for f in chained if f]
    assert names[:2] == ["H1TL5A03", "H1TL5A04"], names[:2]
    # The frame sharing a line with the #cont label must not be dropped.
    assert names[2] == "H4SL4C01", names[2]
    assert not any(n.startswith("H4NM3A") for n in names), names

    # Every attack animation actually wired into the Bret backend must be
    # sliceable, or the extraction backing it is not a real playthrough.
    for label in ("hrt_2_punch_anim", "hrt_4_punch_anim",
                  "hrt_4_super_punch_anim", "hrt_2_kick_anim",
                  "hrt_4_kick_anim", "hrt_2_super_kick_anim",
                  "hrt_2_butt_anim", "hrt_4_butt_anim",
                  "hrt_2_knee_anim", "hrt_4_knee_anim",
                  "hrt_4_uppercut_anim", "hrt_2_stomp_anim",
                  "hrt_4_stomp_anim", "hrt_2_ground_punch_anim",
                  "hrt_4_ground_punch_anim", "hrt_4_push_anim",
                  "hrt_4_jump_kick_anim", "hrt_4_knee_fall_anim",
                  "hrt_kick_TB_anim"):
        assert verdict(label) == [], (label, verdict(label))

    # A SUBR alias (HRTSEQ2.ASM:1334-1335 hrt_2/4_super_kick_anim) must not
    # be mistaken for an empty routine: its own local labels live in the
    # body that follows, and missing them made #missed read as an
    # out-of-routine jump.
    assert "#missed" in wlattack._routine_local_labels(p, "hrt_2_super_kick_anim")


def test_wlattack_frame_indices() -> None:
    """The frame index each inline command falls at -- the number an
    attack window table needs. Checked against windows that were hand
    traced from the .ASM long before this tool existed, which is the whole
    basis for trusting it on animations nobody has traced."""
    p = ROOT / "original" / "wwf-wrestlemania" / "HRTSEQ2.ASM"
    if not p.exists():
        return

    def attack_ons(label):
        _frames, events = wlattack.trace(p, label)
        return [(idx, ops) for idx, cmd, ops in events
                if cmd in ("ANI_ATTACK_ON", "ANI_ATTACK_ON_Z")]

    assert attack_ons("hrt_2_punch_anim") == [
        (5, "AMODE_PUNCH,30,91,-45,50,15,45")]
    assert attack_ons("hrt_2_super_kick_anim") == [
        (4, "AMODE_SUPER_KICK,5,54,70,34")]
    # Multi-pulse: two and three real ANI_ATTACK_ON commands respectively.
    assert attack_ons("hrt_2_stomp_anim") == [
        (4, "AMODE_HITCHECK,7,-10,-40,28,31,50"),
        (7, "AMODE_STOMP2,7,-10,-40,28,31,50")]
    assert attack_ons("hrt_2_ground_punch_anim") == [
        (2, "AMODE_HITCHECK,5-10,-8,-40,32,32,50"),
        (5, "AMODE_LBOWDROP2,5,-8,-40,32,32,50"),
        (8, "AMODE_LBOWDROP2,5,-8,-40,32,32,50")]


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


def test_linked_files_is_the_game() -> None:
    """The drop holds .ASM files the game does not link, and they collide.

    WRESTLE.CMD is the linker command file. ADMSEQ1-3.ASM (Adam Bomb, cut
    from the roster), REFSEQ1.ASM and the superseded HRTSEQ.ASM/YOKSEQ.ASM
    sit in the same directory but are not in it -- and ADMSEQ3.ASM:199
    defines `dnk_3_head_held_anim`, the same global DNKSEQ3.ASM:1624
    defines. Searching the directory alphabetically finds Adam's.
    """
    base = ROOT / "original" / "wwf-wrestlemania"
    if not (base / "WRESTLE.CMD").exists():
        return

    names = {q.name for q in wlanim.linked_files()}
    for dead in ("ADMSEQ1.ASM", "ADMSEQ2.ASM", "ADMSEQ3.ASM", "REFSEQ1.ASM",
                 "HRTSEQ.ASM", "YOKSEQ.ASM"):
        assert dead not in names, f"{dead} is not in WRESTLE.CMD"
    for live in ("HRTSEQ3.ASM", "DNKSEQ3.ASM", "ANIM.ASM", "WRESTLE2.ASM",
                 "FINISEQ.ASM"):
        assert live in names, f"{live} IS in WRESTLE.CMD"

    where = dict((lab, path) for path, lab in wlpuppet.slave_targets())
    if "dnk_3_head_held_anim" in where:
        assert where["dnk_3_head_held_anim"].name == "DNKSEQ3.ASM", \
            where["dnk_3_head_held_anim"]


def test_bare_label_routines() -> None:
    """An animation is not always behind a SUBR.

    UNDSEQ3.ASM:876-1046 is eight choking animations named by plain
    column-0 labels, one after another, none ending in ANI_END -- each
    ends in an ANI_CHANGEANIM, so the only boundary is the next label.
    The slave tables name them exactly as they name SUBRs, so they have to
    resolve, and each has to stop at its own end rather than running on
    into the seven that follow.
    """
    src = ROOT / "original" / "wwf-wrestlemania" / "UNDSEQ3.ASM"
    if not src.exists():
        return

    # Frame counts read off the source, routine by routine.
    expect = {"hrt_choking_anim": 10, "rzr_choking_anim": 10,
              "und_choking_anim": 12, "yok_choking_anim": 9,
              "shn_choking_anim": 9, "bam_choking_anim": 6,
              "dnk_choking_anim": 12, "lex_choking_anim": 8}
    for label, frames in expect.items():
        ops = wlprogram.program_for(src, label)
        got = sum(1 for o in ops if o[0] == "FRAME")
        assert got == frames, (label, got, frames)
        assert ops[-1][0] == "CHANGEANIM", (label, ops[-1])


def test_slave_targets_all_emit() -> None:
    """Every animation ANI_SLAVEANIM can hand a victim has to be playable.

    The op names a whole animation for the OTHER wrestler to run, chosen
    out of a nine-slot table by his own WRESTLERNUM. An entry naming an
    animation the port cannot emit is a hole the runtime would fall into,
    so the set is read out of the tables and every one of them is emitted.
    """
    base = ROOT / "original" / "wwf-wrestlemania"
    if not (base / "ANIM.ASM").exists():
        return

    targets = wlpuppet.slave_targets()
    assert len(targets) > 150, len(targets)
    for path, label in targets:
        ops = wlprogram.program_for(path, label)
        assert ops, label

    generated = (ROOT / "src" / "generated" / "anim_programs.c")
    if generated.exists():
        text = generated.read_text()
        for _path, label in targets:
            assert f'"{label}"' in text, f"{label} is named by a slave table "
    # ...and where a table's slot is `.long 0` it stays empty rather than
    # being filled with a guess. Slot 7 is Adam Bomb's, the wrestler who
    # was cut: most tables write 0 there, some write the Undertaker's
    # animation, and neither is invented here.
    rows = [r for p in wlpuppet.canonical_files()
            for r in wlpuppet.slave_tables_in(p).values()]
    assert rows, "no slave tables at all"
    assert any(r[7] == "" for r in rows), "no `.long 0` slot survived"
    assert all(len(r) == wlpuppet.ROSTER_SLOTS for r in rows), "short table"


def test_code_roster_tables_are_read_not_transcribed() -> None:
    """The per-wrestler tables an ANI_CODE routine indexes itself.

    No opcode names these, so tools/wlpuppet.py's CODE_TABLES names the
    site -- but only the site. Every row still comes out of the source,
    still has to be nine slots, and still has to name animations this port
    can actually play.
    """
    if not (wlanim.ORIG / "DNKSEQ2.ASM").exists():
        return
    tables = wlpuppet.code_tables()
    # DNKSEQ2's `#hit_t`, plus FINISEQ's two -- stand_table and
    # dizzy_table, which the Undertaker's coffin finish uses to stand
    # the dead man up and leave him swaying. Both of those are written
    # ` SUBRP stand_table` rather than as a column-0 label, which is why
    # the reader accepts either spelling.
    assert set(tables) == {"grnd_hit", "stand_wrestler",
                           "dizzy_wrestler"}, sorted(tables)

    generated = ROOT / "src" / "generated" / "anim_programs.c"
    for routine, rows in tables.items():
        assert len(rows) == wlpuppet.ROSTER_SLOTS, (routine, len(rows))
        # Slot 7 is Adam Bomb, cut from the game: a literal 0 in the
        # source, and it stays empty rather than being filled in.
        assert rows[7] == "", (routine, rows[7])
        for i, name in enumerate(rows):
            if not name:
                continue
            assert name.endswith("_anim"), (routine, i, name)
            if generated.exists():
                assert f'"{name}"' in generated.read_text(), (routine, name)

    # And the routine that reads one is registered under exactly the name
    # the generated lookup is keyed on -- the same spelling trap the
    # ANI_CODE registry and the announcer callers each have their own
    # guard for.
    reg = (ROOT / "src" / "core" / "anim_code.c").read_text()
    for routine in tables:
        assert f'{{ "{routine}",' in reg, routine
    aux = ROOT / "src" / "generated" / "anim_aux_tables.c"
    if aux.exists():
        text = aux.read_text()
        for routine in tables:
            assert f'"{routine}",' in text, routine


def test_truncated_frame_names() -> None:
    """A WIMP name field is eight characters; .LOD names can be longer.

    BAM.LOD lists BURNBODY01..BURNBODY05, and bam_jms.img stores five
    images all called `BURNBODY`. The .LOD's packing order is the
    container's order, so the nth full name is the nth image -- but only
    when the counts agree.
    """
    img_dir = ROOT / "original" / "wwf-wrestlemania" / "IMG"
    if not (img_dir / "BAM_JMS.IMG").exists():
        return

    _data, _hdr, images, _pal = wimp.parse_file(img_dir / "BAM_JMS.IMG")
    by_name = {im.name.upper(): im for im in images}
    lod = ["BURNBODY0%d" % n for n in range(1, 6)]
    picked = [geometry_bundle._by_truncated_name(f, lod, images, by_name)
              for f in lod]
    assert all(p is not None for p in picked), picked
    assert len({id(p) for p in picked}) == 5, "five names, five images"

    # A stem the container does not hold resolves to nothing rather than
    # to whatever happens to be near it.
    assert geometry_bundle._by_truncated_name(
        "NOTHERE01", ["NOTHERE01"], images, by_name) is None


def test_gravity_opcodes() -> None:
    """The gravity/ground group, read off the source rather than assumed.

    ANI_WAITHITGND takes no operands at all; ANI_BOUNCE takes one that the
    handler shifts left 16 into OBJ_YVEL; ANI_SETLONG names a process field
    and a LONG, and across the eight playable wrestlers it names exactly
    two -- OBJ_GRAVITY and DEBRIS_X -- so a third would be a silent hole
    and refuses instead.
    """
    base = ROOT / "original" / "wwf-wrestlemania"
    if not (base / "HRTSEQ4.ASM").exists():
        return

    ops = wlprogram.program_for(base / "HRTSEQ4.ASM", "hrt_fall_back_anim")
    kinds = [o[0] for o in ops]
    assert kinds.count("WAITHITGND") == 2, kinds
    assert "BOUNCE" in kinds, kinds
    # ANI_BOUNCE,5 -- the operand is carried through unshifted; the shift
    # is the runtime's, exactly as ANIM.ASM:950 does it.
    assert [o for o in ops if o[0] == "BOUNCE"][0][2] == 5, ops

    # hrt_flyout_anim gives itself a heavier fall and puts the default back.
    ops = wlprogram.program_for(base / "HRTSEQ4.ASM", "hrt_flyout_anim")
    setlongs = [o for o in ops if o[0] == "SETLONG"]
    assert len(setlongs) == 2, setlongs
    assert all(o[1] == 0 for o in setlongs), setlongs   # field 0 = OBJ_GRAVITY
    assert setlongs[0][2] == 0xE000 and setlongs[1][2] == 0x8000, setlongs

    # Only the two known fields; anything else is refused rather than
    # written to whatever happens to be at that offset.
    assert set(wlprogram.SETLONG_FIELDS) == {"OBJ_GRAVITY", "DEBRIS_X"}


def test_a_label_past_the_last_op_is_not_a_position() -> None:
    """A symbol after the animation's last command names no op.

    Every routine ends in trailing assembly, a puppet table or a pair of
    `equ` lines, and each symbol among them was being recorded at index
    len(ops) -- one past the end. 1,794 across the corpus. Nothing at the
    C end distinguishes that from a real index, so an ANIPC redirect to
    `#yoff` or `#puppet_tbl` got back a position the dispatcher then read
    as "the program is over" and killed the animation. Exactly the shape
    of the ANI_IFROPE defect.

    Two rules, both checked here: a branch to such a label is refused
    outright, and the label map a program carries holds only labels that
    name an op the animation can reach.
    """
    base = ROOT / "original" / "wwf-wrestlemania"
    if not (base / "BAMSEQ2.ASM").exists():
        return

    # BAMSEQ2.ASM:759 bam_4_losebal_anim ends on ANI_END, and `#yoff equ
    # 50` follows it -- a scoped equate for the NEXT routine, picked up as
    # a label because it looks like one.
    ops, entry, labels = wlprogram.program_for(
        base / "BAMSEQ2.ASM", "bam_4_losebal_anim", with_entry=True)
    assert "#yoff" not in labels, labels
    for name, at in labels.items():
        assert at < len(ops), (name, at, len(ops))

    # And across the whole roster: no program hands out a label that is
    # not a reachable op index.
    checked = 0
    for src in wlanim.linked_files():
        if "SEQ" not in src.name or src.name.startswith("FINI"):
            continue
        lines = src.read_text(errors="replace").splitlines()
        for line in lines:
            m = wlanim.SUBR_RE.match(line)
            if not m:
                continue
            try:
                ops, entry, labels = wlprogram.program_for(
                    src, m.group(1), with_entry=True)
            except (OSError, ValueError):
                continue
            checked += 1
            assert entry < len(ops), (src.name, m.group(1))
            for name, at in labels.items():
                assert at < len(ops), (src.name, m.group(1), name, at)
    assert checked > 1400, checked


def test_a_branch_can_reach_the_head_of_a_later_routine() -> None:
    """ANI_IFSTATUS writes the animation PC, so it can name a SUBR.

    ANIM.ASM:29 is `move a0,a4` -- a raw PC write, not a call. Three
    animations use that to reach the head of another routine rather than
    a local label inside their own:

        BAMSEQ2.ASM:3443 bam_faceup_getup_anim -> bam_4_faceup_getup_anim
        DNKSEQ2.ASM:2013 dnk_faceup_getup_anim -> dnk_2_faceup_getup_anim
        YOKSEQ3.ASM:3063 yok_combo_scissor_anim -> yok_overhd_slam_anim

    wlanim.label_def cannot see those definitions -- its GLOBAL_LABEL_RE
    wants the name alone in column 0, which is right for the other job it
    does, delimiting a local label's scope. So the emitter refused all
    three, and because ANI_CHANGEANIM elsewhere names them, a wrestler
    knocked onto his back changed to an animation nothing had emitted and
    stopped where he lay.

    The branch has to resolve to a real op, not merely stop raising.
    """
    base = ROOT / "original" / "wwf-wrestlemania"
    if not (base / "BAMSEQ2.ASM").exists():
        return

    for name, src, target in (
            ("bam_faceup_getup_anim", "BAMSEQ2.ASM", "bam_4_faceup_getup_anim"),
            ("dnk_faceup_getup_anim", "DNKSEQ2.ASM", "dnk_2_faceup_getup_anim"),
            ("yok_combo_scissor_anim", "YOKSEQ3.ASM", "yok_overhd_slam_anim")):
        ops, entry, labels = wlprogram.program_for(
            base / src, name, with_entry=True)
        assert target in labels, (name, sorted(labels))
        at = labels[target]
        assert 0 <= at < len(ops), (name, target, at, len(ops))
        # And something actually branches there.
        assert any(o[0] in wlprogram.BRANCH_OPS and o[1] == at for o in ops), \
            (name, target, at)
        # The routine still starts at its own first command, not at the
        # code it merely branches into.
        assert entry < len(ops), (name, entry, len(ops))

    # A SUBR line is a definition for the purpose of resolving a branch,
    # and NOT for the purpose of cutting a local label's scope -- those
    # are different questions and wlanim answers the second one.
    assert wlprogram._label_def(" SUBR\tyok_overhd_slam_anim") == \
        "yok_overhd_slam_anim"
    assert wlanim.label_def(" SUBR\tyok_overhd_slam_anim") is None
    assert wlprogram._label_def("\t.word\tANI_END") is None


def test_per_wrestler_anim_tables_come_out_of_the_dispatch_lists() -> None:
    """Each wrestler's own turn/walk/torso tables, read not assumed.

    Three things here are read from the source rather than built from a
    prefix per wrestler, because each of them is a place the obvious
    guess is wrong:

      * the roster order comes from WRESTLE.ASM's own #wres_*_anims
        lists, where slot 7 (Adam Bomb, cut) carries DOINK's tables and
        the Referee's rotate slot is a plain 0;
      * the defining file is resolved against WRESTLE.CMD, because
        DNK.ASM and DOINK.ASM both define dnk_rotate_anims_table and
        TEMPLATE.ASM defines a copy of bam_rotate_anims_table -- only
        DOINK.ASM is linked;
      * the shapes are the index arithmetic in change_walk_anim
        (`srl 1` + `X4` for the 4x4s, `X8` for the 8x8), so a table that
        does not fill its shape is refused rather than padded.
    """
    if not (wlanim.ORIG / "WRESTLE.ASM").exists():
        return

    tables = wlwrestlertbl.tables()
    assert set(tables) == {"rotate", "torso", "leg"}

    for kind, (_label, rows, cols) in wlwrestlertbl.KINDS.items():
        slots = tables[kind]
        assert len(slots) == wlwrestlertbl.ROSTER_SLOTS, (kind, len(slots))
        for entry in slots:
            if entry is None:
                continue
            name, grid = entry
            assert len(grid) == rows, (name, len(grid))
            for row in grid:
                assert len(row) == cols, (name, row)

    # Adam Bomb's slot is Doink's table, not an empty one and not a
    # table of his own -- ADAM.ASM is in the tree but not linked.
    for kind in ("rotate", "torso", "leg"):
        assert tables[kind][7] == tables[kind][6], kind
        assert tables[kind][7][0].startswith("dnk_"), kind

    # The Referee has no tables at all.
    for kind in ("rotate", "torso", "leg"):
        assert tables[kind][9] is None, kind

    # Doink's table resolves to the LINKED file.
    assert wlwrestlertbl._defining_file("dnk_rotate_anims_table").name \
        == "DOINK.ASM"
    assert wlwrestlertbl._defining_file("bam_rotate_anims_table").name \
        == "BAM.ASM"

    # The 4x4s are a turn matrix: the diagonal is a pose, everything
    # else a turn. The torso's are the `2` variants.
    for slot in range(9):
        rot = tables["rotate"][slot][1]
        tor = tables["torso"][slot][1]
        for d in range(4):
            assert "_stand" in rot[d][d], (slot, rot[d][d])
            assert "_torso" in tor[d][d], (slot, tor[d][d])
        assert "turn" in rot[0][1] and "turn2" not in rot[0][1]
        assert "turn2" in tor[0][1]

    # And every label is an animation, spelled the way a program is.
    for kind in tables:
        for entry in tables[kind]:
            if entry is None:
                continue
            for row in entry[1]:
                for lab in row:
                    assert re.fullmatch(r"[a-z]{3}_[a-z0-9_]+_anim", lab), lab


def test_per_wrestler_tables_generate_the_shipped_file() -> None:
    """The checked-in file is what the tool emits, byte for byte."""
    out = ROOT / "src" / "generated" / "wrestler_anim_tables.c"
    if not (wlanim.ORIG / "WRESTLE.ASM").exists() or not out.exists():
        return
    assert wlwrestlertbl.render_c() == out.read_text()


def test_smove_monitor_registry_is_honest() -> None:
    """The special-move watchdogs the port implements, and the ones it does not.

    WRESTLE2.ASM:4058 init_smoves makes one SMOVE_PID process per smove
    table entry, and 79 entries across the eight tables name 65 distinct
    routines -- std_walk_fast and std_taunt are the only two shared, one
    apiece in all eight. The port now implements all 65: five written
    by hand and sixty read out of the source by tools/wlsmove.py in
    four families (40 head-hold, 6 charge, 8 grab_toss_air, 6 free).
    The point of this test is that nothing can quietly fall back out.

    They are easy to lose. Every one is a bare column-0 label rather
    than a SUBR, so tools/port_coverage.py -- which enumerates SUBR
    lines -- never counted them at all: they were neither implemented
    nor open, they simply were not in the denominator. A registry that
    quietly claimed a name it did not implement, or a table entry that
    stopped resolving, would be invisible the same way.

    So: every name the registry claims must be a real label in the
    source, and the registry must not grow without this count moving.
    """
    if not (wlanim.ORIG / "WRESTLE2.ASM").exists():
        return

    tables = wlwrestlertbl.smove_tables()
    entries = [lab for e in tables if e for lab in e[1]]
    names = sorted(set(entries))
    assert len(entries) == 79, len(entries)
    assert len(names) == 65, len(names)

    reg = (ROOT / "src" / "core" / "arcade" / "wm_arcade_smove.c").read_text()
    gen = (ROOT / "src" / "generated" / "smove_tables.c")
    hand = sorted(set(re.findall(
        r'^\s*\.name = "([a-z0-9_]+)", \.file = "([A-Z0-9]+\.ASM)"',
        reg, re.M)))
    assert [c[0] for c in hand] == ["shn_flipslam", "shn_swirl_speedkick",
                                    "std_taunt", "std_walk_fast",
                                    "und_finish_move1"], hand
    # ...plus the four families, read out of the source by
    # tools/wlsmove.py rather than written here.
    generated = sorted(set(re.findall(
        r'^\s*\{ "([a-z0-9_]+)", "([A-Z0-9]+\.ASM)",',
        gen.read_text(), re.M))) if gen.exists() else []
    claimed = sorted(set(hand) | set(generated))

    for name, fname in claimed:
        # It must be a table entry -- a monitor nothing spawns is not a
        # monitor.
        assert name in names, name
        # ...and the file it says it lives in must really define it, as
        # a bare column-0 label (which all of them are; that is why the
        # coverage tool cannot see them).
        src = (wlanim.ORIG / fname).read_text(errors="replace")
        assert re.search(rf"^{re.escape(name)}\b", src, re.M) or \
               re.search(rf"^\s+SUBRP?\s+{re.escape(name)}\b", src, re.M), \
               (name, fname)

    unported = [n for n in names if n not in {c[0] for c in claimed}]
    assert unported == [], unported

    # And the families really are four, not one reader stretched over
    # sixty routines: each of the three new tables has to be there and
    # to be the size the census says.
    text = gen.read_text()
    for table, count in (("wm_smove_hdhold", 40), ("wm_smove_charge", 6),
                         ("wm_smove_grab", 8), ("wm_smove_free", 6)):
        body = text.split("%s[] = {" % table, 1)[1].split("\n};", 1)[0]
        rows = re.findall(r'^\s*\{ "([a-z0-9_]+)", "', body, re.M)
        assert len(rows) == count, (table, len(rows))


def test_smove_tables_honour_the_finishing_move_switch() -> None:
    """Each wrestler's secret-move process list, as ASSEMBLED.

    Every one of the eight tables wraps its finishing-move entries in
    `.if NUM_<name>_FINISHES`, and GAME.EQU:580-587 sets seven of the
    eight switches to 0 -- Undertaker's is the only 1, and his `> 1`
    guard still excludes und_finish_move2. So the shipped game has
    exactly ONE finishing move, und_finish_move1, and the other fifteen
    *_finish_move1/2 routines are source text the assembler skipped
    (their SUBR definitions are inside the same `.if` blocks).

    Reading the table as plain text misses all of that, which is what
    the port's eight hand-written copies did: eleven entries between
    them for moves the arcade never spawned.
    """
    if not (wlanim.ORIG / "WRESTLE2.ASM").exists():
        return

    tables = wlwrestlertbl.smove_tables()
    assert len(tables) == 9, len(tables)
    by_name = {e[0]: e[1] for e in tables if e}

    # Adam Bomb's slot is a plain 0 here -- unlike the anim dispatch
    # lists, which hand him Doink's tables.
    assert tables[7] is None

    # Exactly one finishing move in the whole roster.
    finishes = [lab for rows in by_name.values() for lab in rows
                if "_finish_move" in lab]
    assert finishes == ["und_finish_move1"], finishes

    # And the switch really is what decides: flipping the reading to
    # ignore `.if` would put the other fifteen back.
    consts = wlwrestlertbl._constants()
    assert consts["NUM_TAKER_FINISHES"] == 1
    for other in ("BRET", "BAM", "YOKO", "DOINK", "RAZOR", "LEX", "SHAWN"):
        assert consts[f"NUM_{other}_FINISHES"] == 0, other

    # A condition the tool cannot decide is refused, not assumed true.
    try:
        wlwrestlertbl._cond("SOMETHING_UNKNOWN", consts)
    except ValueError:
        pass
    else:
        assert False, "_cond accepted an unknown symbol"

    # The in-tree copies match, wrestler for wrestler.
    port = {
        "hrt_smove_table": ("src/core/arcade/wm_arcade_wrestler_port.c",
                            "bret_smove"),
        "rzr_smove_table": ("src/core/arcade/wm_arcade_wrestler_port.c",
                            "razor_smove"),
        "und_smove_table": ("src/core/arcade/wm_arcade_taker.c",
                            "special_processes"),
        "yok_smove_table": ("src/core/arcade/wm_arcade_yoko.c",
                            "special_processes"),
        "shn_smove_table": ("src/core/arcade/wm_arcade_shawn.c",
                            "special_processes"),
        "bam_smove_table": ("src/core/arcade/wm_arcade_bam.c",
                            "special_processes"),
        "dnk_smove_table": ("src/core/arcade/wm_arcade_doink.c",
                            "special_processes"),
        "lex_smove_table": ("src/core/arcade/wm_arcade_lex.c",
                            "special_processes"),
    }
    for table, (rel, sym) in port.items():
        path = ROOT / rel
        if not path.exists():
            continue
        text = path.read_text()
        m = re.search(re.escape(sym) + r"\[\]\s*=\s*\{(.*?)\n\};",
                      text, re.S)
        assert m, (rel, sym)
        got = re.findall(r'"([A-Za-z0-9_]+)"', m.group(1))
        assert got == by_name[table], (rel, sym, got, by_name[table])


def test_coverage_does_not_count_code_the_assembler_skipped() -> None:
    """A SUBR inside a false `.if` is not an untranslated routine.

    It is in the FILE, so an enumeration that greps SUBR lines counts
    it, but the object never held it and nothing can call it. Reporting
    it as open inflates the denominator with work that does not exist:
    77 routines in the linked source are like this.

    The direction of the doubt matters. A condition the tool cannot
    decide leaves the routine alone -- assumed assembled, still counted
    -- so an unreadable `.if` can only ever make coverage look worse
    than it is, never better.
    """
    if not (wlanim.ORIG / "GAME.EQU").exists():
        return

    skipped = port_coverage.unassembled_routines()
    consts = port_coverage.equ_constants()

    # The switches that decide most of them, read not assumed.
    assert consts["DEBUG"] == 0                    # SYS.EQU:13
    assert consts["NUM_TAKER_FINISHES"] == 1       # GAME.EQU:586
    assert consts["NUM_BRET_FINISHES"] == 0        # GAME.EQU:580

    # Every wrestler's finishing moves are skipped except Undertaker's
    # first -- and his second is skipped by the `> 1` guard.
    for pre in ("hrt", "rzr", "yok", "shn", "bam", "dnk", "lex"):
        assert f"{pre}_finish_move1" in skipped, pre
        assert f"{pre}_finish_move2" in skipped, pre
    assert "und_finish_move1" not in skipped
    assert "und_finish_move2" in skipped

    # `.if DEBUG` catches SPECIAL.ASM's development helpers.
    assert "CHANGE_SKIRTS" in skipped
    assert ".if DEBUG" in skipped["CHANGE_SKIRTS"]

    # An undecidable condition must NOT mark anything skipped.
    assert port_coverage._cond_value("SOME_UNKNOWN_SYMBOL", consts) is None
    assert port_coverage._cond_value("0", consts) is False
    assert port_coverage._cond_value("1", consts) is True

    # And a routine that is plainly in the game is not caught.
    for live in ("wrestler_veladd", "change_walk_anim", "adjust_health"):
        assert live not in skipped, live

    # The classification uses it, and `unassembled` is never also
    # claimed by the ledger -- the two would be answering the same
    # question twice.
    data = port_coverage.classify()
    rows = {r["name"]: r for r in data["routines"]}
    assert rows["hrt_finish_move2"]["status"] == "unassembled"
    ledger = port_coverage.load_ledger()
    for name in skipped:
        assert name not in ledger, name


def test_the_two_bmod_producers_agree() -> None:
    """src/generated/bmod_tables.c has two producers; they must match.

    scripts/prepare_frontend_assets.sh and scripts/prepare_select_assets.sh
    both run tools/bmod_source.py against the same output file, and
    bmod_source numbers its bmod_words_N arrays by the order the modules
    arrive in. So a difference in the two `--module` lists is not a
    harmless style difference: whichever script ran last renumbers the
    arrays, the checked-in file stops reproducing, and every regeneration
    produces a spurious diff.

    They DID differ -- LADDERBMOD and wwfselbkBMOD were swapped -- which
    is how this was found: running the frontend script rewrote a file
    nothing had asked it to change, with identical data under different
    indices.
    """
    scripts = [ROOT / "scripts" / "prepare_frontend_assets.sh",
               ROOT / "scripts" / "prepare_select_assets.sh"]
    if not all(p.exists() for p in scripts):
        return

    lists = []
    for path in scripts:
        text = path.read_text()
        # The bmod_source.py invocation and the --module lines under it.
        at = text.index("bmod_source.py")
        chunk = text[at:]
        end = chunk.index("--out")
        lists.append((path.name, re.findall(r"--module\s+(\w+)",
                                            chunk[:end])))

    (name_a, mods_a), (name_b, mods_b) = lists
    assert mods_a, name_a
    assert mods_a == mods_b, (name_a, mods_a, name_b, mods_b)

    # And the checked-in file is in that order, so it reproduces.
    out = ROOT / "src" / "generated" / "bmod_tables.c"
    if out.exists():
        order = re.findall(r'\{"(\w+)", \{', out.read_text())
        assert order == mods_a, (order, mods_a)


def test_extracted_cut_content_says_so() -> None:
    """A generated file for a routine the assembler skipped is labelled.

    src/generated/finish_sequences.c holds FINISEQ.ASM's
    hrt_finish1_move, and GAME.EQU:580 sets NUM_BRET_FINISHES to 0 --
    the routine is inside that `.if`, so Bret has no finishing move in
    the shipped game. Extracting it is fine; leaving it unlabelled is
    not, because a generated file with no note reads as translated
    behaviour.

    The banner is decided by port_coverage from the `.if` conditions
    rather than written by hand, so it tracks the switch instead of
    going stale.
    """
    if not (wlanim.ORIG / "FINISEQ.ASM").exists():
        return

    assert "hrt_finish1_move" in port_coverage.unassembled_routines()

    note = asmseq._unassembled_note("hrt_finish1_move")
    assert "NOT IN THE SHIPPED GAME" in note
    assert "NUM_BRET_FINISHES" in note

    # A routine that IS assembled gets no banner.
    assert asmseq._unassembled_note("wrestler_veladd") == ""

    out = ROOT / "src" / "generated" / "finish_sequences.c"
    if out.exists():
        assert "NOT IN THE SHIPPED GAME" in out.read_text()


def test_sprite_banks_can_be_bundled_per_wrestler() -> None:
    """tools/bret_bundle.py can bundle any wrestler, not just Bret.

    It was written around one wrestler: one .LOD, one output, symbols
    named for him. The other seven need their own banks, which needs
    two things -- a symbol prefix so eight of them do not collide, and
    a way to keep each bundle to the frames its own .LOD owns.

    This checks the capability and the ownership it relies on. It does
    NOT bundle anything: one wrestler is 3.5-4.5 MiB of CI8 pixels and
    tens of megabytes of C (see docs/N64_FIRST.md, which records why
    that rules out holding even two resident).
    """
    img = wlanim.ORIG / "IMG"
    if not img.is_dir():
        return

    sig = inspect.signature(bret_bundle.emit)
    assert "prefix" in sig.parameters
    assert "only_mapped" in sig.parameters

    # Frames belong to their own wrestler's container, which is what
    # lets --only-mapped split the corpus eight ways.
    bret = bret_manifest.parse_lod(img / "BRET.LOD")
    yoko = bret_manifest.parse_lod(img / "YOKO.LOD")
    assert "H2WL1A01" in bret and "H2WL1A01" not in yoko
    assert "Y3GS3A03" in yoko and "Y3GS3A03" not in bret


def test_waithitopp_is_a_mode_and_a_frame() -> None:
    """ANIM.ASM:2300's own note: "just like an ordinary WL ticks,frame type
    command except that the ANICNT is zeroed if we hit the opponent." The
    handler only sets MODE_WAITHITOPP and hands the operands back to the
    dispatcher, so one source line is two ops. The frame was always being
    read (wlanim's WAIT_FRAME_RE); the mode was what got dropped.
    """
    src = ROOT / "original" / "wwf-wrestlemania" / "HRTSEQ3.ASM"
    if not src.exists():
        return

    ops = wlprogram.program_for(src, "hrt_3_pile_driver_anim")
    kinds = [o[0] for o in ops]
    for i, k in enumerate(kinds):
        if k == "WAITHITOPP":
            assert kinds[i + 1] == "FRAME", (i, kinds[i:i + 3])
    # HRTSEQ3.ASM:317 is `WWL ANI_WAITHITOPP,4,H3HT3X+FR3`.
    text = src.read_text(errors="replace")
    assert "ANI_WAITHITOPP,4,H3HT3X+FR3" in text.replace("\r", "")


def test_command_table_ops_keep_their_operands() -> None:
    """Every op from wlcommands' table must reach the C with its operands.

    tools/wlprogram.py's MOTION_OPS decides which ops get their (mode, a,
    b, c) written out. It used to be a hand-kept set, so an op added to the
    command table but not to that set fell through to the no-operand
    default and was emitted with its operands silently zeroed -- which is
    what happened to ANI_BOUNCE (its upward kick became 0) and ANI_GETUP
    (its GETUP_TIME became 0). The set is derived from the table now, and
    this checks it stays that way.
    """
    kinds = {k for k, _n, _m in wlcommands.COMMANDS.values()}
    assert kinds <= wlprogram.MOTION_OPS, kinds - wlprogram.MOTION_OPS

    base = ROOT / "original" / "wwf-wrestlemania"
    generated = ROOT / "src" / "generated" / "anim_programs.c"
    if not (base / "HRTSEQ4.ASM").exists() or not generated.exists():
        return

    # hrt_fall_back_anim's ANI_BOUNCE,5 and hrt_tossed_anim's
    # ANI_GETUP,STAY_TIME (270), both read straight out of the source.
    ops = wlprogram.program_for(base / "HRTSEQ4.ASM", "hrt_fall_back_anim")
    assert [o for o in ops if o[0] == "BOUNCE"][0][2] == 5
    ops = wlprogram.program_for(base / "HRTSEQ2.ASM", "hrt_tossed_anim")
    assert [o for o in ops if o[0] == "GETUP"][0][2] == 270

    text = generated.read_text()
    assert "{ WM_AOP_BOUNCE, 0, -1, 5," in text, "ANI_BOUNCE lost its operand"
    assert "{ WM_AOP_GETUP, 0, -1, 270," in text, "ANI_GETUP lost its operand"


def test_roll_tables() -> None:
    """WRESTLE2.ASM:1290 do_roll's per-wrestler roll tables.

    Every playable wrestler has one, they all share the same speed and z
    velocity, and -- the check that matters -- the largest index the
    multiplier can produce from a 0..255 ROLL_POS is inside the frame list,
    so do_roll can never read past the end of one.
    """
    base = ROOT / "original" / "wwf-wrestlemania"
    if not (base / "WRESTLE2.ASM").exists():
        return

    rows = wlroll.tables()
    assert len(rows) == 9, len(rows)
    assert rows[7] is None, "slot 7 is Adam Bomb's `.long 0`"
    for r in rows:
        if not r:
            continue
        assert r["speed"] == 7, r
        assert r["zvel"] == 0x50000, r
        assert (255 * r["multiplier"]) >> 16 < len(r["frames"]), r

    # Bret's list is not in ascending frame order: it runs FR1, then FR13
    # down to FR2, because that is the direction his artwork rolls.
    hrt = rows[0]
    assert hrt["frames"][0] == "H3RL1A01", hrt["frames"][:3]
    assert hrt["frames"][1] == "H3RL1A13", hrt["frames"][:3]
    assert hrt["frames"][2] == "H3RL1A12", hrt["frames"][:3]

    # The multiplier is written `10000h*12/255` and the assembler's divide
    # truncates, so 255 reaches index 11 and the thirteenth frame is
    # unreachable. That is the shipped data, not an extraction error.
    assert hrt["multiplier"] == 3084, hrt["multiplier"]
    assert hrt["top"] == 11 and len(hrt["frames"]) == 13, hrt


def test_self_contained_command_ops() -> None:
    """The batch of state commands with no subsystem behind them.

    ANI_FACE's operand is a direction written as a bit set
    (`MOVE_LEFT|MOVE_UP`), which is why operand parsing had to learn `|`;
    ANI_SETWORD names exactly three fields across the roster, and a fourth
    would be a silent write to whatever sits at that offset.
    """
    base = ROOT / "original" / "wwf-wrestlemania"
    if not (base / "HRTSEQ3.ASM").exists():
        return

    assert wlcommands._value("MOVE_LEFT|MOVE_UP", {}) == (
        wlanim.GLOBAL_EQU["MOVE_LEFT"] | wlanim.GLOBAL_EQU["MOVE_UP"])

    assert set(wlprogram.SETWORD_FIELDS) == {"USR_VAR1", "USR_VAR2",
                                             "DELAY_METER"}

    # Every one of these has to survive into the generated C with its
    # operands, the way ANI_BOUNCE and ANI_GETUP did not.
    generated = ROOT / "src" / "generated" / "anim_programs.c"
    if not generated.exists():
        return
    text = generated.read_text()
    for op in ("FACE", "GRAVITY_OFF", "DAMAGE", "SETOPP_PLYRMODE",
               "OPP_GETUP", "ATTACHVEL", "SETWORD", "IFNOT_RPTCOUNT"):
        assert f"WM_AOP_{op}," in text, op
    # ...and the ones carrying an operand are not all zeroes.
    for op in ("FACE", "DAMAGE", "SETWORD"):
        rows = [l for l in text.splitlines() if f"WM_AOP_{op}," in l]
        assert any(re.search(r"-1, (?!0,)", l) for l in rows), op


def test_per_wrestler_aux_tables() -> None:
    """ANI_CHANGEANIM_TBL, ANI_XFLIP_TBL and ANI_OPPOFFSET's tables.

    Same family as the puppet and slave tables, and the same trap: the
    labels are `#local` and reused across files, so they resolve per use
    site rather than by name.
    """
    base = ROOT / "original" / "wwf-wrestlemania"
    if not (base / "ANIM.ASM").exists():
        return

    ca = wlpuppet.aux_table_ids("changeanim")
    xf = wlpuppet.aux_table_ids("xflip")
    oo = wlpuppet.aux_table_ids("oppoffset")
    # Five, not four: REACT1.ASM's xxx_aborted_attach_anim ends
    # `ANI_CHANGEANIM_TBL,#getup_tbl`, and canonical_files() used to be
    # selected by filename (*SEQ*.ASM) rather than by whether a file
    # writes one of these ops. That table had no id, so program_for
    # refused the animation outright and it was never emitted.
    assert len(ca) == 5, len(ca)
    assert len(xf) >= 15, len(xf)
    assert len(oo) >= 15, len(oo)

    # Every row list is exactly the nine roster slots.
    for kind in ("changeanim", "xflip", "oppoffset"):
        for path in wlpuppet.canonical_files():
            for rows in wlpuppet._aux_tables_in(path, kind).values():
                assert len(rows) == wlpuppet.ROSTER_SLOTS, (kind, rows)

    # `#xflip_tbl` is defined in more than one file with different
    # contents, which is exactly why resolution is per use site.
    seen = {}
    for path in wlpuppet.canonical_files():
        for key, rows in wlpuppet._aux_tables_in(path, "xflip").items():
            seen[key] = rows
    assert len({tuple(v[0] for v in r) for r in seen.values()}) > 1, \
        "every xflip table came out identical -- resolution collapsed"

    # ANI_CHANGEANIM_TBL names whole animations, so like the slave targets
    # every one of them has to be emitted.
    generated = ROOT / "src" / "generated" / "anim_programs.c"
    if generated.exists():
        text = generated.read_text()
        for _path, label in wlpuppet.changeanim_targets():
            assert f'"{label}"' in text, label


def test_anim_code_registry_reaches_its_call_sites() -> None:
    """Every ANI_CODE registry row must match a real call site, exactly.

    A `#`-prefixed label is file-scoped and the emitter carries the `#`
    into the generated op, so a row spelled without it never matches and
    the routine is simply never called. That is not hypothetical: thirteen
    rows shipped that way, and their unit tests passed because they called
    wm_anim_code_run with the un-prefixed name directly -- testing the row
    rather than the wiring. This checks the spelling against the source.
    """
    base = ROOT / "original" / "wwf-wrestlemania"
    src = ROOT / "src" / "core" / "anim_code.c"
    if not (base / "ANIM.ASM").exists() or not src.exists():
        return

    rows = re.findall(r'\{\s*"(#?[A-Za-z_0-9]+)"\s*,\s*(NULL|"[^"]+")',
                      src.read_text())
    assert rows, "no registry rows found -- the pattern stopped matching"

    use = re.compile(r"^\s*(?:\.word|W+L+W*)\s+ANI_CODE\s*,\s*(#?\w+)", re.I)
    # The per-wrestler sequence files hold nearly every ANI_CODE call, but
    # not all of them: REACT1.ASM:1884 puts one in xxx_aborted_attach_anim.
    # Any file a registry row names is scanned too, so a row cannot claim
    # a file this never looks at.
    scan = {q for who in ("HRT", "RZR", "UND", "YOK", "SHN", "BAM", "DNK",
                          "LEX")
            for q in base.glob(who + "SEQ*.ASM") if "'" not in q.name}
    for _name, where in rows:
        if where != "NULL":
            scan.add(base / where.strip('"'))

    sites = {}
    for p in sorted(scan):
        if not p.exists():
            continue
        for raw in p.read_text(errors="replace").splitlines():
            m = use.match(wlanim.strip_comment(raw))
            if m:
                sites.setdefault(m.group(1), set()).add(p.name)

    unreachable = []
    for name, where in rows:
        if name not in sites:
            unreachable.append((name, where, "no call site spells it this way"))
            continue
        if where != "NULL":
            f = where.strip('"')
            if f not in sites[name]:
                unreachable.append((name, where, "not called from that file"))
    assert not unreachable, unreachable



def test_a_local_routine_defined_twice_gets_two_rows() -> None:
    """The same `#name`, twice in one file, with a different body each.

    This is the file-scoping trap one level deeper, and it shipped:
    BAMSEQ2.ASM defines `#set_zvel2` at 4287 as -5.0 and again at 4371 as
    +5.0 -- opposite directions -- and UNDSEQ3.ASM defines `#inc_loop` at
    168 capped at 3 and at 2888 capped at 2. Keying a registry row on
    (name, file) can only be one of each pair, so half the call sites in
    every affected file were running the wrong routine.

    So a call site now carries the source line of the definition it
    resolves to, and this checks three things: that every registry row's
    line really is that label in that file, that a file which defines a
    translated local name more than once has a row for EVERY definition,
    and that the emitter resolved every local call site to some
    definition.
    """
    base = wlanim.ORIG
    src = ROOT / "src" / "core" / "anim_code.c"
    if not (base / "ANIM.ASM").exists() or not src.exists():
        return

    row_re = re.compile(
        r'^\s*\{\s*"(#?[A-Za-z_0-9]+)"\s*,\s*(NULL|"[^"]+")\s*,'
        r'.*,\s*(-?\d+)\s*\},\s*$', re.M)
    rows = row_re.findall(src.read_text())
    assert rows, "no registry rows found -- the pattern stopped matching"

    lines_of = {}

    def file_lines(name):
        if name not in lines_of:
            lines_of[name] = [r.split(";", 1)[0]
                              for r in (base / name).read_text(
                                  errors="replace").splitlines()]
        return lines_of[name]

    # 1. Every numbered row names that exact label at that exact line.
    numbered = {}
    for name, where, line in rows:
        line = int(line)
        if not line:
            continue
        assert where != "NULL", (name, line)
        f = where.strip('"')
        assert line in wlanim.local_label_defs(file_lines(f), name), \
            (name, f, line)
        numbered.setdefault((name, f), set()).add(line)

    # 2. A translated local name defined more than once in its file needs
    #    a row per definition -- otherwise the definitions the rows do not
    #    cover fall through to nothing. This looks at EVERY row, numbered
    #    or not: an unnumbered row for a name with two definitions is the
    #    exact shape of the defect.
    covered = {}
    for name, where, line in rows:
        if where == "NULL" or not name.startswith("#"):
            continue
        covered.setdefault((name, where.strip('"')), set()).add(int(line))
    for (name, f), have in sorted(covered.items()):
        defs = set(wlanim.local_label_defs(file_lines(f), name))
        if len(defs) <= 1:
            continue
        assert have == defs, (name, f, "rows", sorted(have),
                              "definitions", sorted(defs))

    # 3. And the emitter resolved every local call site to a definition:
    #    a 0 there would silently fall back to whichever row came first.
    generated = ROOT / "src" / "generated" / "anim_programs.c"
    if generated.exists():
        code = re.findall(r'WM_AOP_CODE, 0, -1, (-?\d+),[^"]*"(#?[^"]+)"',
                          generated.read_text())
        assert code, "no WM_AOP_CODE rows in the generated programs"
        unresolved = sorted({n for a, n in code
                             if n.startswith("#") and a == "0"})
        assert not unresolved, unresolved
        # ...and every global one carries 0, since it has no local scope.
        assert not [n for a, n in code
                    if not n.startswith("#") and a != "0"]


def test_the_emitter_skips_nothing() -> None:
    """Every animation command in the roster is emitted, not dropped.

    A skipped command is silent in the generated C -- the frame order
    still comes out right, because order comes from the branches, so a
    dropped command looks like nothing at all until you go looking for
    the behaviour it was meant to cause. The emitter counts them for
    exactly that reason, and this asserts the count is zero.

    It reached zero in this commit. If it stops being zero this names the
    command, which is the whole point of counting.
    """
    if not (wlanim.ORIG / "ANIM.ASM").exists():
        return
    wlprogram.SKIPPED.clear()
    wlprogram.SKIPPED_WHERE.clear()
    n = 0
    for src in wlanim.linked_files():
        if "SEQ" not in src.name or src.name.startswith("FINI"):
            continue
        lines = [wlanim.strip_comment(r)
                 for r in src.read_text(errors="replace").splitlines()]
        for line in lines:
            m = wlanim.SUBR_RE.match(line)
            if not m:
                continue
            try:
                wlprogram.program_for(src, m.group(1))
                n += 1
            except ValueError:
                # A routine the emitter legitimately declines (a branch it
                # cannot follow) is a different question, checked by
                # test_wlprogram_roster_wide.
                pass
    assert n > 1000, n
    skipped = dict(wlprogram.SKIPPED)
    assert not skipped, sorted(skipped.items(), key=lambda kv: -kv[1])


def test_every_ani_code_call_site_resolves() -> None:
    """The other direction: every CALL SITE must reach a registry row.

    test_anim_code_registry_reaches_its_call_sites checks that no row is
    spelled in a way nothing calls. This checks that nothing called is
    left unanswered -- resolved the way wm_anim_code_run resolves it, so
    a routine translated for one file does not count as translated for a
    file it was never written in, and a local definition without a row
    does not hide behind a sibling that has one.

    ANI_CODE is at 100% as of this commit. If that ever stops being true
    this test says which routine, rather than a percentage quietly
    slipping in a report nobody reads.
    """
    if not (wlanim.ORIG / "ANIM.ASM").exists():
        return
    uses = ani_code_census.call_sites()
    known = ani_code_census.covered()
    gaps = sorted(k for k in uses if not ani_code_census.resolves(k, known))
    assert not gaps, gaps
    assert sum(uses.values()) == 2303, sum(uses.values())


def test_target_offsets_grid() -> None:
    """TABLES.ASM's target grid, checked against the source a second way.

    set_target_offsets indexes it `WRESTLERNUM * 5 + area`, three words
    each, into `#mode_table[PLYRMODE]`. get_mode_height reads the SAME
    grid at `15n + 1` words -- which is wrestler n's head Y, i.e. area 0's
    Y -- so the two agree only if the row width really is five areas of
    three words. That agreement is the check: it would break if the
    extractor ever read the grid at the wrong stride.
    """
    if not (wlanim.ORIG / "TABLES.ASM").exists():
        return
    table = wltarget.mode_table()
    blocks = wltarget.blocks()

    assert len(table) == wltarget.MODE_SLOTS, len(table)
    assert set(table) <= set(blocks), sorted(set(table) - set(blocks))

    for name, grid in blocks.items():
        assert len(grid) == wltarget.ROSTER_SLOTS, (name, len(grid))
        for w, areas in enumerate(grid):
            assert len(areas) == wltarget.AREAS, (name, w, len(areas))

    # Every block collapses to one of four distinct ones, and PLYRMODE 17,
    # 18, 22 and 23 have no block of their own -- the table points them at
    # mode_normal, which is the source's own choice, not a fallback here.
    distinct = {tuple(tuple(a) for a in blocks[n]) for n in table}
    assert len(distinct) == 4, len(distinct)
    for slot in (17, 18, 22, 23):
        assert table[slot] == "mode_normal", (slot, table[slot])

    # get_mode_height's own index: `15n + 1` WORDS into the block. Walk the
    # rows flat and check that word is area 0's Y for every wrestler, in
    # every block.
    for name, grid in blocks.items():
        flat = [v for areas in grid for xyz in areas for v in xyz]
        for w in range(wltarget.ROSTER_SLOTS):
            assert flat[15 * w + 1] == grid[w][0][1], (name, w)

    # Slot 7 is Adam Bomb, cut from the game: every block writes him as
    # literal zeroes rather than borrowing somebody else's body.
    for name, grid in blocks.items():
        assert all(xyz == (0, 0, 0) for xyz in grid[7]), name

    # ...and nobody else is all-zero, which is what a mis-read block would
    # look like.
    for name, grid in blocks.items():
        for w in range(wltarget.ROSTER_SLOTS):
            if w == 7:
                continue
            assert any(v for xyz in grid[w] for v in xyz), (name, w)

    # A head is above a chest is above a groin is above the knees is above
    # the feet, for every real wrestler in the standing block. This is what
    # makes ANI_TARGET's "higher index is nearer his feet" reading true.
    for w in range(wltarget.ROSTER_SLOTS):
        if w == 7:
            continue
        ys = [blocks["mode_normal"][w][a][1] for a in range(wltarget.AREAS)]
        assert ys == sorted(ys, reverse=True), (w, ys)


def test_target_tables_generate_the_shipped_file() -> None:
    out = ROOT / "src" / "generated" / "target_tables.c"
    if not (wlanim.ORIG / "TABLES.ASM").exists() or not out.exists():
        return
    assert wltarget.render_c() == out.read_text()


def test_announce_tables() -> None:
    """DCSSOUND.ASM's announcer tables, against their own headers.

    Every table writes its shape in three values immediately BEFORE its
    label, which ADD_TO_QUEUE reads at negative offsets: a reset-repeat
    flag at -050H, a crowd table at -040H, and the last row index and the
    row stride at -020H/-010H. The stride is a TMS34010 BIT count, so
    010H is one word. If the extractor ever reads those in the wrong
    order it will silently produce a table that draws the wrong rows, so
    each one is checked against the data that follows it.
    """
    if not (wlanim.ORIG / "DCSSOUND.ASM").exists():
        return
    tables = wlvoice.announce_tables()
    calls = wlvoice.callers()
    assert len(tables) >= 20, len(tables)

    for name, t in tables.items():
        assert t["stride"] in (1, 2), (name, t["stride"])
        rows = t["rows"]
        assert all(len(r) == t["stride"] for r in rows), name
        # RNDRNG0's maximum has to be inside the data.
        assert t["last_index"] < len(rows), (name, t["last_index"], len(rows))

    # ARE_WE_REPEATING's walk-forward runs off the end of the drawn range,
    # so every table a rejected draw can reach the end of carries rows
    # past it. Mostly those repeat lines from the table's own opening, but
    # not always: FACE_HIT, MID_HIT, MISSES and MISS_YOKO each put a line
    # there that appears nowhere in the drawn range, so the ONLY way the
    # game ever says it is through the anti-repeat walk. They are part of
    # the table for that reason, and are extracted with it. (The one-line
    # *_FINISHES tables have no such rows: there is nowhere to walk to.)
    for name in sorted({c["table"] for c in calls.values()}):
        t = tables[name]
        assert t["rows"][t["last_index"] + 1:], name

    # The end-of-match family is the exception, and deliberately so: every
    # row of MATCH_OVER's *_FINISHES tables is drawable and there is
    # nothing after them. Several hold a single line, so a wrestler who
    # just said his own line fails ARE_WE_REPEATING and the walk runs
    # straight off the end -- into whatever the assembler put next. This
    # port stops there and says nothing instead.
    for name in ("HART_FINISHES", "BAM_FINISHES", "MATCH_OVER"):
        t = tables[name]
        if name.endswith("_FINISHES"):
            assert not t["rows"][t["last_index"] + 1:], name
        else:
            assert t["rows"][t["last_index"] + 1:], name
    # Two wrestlers win in silence: their table is a single zero.
    assert tables["UNDERTAKER_FINISHES"]["rows"] == [[0]]
    assert tables["YOKO_FINISHES"]["rows"] == [[0]]

    # Every CALL_x names a table that exists, with a real percentage.
    for name, c in calls.items():
        assert c["table"] in tables, (name, c["table"])
        assert 0 < c["percent"] <= 1000, (name, c["percent"])
        assert 0 <= c["sleep"] <= 60, (name, c["sleep"])

    # DCSSOUND.ASM:3282 PROC_MISSES, read straight off the source.
    assert calls["CALL_MISSES"]["table"] == "MISSES"
    assert calls["CALL_MISSES"]["sleep"] == 5
    assert calls["CALL_MISSES"]["percent"] == 350
    # It never copies WRESTLERNUM into A5, and CALL_SPECIAL_MOVE does.
    assert calls["CALL_MISSES"]["personal"] is False
    assert calls["CALL_SPECIAL_MOVE"]["personal"] is True

    # The five per-wrestler tables all have the cut wrestler's zero slot.
    personal = wlvoice.personal_tables()
    for want, rows in personal.items():
        assert len(rows) == 9, want
        assert rows[7] == 0, want
        assert all(v > 0 for i, v in enumerate(rows) if i != 7), want

    # ASCENDING_TABLE climbs as REPEAT_STATE counts down 3, 2, 1, 0.
    for i, row in enumerate(wlvoice.ascending_table()):
        if i == 7:
            assert row == [0, 0, 0, 0]
            continue
        assert row == sorted(row, reverse=True), (i, row)


def test_crowd_tables_come_out_of_the_source() -> None:
    """DCSSOUND.ASM:4443's CROWD TABLES, read the way DO_CROWD_ANYWAY does.

    A one-value header (`.WORD n`) before the label and four words a row:
    sound, duration, crowd_cheer flags, RNDPER percentage. The routine
    picks a row with `SLL 6,A0`, which is four 16-bit words, so a row that
    came out any other width would put the draw off the end of the table.
    """
    if not (wlanim.ORIG / "DCSSOUND.ASM").exists():
        return
    crowd = wlvoice.crowd_tables()
    # The eight the source writes under its own "*CROWD TABLES" banner.
    assert set(crowd) == {
        "SETUP_TABLE", "CRESCENDO_TABLE", "ROPES_CHEER", "CROWD_FAIL",
        "CROWD_SPECIAL", "CROWD_CHEER", "CROWD_THROWN", "CROWD_ORDINARY",
    }, sorted(crowd)

    c_long, c_override, c_random = 1, 2, 4
    for name, t in crowd.items():
        assert t["last_index"] < len(t["rows"]), (name, t)
        for row in t["rows"]:
            assert len(row) == 4, (name, row)
            sound, ticks, flags, percent = row
            # SOUND.EQU's crowd block; the durations are D_CROWD_* ticks.
            assert 2048 <= sound <= 2100, (name, sound)
            assert 0 < ticks < 512, (name, ticks)
            assert flags & ~(c_long | c_override | c_random) == 0, (name, flags)
            # crowd_cheer reads A4 only when B_RANDOM is set, and the
            # source leaves the column 0 on every row that does not.
            if flags & c_random:
                assert 0 < percent <= 1000, (name, percent)
            else:
                assert percent == 0, (name, percent)

    # Every crowd `.LONG` an announcer table carries names one of them.
    named = {t["crowd"] for t in wlvoice.announce_tables().values()
             if t["crowd"]}
    assert named and named <= set(crowd), sorted(named - set(crowd))

    # ...and the one table with a single row is the one the source wrote
    # with a `.WORD 0` header, so a draw can only ever land on row 0.
    assert crowd["CRESCENDO_TABLE"]["last_index"] == 0
    assert len(crowd["CRESCENDO_TABLE"]["rows"]) == 1


def test_announce_tables_generate_the_shipped_file() -> None:
    """The checked-in generated file is what the tool produces today."""
    out = ROOT / "src" / "generated" / "announce_tables.c"
    if not (wlanim.ORIG / "DCSSOUND.ASM").exists() or not out.exists():
        return
    assert wlvoice.render_c() == out.read_text()


def test_announce_calls_are_spelled_as_the_call_sites_spell_them() -> None:
    """The same guard the ANI_CODE registry has, for the announcer group.

    These routines resolve by name out of wm_announce_calls[] rather than
    from a hand-kept row, so the spelling comes from DCSSOUND.ASM itself.
    That only helps if the sequence files spell them the same way, which
    is what this checks -- and it is how the group's real reach is known.
    """
    base = ROOT / "original" / "wwf-wrestlemania"
    if not (base / "ANIM.ASM").exists():
        return
    use = re.compile(r"^\s*(?:\.word|W+L+W*)\s+ANI_CODE\s*,\s*(#?\w+)", re.I)
    sites: dict[str, int] = {}
    for who in ("HRT", "RZR", "UND", "YOK", "SHN", "BAM", "DNK", "LEX"):
        for p in sorted(q for q in base.glob(who + "SEQ*.ASM")
                        if "'" not in q.name):
            for raw in p.read_text(errors="replace").splitlines():
                m = use.match(wlanim.strip_comment(raw))
                if m:
                    sites[m.group(1)] = sites.get(m.group(1), 0) + 1

    calls = wlvoice.callers()
    reached = {n: sites[n] for n in calls if n in sites}
    # CALL_MISSES alone is the biggest single ANI_CODE routine in the game.
    assert reached.get("CALL_MISSES", 0) >= 150, reached.get("CALL_MISSES")
    assert sum(reached.values()) >= 300, sum(reached.values())
    # The callers that are NOT reached from an animation are reached from
    # the wrestler control layer instead (CALL_DROP_KICK, CALL_FACE_HIT,
    # CALL_MID_HIT, CALL_AVERAGE_MOVE, DO_REVERSAL) -- they are extracted
    # because they share the tables, not because a sequence file calls
    # them, so this only requires that the ones that ARE reached match.
    assert reached, "no announcer routine is reached from any animation"



def test_wrsnd_tables() -> None:
    """DCSSOUND.ASM's per-wrestler sound grid, against its own shape.

    MASTER_SOUND_TABLE is written as nine blocks of `.word` rows with only
    a comment (";Bret Hart\t00") separating them, so nothing in the file
    marks where one wrestler ends and the next begins -- the extractor
    relies entirely on LAST_MOVE+1. That is checked here by counting the
    words in each commented block independently and requiring the two
    readings to agree.
    """
    src = wlanim.ORIG / "DCSSOUND.ASM"
    if not src.exists():
        return

    master = wlwrsnd.master_table()
    default = wlwrsnd.default_table()
    assert len(default) == wlwrsnd.MOVES_PER_WRESTLER
    assert len(master) == wlwrsnd.WRESTLERS
    assert all(len(r) == wlwrsnd.MOVES_PER_WRESTLER for r in master)

    # The independent reading: count words between the ";<name> NN"
    # comments that head each wrestler's block.
    lines = [r.rstrip() for r in src.read_text(errors="replace").splitlines()]
    start = next(i for i, l in enumerate(lines)
                 if l.strip() == "MASTER_SOUND_TABLE")
    counts, cur, n = [], None, 0
    for q in range(start + 1, len(lines)):
        raw = lines[q]
        head = re.match(r"^\s*;(\w[^\d]*)\s*(\d\d)\s*$", raw)
        if head:
            if cur is not None:
                counts.append((cur, n))
            cur, n = head.group(1).strip(), 0
            continue
        body = wlanim.strip_comment(raw)
        mw = re.match(r"^\s*\.WORD\s+(.+)$", body, re.I)
        if mw:
            n += len([p for p in mw.group(1).split(",") if p.strip()])
        elif body.strip():
            break
    if cur is not None:
        counts.append((cur, n))
    assert len(counts) == wlwrsnd.WRESTLERS, counts
    for name, got in counts:
        assert got == wlwrsnd.MOVES_PER_WRESTLER, (name, got)

    # Every random index in either table names a real sub-table.
    rnd = wlwrsnd.random_tables()
    assert len(rnd) == 67, len(rnd)
    for t in rnd:
        assert t["last_index"] < len(t["entries"]), t["name"]
    for row in list(master) + [default]:
        for v in row:
            if v == wlwrsnd.DEFLT or v == 0:
                continue
            if v & wlwrsnd.RANDOM_BIT:
                assert (v ^ wlwrsnd.RANDOM_BIT) < len(rnd), hex(v)

    # The move names line up with what the ANI_CODE routines ask for.
    names = wlwrsnd.move_names()
    assert names[0] == "PUNCH_T1"
    assert names[32] == "GRABFLING_T1"
    assert names[54] == "RUGSLAM_YELL"
    assert names[55] == "RUGSLAM_IMPACT"
    assert names[58] == "YELL_THROW"
    # The four BIGBOOT_* equates are commented out in SOUND.H, so their
    # rows exist but have no name. That gap is left open, not closed up.
    assert 44 not in names and 45 not in names

    # Nothing in the shipped grid is a zero, so WRSNDX's `jrz DONE?` on
    # the wrestler's own entry is unreachable as written; the zeros are
    # all in DEFAULT_SOUND_TABLE.
    assert all(v != 0 for row in master for v in row)
    assert sum(1 for v in default if v == 0) == 13

    # Four wrestlers reuse another's row verbatim -- worth pinning,
    # because a drifted split would look exactly like this.
    assert master[2] == master[0]      # Undertaker == Bret
    assert master[4] == master[0]      # Shawn == Bret
    assert master[8] == master[0]      # Luger == Bret
    assert master[7] == master[3]      # Adam Bomb == Yokozuna
    assert master[1] != master[0] and master[5] != master[0]
    assert master[6] != master[0]      # Doink has his own


def test_wrsnd_tables_generate_the_shipped_file() -> None:
    out = ROOT / "src" / "generated" / "wrestler_sound_tables.c"
    if not (wlanim.ORIG / "DCSSOUND.ASM").exists() or not out.exists():
        return
    assert wlwrsnd.render_c() == out.read_text()



def test_digit_leading_local_labels_are_seen() -> None:
    """A local label may begin with a digit after the '#'.

    Five real ones do -- HRTSEQ4.ASM's `#4block` and SHNSEQ3.ASM's `#4`,
    `#2`, `#4xc`, `#2xc`, named after the 2-count and 4-count pin they
    lead to. Requiring a letter there made them invisible on BOTH sides:
    the definitions were not seen as labels, and the seven branches to
    them were silently skipped, so four animations ran straight through
    branches the original takes. (A sixth, `#2block`, appears only in
    commented-out lines in HRTSEQ4.ASM, so it is not counted here --
    which is the difference between grepping the file and reading it.)
    """
    base = ROOT / "original" / "wwf-wrestlemania"
    if not (base / "HRTSEQ4.ASM").exists():
        return

    defined, referenced = set(), 0
    branch = re.compile(r"ANI_(?:GOTO|IFSTATUS|IFNOTSTATUS|IFBLOCKED)\s*,"
                        r"\s*(#[0-9][A-Za-z0-9_]*)", re.I)
    for p in sorted(base.glob("*SEQ*.ASM")):
        if "'" in p.name:
            continue
        for raw in p.read_text(errors="replace").splitlines():
            line = wlanim.strip_comment(raw)
            m = wlanim.LOCAL_LABEL_RE.match(line)
            if m and m.group(1)[1].isdigit():
                defined.add((p.name, m.group(1)))
            referenced += len(branch.findall(line))

    assert len(defined) == 5, sorted(defined)
    assert referenced == 6, referenced

    # Both regexes see them, including the form where the label shares a
    # line with the frame it names (`#4\tWL\t1,S4ST4C+FR1`).
    assert wlanim.LOCAL_LABEL_RE.match("#4block")
    assert wlanim.LOCAL_LABEL_RE.match("#4\tWL\t1,S4ST4C+FR1")
    assert wlanim.LEADING_LOCAL_LABEL_RE.sub("", "#4\tWL\t1,S4ST4C+FR1") == \
        "WL\t1,S4ST4C+FR1"


def test_programs_record_where_they_start() -> None:
    """A routine that branches BACK into shared code starts partway in.

    The emitted body has to begin at the branch target for the branch to
    resolve, so the routine's own first command is some ops in. Starting
    such a program at op 0 plays code it merely jumps into -- a different
    animation. Every wrestler's `*_4_hitblock_anim` is one of these: it
    jumps into his `*_4_block_anim` hold at `#4block`.
    """
    if not (wlanim.ORIG / "HRTSEQ4.ASM").exists():
        return
    ops, entry, _labels = wlprogram.program_for(
        wlanim.ORIG / "HRTSEQ4.ASM", "hrt_4_hitblock_anim", with_entry=True)
    assert entry > 0, entry
    assert entry < len(ops)
    # Its own body starts with the SETMODE the routine opens with, and
    # the shared hold it branches into sits before that.
    assert ops[entry][0] == "SETMODE", ops[entry]
    assert any(o[0] == "FRAME" and o[1] == "H4BK3A02" for o in ops[:entry])

    # And the count is stable: 45 of the emitted programs are like this.
    # It was 42 until branch resolution learned to see a SUBR line as a
    # definition; the three that joined are yok_spinslam_anim,
    # yok_graboh_TB_anim and yok_combo_scissor_anim, each of which
    # ANI_IFSTATUSes back into yok_overhd_slam_anim earlier in the file.
    odd = 0
    for src in wlanim.linked_files():
        if "SEQ" not in src.name or src.name.startswith("FINI"):
            continue
        seen = []
        for line in src.read_text(errors="replace").splitlines():
            m = wlanim.SUBR_RE.match(line)
            if m and m.group(1) not in seen:
                seen.append(m.group(1))
        for lab in seen:
            try:
                _o, e, _l = wlprogram.program_for(src, lab, with_entry=True)
            except (OSError, ValueError):
                continue
            if e:
                odd += 1
    assert odd == 45, odd



def test_emitted_programs_hold_together() -> None:
    """Structural invariants over every emitted program.

    None of this proves parity with the arcade -- there is no reference to
    compare against -- but each one is a way the emitter could be wrong
    that would otherwise be silent, and every such check built so far has
    found something. Specifically:

      - a branch target outside its own program would run off the end
      - a program with no terminator would simply stop, freezing the
        wrestler, instead of ending or looping
      - a frame the wrestler shows with no extracted geometry has no hurt
        box, so he cannot be hit on that frame
    """
    out = ROOT / "src" / "generated" / "anim_programs.c"
    geo_file = ROOT / "src" / "generated" / "frame_geometry.c"
    if not out.exists() or not geo_file.exists():
        return
    src = out.read_text()
    geo = set(re.findall(r'"([A-Z][0-9][A-Z0-9]{4,6})"', geo_file.read_text()))

    bad_target, no_terminator, missing_geo = [], [], set()
    programs = 0
    for m in re.finditer(r"static const wm_anim_op prog_(\w+)_ops\[\] = \{(.*?)\n\};",
                         src, re.S):
        label, body = m.group(1), m.group(2)
        programs += 1
        rows = re.findall(r"\{\s*(WM_AOP_\w+),\s*\d+,\s*(-?\d+),", body)
        n = len(rows)
        for op, tgt in rows:
            if int(tgt) >= n:
                bad_target.append((label, op, tgt, n))
        ops = [o for o, _ in rows]
        # A program ends, loops, or hands off. Anything else runs out.
        # ANI_ROT parks forever -- next_pc stays put and the frame holds
        # -- so it never runs off the end either. xxx_dead_anim ends that
        # way and nothing else does.
        if not ({"WM_AOP_END", "WM_AOP_REPEAT", "WM_AOP_GOTO",
                 "WM_AOP_CHANGEANIM", "WM_AOP_ROT"} & set(ops[-1:])) and \
                "WM_AOP_END" not in ops:
            no_terminator.append(label)
        for f in re.findall(r'WM_AOP_FRAME,[^}]*?"([A-Z0-9]+)"', body):
            if f not in geo:
                missing_geo.add(f)

    assert programs > 1500, programs
    assert not bad_target, bad_target[:5]
    assert not no_terminator, no_terminator[:5]
    # The only frames a wrestler shows without geometry are the five
    # BURNBODY effect frames and the two the original has no artwork for
    # (HRTSEQ3.ASM names H4HU4B+FR10 and YOKSEQ1.ASM names Y2ST2Z+FR1;
    # neither symbol is in any shipped .LOD). A NEW one is a real gap.
    assert missing_geo == {
        "BURNBODY01", "BURNBODY02", "BURNBODY03", "BURNBODY04",
        "BURNBODY05", "H4HU4B10", "Y2ST2Z01",
    }, sorted(missing_geo)


def test_the_two_extractors_agree() -> None:
    """The flat frame extractor and the program emitter, cross-checked.

    src/generated/bret_visuals.c is an older, separate extraction of the
    same assembly into flat frame lists. Where both cover an animation
    they must agree on the frames AND their tick counts. They share
    wlanim.py's line parsing, so this is partial independence rather than
    two clean-room readings -- but it is the only second opinion this
    port has, and a divergence in either direction is a real signal.
    """
    vis_file = ROOT / "src" / "generated" / "bret_visuals.c"
    out = ROOT / "src" / "generated" / "anim_programs.c"
    if not vis_file.exists() or not out.exists():
        return
    vis = vis_file.read_text()

    flat = {}
    for m in re.finditer(r"static const wm_visual_frame (\w+)_frames\[\] = \{(.*?)\};",
                         vis, re.S):
        flat[m.group(1)] = [(f, int(t)) for f, t in
                            re.findall(r'\{"([A-Z0-9]+)",\s*(\d+)\}', m.group(2))]
    labels = {}
    seq_re = (r'const wm_visual_sequence \w+ = \{\s*\.source_file = "[^"]+",'
              r'\s*\.source_label = "([^"]+)",\s*\.frames = (\w+)_frames')
    for m in re.finditer(seq_re, vis):
        labels[m.group(2)] = m.group(1)

    src = out.read_text()
    progs = {}
    for m in re.finditer(r"static const wm_anim_op prog_(\w+)_ops\[\] = \{(.*?)\n\};",
                         src, re.S):
        progs[m.group(1)] = [
            (f, int(t)) for t, f in re.findall(
                r'\{\s*WM_AOP_FRAME,[^}]*?(\d+),\s*0,\s*0,\s*0,\s*0,\s*0,\s*"([A-Z0-9]+)"\s*\}',
                m.group(2))]

    compared, disagree = 0, []
    for sym, seq in flat.items():
        label = labels.get(sym)
        if not label or label not in progs:
            continue
        compared += 1
        if progs[label] != seq:
            disagree.append(label)
    assert compared >= 30, compared
    assert not disagree, disagree



def test_an_independent_reading_agrees() -> None:
    """The strongest parity evidence this port can produce on its own.

    tools/wlverify.py shares NOTHING with the rest of tools/ -- its own
    lexer, its own routine spans, its own label scoping, its own branch
    resolution -- and it RUNS the animation rather than re-extracting it,
    so what it produces is the frame sequence actually played. Comparing
    that against the emitted programs checks the emitter and the
    interpreter's frame/tick model together.

    What this does NOT cover, and the limit is the point: it can only
    play animations built from frames, ANI_GOTO, ANI_REPEAT, the repeat
    counter and ANI_PAUSE. Every conditional branch and every wait --
    ANI_IFSTATUS, ANI_WAITHITGND, ANI_CHANGEANIM and the rest -- makes a
    body unverifiable here, so the interesting half of the VM is exactly
    what is excluded. Agreement raises confidence in the frame and tick
    backbone, not in the branching semantics.

    Its reach is also bounded by something structural rather than by
    effort: a routine whose body BRANCHES INTO ANOTHER one (BAMSEQ1.ASM's
    bam_run2_anim is a bare `ANI_GOTO,#run2` into bam_run_anim) has no
    self-contained body to play. The emitter handles those by growing the
    program across routine boundaries; this reader deliberately does not,
    because copying that logic is exactly what would stop it being an
    independent check.
    """
    if not (wlverify.ASM / "WRESTLE.CMD").exists():
        return
    out = ROOT / "src" / "generated" / "anim_programs.c"
    if not out.exists():
        return

    live = wlverify.linked()
    independent = {}
    for path in sorted(wlverify.ASM.glob("*SEQ*.ASM")):
        if "'" in path.name or path.name.upper() not in live:
            continue
        for label, body in wlverify.routines(path).items():
            if not label.endswith("_anim"):
                continue
            try:
                seq, why = wlverify.play(body)
            except wlverify.Unverifiable:
                continue
            if seq and why == "end":
                independent[label] = seq

    src = out.read_text()
    emitted = {}
    for m in re.finditer(r"static const wm_anim_op prog_(\w+)_ops\[\] = \{(.*?)\n\};",
                         src, re.S):
        emitted[m.group(1)] = [
            (f, int(t)) for t, f in re.findall(
                r'\{\s*WM_AOP_FRAME,[^}]*?(\d+),\s*0,\s*0,\s*0,\s*0,\s*0,\s*"([A-Z0-9]+)"\s*\}',
                m.group(2))]

    compared, disagree, absent = 0, [], []
    for label, seq in independent.items():
        if label not in emitted:
            absent.append(label)
            continue
        compared += 1
        if emitted[label] != [(f, t) for f, t in seq]:
            disagree.append(label)

    # Enough of them for the check to mean something.
    assert compared >= 285, compared
    assert not disagree, disagree[:5]
    # Everything the independent reader can play and that the linker
    # builds must have been emitted. A gap here is a missing program.
    assert not absent, absent[:5]



def test_every_hard_coded_animation_label_resolves() -> None:
    """A label the C code hands off to by name must be a real program.

    src/core/anim_program.c sets `exec->become = "xxx_dead_anim"` on the
    death path. That program is not part of the roster sweep -- it comes
    from an explicit `--animation WRESTLE2.ASM xxx_dead_anim` in
    scripts/regenerate_source_data.sh -- so regenerating the file by hand
    with only --roster --slave-targets silently drops it, and the death
    hand-off then resolves to nothing at all. That happened during
    development and nothing caught it, because a `become` that finds no
    program just ends the animation quietly.
    """
    out = ROOT / "src" / "generated" / "anim_programs.c"
    core = ROOT / "src" / "core" / "anim_program.c"
    if not out.exists() or not core.exists():
        return
    emitted = set(re.findall(r"static const wm_anim_op prog_(\w+)_ops\[\]",
                             out.read_text()))
    wanted = set(re.findall(r'exec->become\s*=\s*"([A-Za-z0-9_]+)"',
                            core.read_text()))
    assert wanted, "no hard-coded become targets found -- pattern stale?"
    assert wanted <= emitted, sorted(wanted - emitted)


def test_font_tables_are_read_not_transcribed() -> None:
    """STRING.ASM's ten *_ascii tables, checked against the source's shape.

    Each is exactly 128 .long slots because the printer indexes it with
    `sll 5,a0` on a byte-masked ASCII code. Two things the extractor could
    get wrong and this catches: picking up the commented-out sgmd8_ascii
    (a leading `;` on every one of its lines), and running a table's
    parse past its end into the next label's rows.
    """
    if not wlstring.SRC.exists():
        return
    tables = wlstring.font_tables()
    assert len(tables) == 10, sorted(tables)
    assert "sgmd8_ascii" not in tables

    for name, slots in tables.items():
        assert len(slots) == wlstring.TABLE_SLOTS, (name, len(slots))
        # Space is never looked up -- print_string handles code 32 before
        # the table, so every table has 0 there.
        assert slots[ord(" ")] is None, name
        # Lower case aliases upper case in all ten.
        for ch in range(ord("a"), ord("z") + 1):
            assert slots[ch] == slots[ch - 0x20], (name, chr(ch))
        # A table that ran on into the next label would carry that
        # label's symbols. Every glyph in a table belongs to one image
        # family, named by the prefix its 'A' carries -- the only
        # documented exception being the digits, which font9A and
        # win_ascii swap for another family's.
        family = slots[ord("A")]
        assert family and family.endswith("_A"), (name, family)
        family = family[:-2]
        for ch, sym in enumerate(slots):
            if sym is None or ord("0") <= ch <= ord("9"):
                continue
            assert sym.startswith(family), (name, chr(ch), sym)

    # font9A is font9 with the alternate digits and nothing else; win is
    # ogmd10 with the WFONT digits and nothing else. Both differences are
    # exactly ten slots wide.
    for base, variant, marker in (("font9_ascii", "font9A_ascii", "A"),
                                  ("ogmd10_ascii", "win_ascii", "WFONT_")):
        a, b = tables[base], tables[variant]
        differ = [i for i in range(128) if a[i] != b[i]]
        assert differ == list(range(ord("0"), ord("9") + 1)), (variant, differ)
        if marker == "WFONT_":
            assert all(b[i].startswith(marker) for i in differ)
        else:
            assert all(b[i].endswith(marker) for i in differ)

    # The high-score editor's three cursor glyphs sit at $10-$12, above
    # the control codes and below the printable range.
    assert tables["font9_ascii"][0x10] == "FNT9_SPC"
    assert tables["font9_ascii"][0x11] == "FNT9_DEL"
    assert tables["font9_ascii"][0x12] == "FNT9_END"


def test_font_tables_generate_the_shipped_file() -> None:
    out = ROOT / "src" / "generated" / "font_tables.c"
    if not wlstring.SRC.exists() or not out.exists():
        return
    assert wlstring.render_c() == out.read_text()


def test_glyph_metrics_come_out_of_the_artwork() -> None:
    """Glyph widths, read from the .IMG containers rather than invented.

    STRING.ASM has no widths in it -- the printer reads the image header's
    first word -- so this is the one part of the text engine that has to
    come from the art. Two limits are real and are asserted here rather
    than papered over: a WIMP name field is eight characters, so symbols
    that truncate to a shared stem cannot be separated without FONTS.LOD's
    packing order (that file is empty in this tree), and the osgmd10_*
    family has no artwork at all.
    """
    if not wlstring.IMG_DIR.exists() or not wlstring.SRC.exists():
        return
    measured, ambiguous, absent = wlstring.glyph_metrics()

    # Every symbol the tables name is accounted for exactly once.
    named = set()
    for slots in wlstring.font_tables().values():
        named.update(s for s in slots if s)
    assert set(measured) | set(ambiguous) | set(absent) == named
    assert not (set(measured) & set(ambiguous))
    assert not (set(measured) & set(absent))

    # font9_ascii is measurable end to end: FNT9.IMG's names all fit the
    # eight-character field, so nothing in it collides.
    for sym in wlstring.font_table("font9_ascii"):
        if sym:
            assert sym in measured, sym

    # Widths are real pixels, not placeholders.
    assert measured["FNT9_A"] == 11
    assert measured["FNT9_I"] == 4        # a narrow letter stays narrow
    assert measured["FNT9_1"] == 5
    assert measured["FNT9_4"] == 11
    assert measured["WFONT_0"] == 6
    assert all(1 <= w <= 64 for w in measured.values())

    # The two documented gaps, spelled out.
    assert "osgmd8_A" in ambiguous and "osgmd8_B" in measured
    assert all(a.startswith("osgmd10_") for a in absent)
    assert len(absent) == 49


def test_glyph_metrics_generate_the_shipped_file() -> None:
    out = ROOT / "src" / "generated" / "font_metrics.c"
    if not wlstring.IMG_DIR.exists() or not wlstring.SRC.exists() or not out.exists():
        return
    assert wlstring.render_metrics_c() == out.read_text()


def test_imgpal_palettes_are_self_consistent() -> None:
    """IMGPAL.ASM's palettes, checked against their own count words.

    Every routine in PAL.ASM reads a palette as [count word][colours],
    masking the count to nine bits (`sll 32-9 / srl 32-9`). So the one
    thing that can go wrong in extraction -- running a block on into the
    next label, or stopping short -- shows up as a count that disagrees
    with the block's length. wlpal.palettes() refuses rather than
    truncating, so simply calling it is the check; these assertions pin
    the shape it produced.
    """
    if not wlpal.SRC.exists() or not wlpal.WRESPAL.exists():
        return
    pals, aliases = wlpal.palettes_with_aliases()
    assert len(pals) == 396, len(pals)

    # WRESTLE.CMD links both files and they share names, so the `.if`
    # blocks decide which copy assembles. All seven of WRESPAL.ASM's
    # `.if 0` palettes have a live counterpart in IMGPAL.ASM -- that is
    # what the guard is for. Reading them as live would define each
    # name twice, which the extractor refuses outright.
    img = wlpal.palettes([wlpal.SRC])
    live_wrespal = wlpal.palettes([wlpal.WRESPAL])
    assert len(img) == 337 and len(live_wrespal) == 59
    assert not (set(img) & set(live_wrespal))
    for name in ("BAMBLU_P", "DNKBLU_P", "LEXWHT_P", "RZRGRN_P",
                 "SHNRED_P", "UNDPRP_P", "YOKRED_P"):
        assert name not in live_wrespal, name
        assert name in img, name

    # Two labels stacked on one block share it rather than one of them
    # silently taking the next palette's colours.
    assert aliases == {"UNDBLU_P": "UNDGRN_P"}
    assert pals["UNDBLU_P"] is pals["UNDGRN_P"]

    for name, words in pals.items():
        count = words[0] & wlpal.COUNT_MASK
        assert len(words) - 1 == count, name
        assert 1 <= count <= 256, (name, count)
        assert all(0 <= w <= 0xFFFF for w in words), name

    # No shipped palette sets any of the count word's flag bits, so the
    # port's mask is load-bearing only in principle -- worth knowing.
    assert {w[0] & ~wlpal.COUNT_MASK for w in pals.values()} == {0}

    # PAL.ASM:118 takes DIAGP first so it is always colour map 0; its
    # first colours are read straight out of IMGPAL.ASM:739.
    assert pals["DIAGP"][0] == 29
    assert pals["DIAGP"][1:4] == [0x0000, 0x56B5, 0x7BDE]

    # The palette anim_code's #set_pal asks for by name (BAMSEQ2.ASM
    # #set_pal -> `movi BAMBLU_P,a0 / calla pal_getf`).
    assert "BAMBLU_P" in pals
    assert pals["BAMBLU_P"] == img["BAMBLU_P"]


def test_palettes_generate_the_shipped_file() -> None:
    out = ROOT / "src" / "generated" / "palettes.c"
    if not wlpal.SRC.exists() or not out.exists():
        return
    assert wlpal.render_c() == out.read_text()


def test_roster_anim_tables_name_real_routines() -> None:
    """Every label in a per-wrestler table is a routine that exists.

    This is the check that says the extraction is reading and not
    pattern-matching: the table rows look derivable from the wrestler
    prefix, and they are not. fall_back_tbukl_tbl gives Yokozuna
    `yok_fall_back_anim` where the pattern would predict
    `yok_fall_back_tbukl_anim`, and that second name is not a routine
    at all -- so a generator that derived rows would emit a dangling
    reference here and this would catch it.
    """
    tables = wlrostertbl.roster_tables()
    if not tables:
        return
    assert len(tables) == 36, sorted(tables)

    routines = set()
    for path in sorted(wlanim.ORIG.glob("*.ASM")):
        text = path.read_text(errors="replace")
        routines.update(re.findall(r"^\s*SUBRP?\s+([A-Za-z_]\w*)\s*$",
                                   text, re.M))
        # PROGRESS.ASM's leg/torso rows name animations declared as bare
        # column-0 labels rather than with SUBR.
        routines.update(re.findall(r"^([A-Za-z_]\w*)\s*$", text, re.M))

    for name, (fname, line, rows) in tables.items():
        for slot, label in enumerate(rows):
            if label is None:
                continue
            assert label in routines, (name, fname, line, slot, label)

    # The two details the header claims, asserted against the data.
    # (Both of these tables are one column wide, so a row index is a
    # roster slot.)
    assert tables["fall_back_tbukl_tbl"][2][3] == "yok_fall_back_anim"
    assert "yok_fall_back_tbukl_anim" not in routines
    for slot in (7, 9):
        assert tables["climbthru_bot_anims"][2][slot] == "dnk_climbthru_bot_anim"


def test_face24_tables_are_two_columns_wide() -> None:
    """FACE24TBL's tables carry a facing pair per wrestler.

    The macro (MACROS.H:65) scales WRESTLERNUM by X64 rather than X32,
    so each wrestler owns two longs, and it adds a long when
    MOVE_UP_BIT is *clear* -- column 0 is facing up, column 1 facing
    down. Reading these as one long per wrestler, which is what the
    nine/ten-row shape test used to do, dropped them entirely: nine
    rows of two is eighteen longs and matched nothing.
    """
    tables = wlrostertbl.roster_tables()
    if not tables:
        return
    expected = {
        "head_hit_tbl", "head_hit2_tbl", "head_hit_dizzy_tbl",
        "body_hit_tbl", "body_hit_dizzy_tbl", "knee_hit_tbl",
        "bncoff", "bncoff_gate",
    }
    for name in expected:
        rows = tables[name][2]
        assert len(rows) == wlrostertbl.ROSTER_SLOTS * 2, (name, len(rows))
        assert wlrostertbl.column_kind(name, 2, rows) == \
            wlrostertbl.COL_FACING, name

    # head_hit_tbl's own rows, straight off REACT1.ASM:1723.
    rows = tables["head_hit_tbl"][2]
    assert rows[0] == "hrt_2_head_hit_anim"
    assert rows[1] == "hrt_4_head_hit_anim"
    assert rows[14] is None and rows[15] is None      # slot 7, Adam Bomb
    assert rows[16] == "lex_2_head_hit_anim"

    # Three wrestlers share one head_hit2 animation across both facings.
    h2 = tables["head_hit2_tbl"][2]
    for slot in (2, 5, 6):
        assert h2[slot * 2] == h2[slot * 2 + 1]
        assert "_2_" not in h2[slot * 2] and "_4_" not in h2[slot * 2]


def test_a_two_long_row_is_not_always_a_facing_pair() -> None:
    """PROGRESS.ASM's `*_addr` tables have the shape and not the meaning.

    The loop at PROGRESS.ASM:3218 reads two longs and hands the first
    to change_anim1a and the second to change_anim2a -- a leg animation
    and a torso animation. Calling that a facing pair because it is two
    longs wide would be deriving the semantics from the shape, which is
    exactly the mistake this whole tool exists to avoid. The column
    meaning comes off the use site: a FACE24TBL operand is a facing,
    and nothing else is.
    """
    tables = wlrostertbl.roster_tables()
    if not tables:
        return
    for name in ("taunting_addr", "standing_addr", "dead_addr",
                 "running_addr", "clever_addr", "waiting_addr"):
        rows = tables[name][2]
        assert len(rows) == wlrostertbl.ROSTER_SLOTS * 2, name
        assert wlrostertbl.column_kind(name, 2, rows) == \
            wlrostertbl.COL_PAIR, name
        # Column 1 is a torso animation wherever it is filled at all.
        for slot in range(wlrostertbl.ROSTER_SLOTS):
            torso = rows[slot * 2 + 1]
            assert torso is None or "_torso" in torso, (name, slot, torso)

    assert tables["taunting_addr"][2][0] == "hrt_taunt4_anim"
    assert tables["taunting_addr"][2][1] == "hrt_torso4_anim"

    # And these really are not FACE24TBL operands anywhere in the game.
    operands = wlrostertbl.face24_operands()
    assert "taunting_addr" not in operands
    assert "bncoff_gate" in operands       # WRESTLE.ASM:3698, behind a label


def test_a_roster_table_row_gets_a_program() -> None:
    """Every animation a global table names is emitted, wherever it lives.

    The roster sweep in wlprogram walks SUBR lines in the per-wrestler
    SEQ files and deliberately skips FINISEQ.ASM, because a sweep there
    would emit the fourteen finishing moves GAME.EQU:580-587 assembles
    OUT, plus its data tables, plus a dozen plain logic routines. The
    cost of that skip was sixteen real animations: stand_table and
    dizzy_table (FINISEQ.ASM:1476 and :1493) name every wrestler's
    stand and fdizzy, and nothing else in the game reaches them.

    Naming the rows rather than sweeping the file is what gets them
    without the rest, so this checks both halves: the sixteen are
    present, and the cut finishing moves still are not.
    """
    out = ROOT / "src" / "generated" / "anim_programs.c"
    if not out.exists() or not wlanim.ORIG.exists():
        return
    text = out.read_text()

    for who in ("hrt", "rzr", "und", "yok", "shn", "bam", "dnk", "lex"):
        for kind in ("stand", "fdizzy"):
            sym = "prog_%s_%s_anim_ops[]" % (who, kind)
            assert sym in text, sym

    # GAME.EQU:580-587 sets seven of the eight NUM_*_FINISHES to zero,
    # so only the Undertaker's first finish is in the shipped game.
    for who in ("bret", "bam", "yoko", "doink", "razor", "lex", "shawn"):
        assert ("NUM_%s_FINISHES" % who.upper()) in \
            (wlanim.ORIG / "GAME.EQU").read_text()
    for sym in ("prog_bam_finish1_move_ops[]", "prog_hrt_finish1_move_ops[]",
                "prog_yok_finish1_move_ops[]", "prog_rzr_finish1_move_ops[]"):
        assert sym not in text, sym

    # And neither the data tables nor the logic routines beside them.
    for sym in ("prog_stand_table_ops[]", "prog_dizzy_table_ops[]",
                "prog_check_roll_ops[]", "prog_is_door_open_ops[]"):
        assert sym not in text, sym


def test_a_file_local_equate_resolves() -> None:
    """`.equ` without a `#`, and `/` in an operand.

    Two small holes in the operand reader, each of which refused a real
    animation outright rather than getting it wrong -- which is the
    right failure, but it still cost the programs. FINISEQ.ASM:1544
    writes `TIME_FOR_MOVE .equ 16` with no `#` prefix and a leading dot
    on the directive, and push_in_anim at :1588 uses it;
    raise_dead_anim waits `(TSEC/2)`, and division was not in the
    expression character set.
    """
    if not wlanim.ORIG.exists():
        return
    src = wlanim.ORIG / "FINISEQ.ASM"
    if not src.exists():
        return
    for label in ("push_in_anim", "raise_dead_anim"):
        prog = wlprogram.program_for(src, label)
        assert prog, label


def test_declarations_are_not_the_end_of_a_table() -> None:
    """`.ref` lines mid-table, and rows written as REFLONG.

    knee_hit_tbl (REACT3.ASM:223) declares its eight `.ref` lines
    between the label and the data, and head_hit2_sand_tbl
    (REACT1.ASM:1746) writes every row as REFLONG -- which MACROS.H:45
    expands to `.globl label` plus `.long label`. A reader that stops
    at the first line that is not a `.long` sees neither table.
    """
    tables = wlrostertbl.roster_tables()
    if not tables:
        return
    knee = tables["knee_hit_tbl"][2]
    assert knee[0] == "hrt_2_knee_hit_anim"
    assert knee[1] == "hrt_4_knee_hit_anim"

    sand = tables["head_hit2_sand_tbl"][2]
    assert len(sand) == wlrostertbl.ROSTER_SLOTS
    assert sand[0] == "hrt_4_head_hit2s_anim"
    assert sand[7] is None
    assert sand[8] == "lex_4_head_hit2s_anim"


def test_roster_anim_tables_refuse_ambiguous_labels() -> None:
    """A label defined more than once is left to wlpuppet.py.

    Those are `#local` and block-scoped; only a use site can say which
    definition a caller means, and picking one would be the same class
    of mistake as resolving #make_black by bare name.
    """
    if not wlanim.ORIG.exists():
        return
    tables = wlrostertbl.roster_tables()
    ambiguous = wlrostertbl._ambiguous()
    assert ambiguous, "expected at least one reused table label"
    for name in ambiguous:
        assert name not in tables, name


def test_roster_anim_tables_generate_the_shipped_file() -> None:
    out = ROOT / "src" / "generated" / "roster_anim_tables.c"
    if not wlanim.ORIG.exists() or not out.exists():
        return
    assert wlrostertbl.render_c() == out.read_text()


def test_coverage_does_not_count_a_comment_as_an_implementation() -> None:
    """The bug this tool exists to fix.

    An earlier ad-hoc measurement grepped each routine name in the
    port's text, so a comment saying "foo is not translated" counted
    foo as translated. Definitions and prose are separated before
    anything is matched; this checks that separation directly rather
    than trusting it.
    """
    code, prose = port_coverage._split_code_and_prose(
        "/* mode_dead is not translated */\n"
        "int wm_thing(void) { return 1; } // see also other_thing\n"
        'static const char *n = "named_in_a_string";\n')
    assert "wm_thing" in code
    assert "mode_dead" not in code
    assert "other_thing" not in code
    assert "named_in_a_string" not in code
    # ...but the prose keeps them, so "cited" can be reported as its
    # own answer instead of being mistaken for either extreme.
    assert "mode_dead" in prose
    assert "named_in_a_string" in prose


def test_coverage_matches_only_a_suffix_or_a_generated_wrapper() -> None:
    """A port identifier counts for a routine when it IS the name, ends
    with `_` and the name, or is a shape a generator wraps it in.

    "Contains it somewhere" was the first rule and it matched
    LIFEBAR.ASM's `meters` against AWARD.ASM's `drone_meters_on` flag.
    The ledger guard caught that, which is the whole reason the guard
    exists.
    """
    pool = {"wm_arcade_round_award", "round_award", "unround_awardx",
            "round_award_fn", "drone_round_award_on",
            "prog_round_award_ops"}
    hits = set(port_coverage._boundary_hits("round_award", pool))
    assert hits == {"wm_arcade_round_award", "round_award",
                    "prog_round_award_ops"}
    assert "round_award_fn" not in hits          # trailing qualifier
    assert "drone_round_award_on" not in hits    # embedded, not a suffix
    assert "unround_awardx" not in hits

    # The real false positive that motivated the rule.
    assert port_coverage._boundary_hits("meters", {"drone_meters_on"}) == []
    # Below the minimum length nothing is matched loosely at all.
    assert port_coverage._boundary_hits("abc", {"wm_abc_thing"}) == []


def test_coverage_only_counts_linked_files() -> None:
    """ROBO.ASM and friends are other Williams games sitting in the same
    tree. Counting `bullet`, `hulk` and `enforcer` as untranslated
    WrestleMania routines inflated the denominator by 250."""
    if not (wlanim.ORIG / "WRESTLE.CMD").exists():
        return
    linked = port_coverage.linked_asm()
    assert "WRESTLE.ASM" in linked and "LIFEBAR.ASM" in linked
    assert "ROBO.ASM" not in linked
    assert "FIREBAK.ASM" not in linked
    assert "ADAM.ASM" not in linked
    routines = port_coverage.source_routines()
    for gone in ("bullet", "hulk", "enforcer", "quark"):
        assert gone not in routines, gone


def test_coverage_dead_means_nothing_names_it() -> None:
    """`dead` used to mean "no caller outside its own file", which is
    wrong for anything reached through an address table -- ATTRACT.ASM's
    show_operatormsg sits in the attract sequence table in its own file.
    It now means zero references anywhere, definition line excluded."""
    if not wlanim.ORIG.exists():
        return
    outside, total = port_coverage.reference_counts()
    assert total["show_operatormsg"] > 0
    assert outside["show_operatormsg"] == 0      # the old test would fire
    # A genuinely unreferenced routine, verified by hand.
    assert total["HORZ_SHAKER2"] == 0

    data = port_coverage.classify()
    for row in data["routines"]:
        if row["status"] == "dead":
            assert row["references"] == 0, row["name"]


def test_the_routine_ledger_justifies_every_entry() -> None:
    """port/routine_map.json is where an unanswered question becomes an
    answered one, not where it goes to be forgotten.

    Each entry has to name a real routine in a linked file, carry a
    status the tool knows and a note, and -- if it claims the port does
    the work under another name -- name a port identifier that really
    is defined in code rather than merely mentioned.
    """
    if not wlanim.ORIG.exists():
        return
    ledger = port_coverage.load_ledger()
    assert ledger, "expected a non-empty ledger"

    routines = port_coverage.source_routines()
    code_ids, _prose, _gen = port_coverage.port_symbols()
    _outside, total = port_coverage.reference_counts()

    for name, entry in ledger.items():
        assert name in routines, "%s is not a routine in a linked file" % name
        assert entry["status"] in port_coverage.LEDGER_STATUSES, name
        assert entry.get("note"), "%s has no note" % name
        # The recorded file has to be where the routine actually is.
        files = {f for f, _l, _loc in routines[name]}
        assert entry["file"] in files, (name, entry["file"], sorted(files))
        if entry["status"] in ("inlined", "renamed", "partial"):
            sym = entry.get("symbol")
            assert sym, "%s claims %s but names no symbol" % (name, entry["status"])
            assert sym in code_ids, \
                "%s points at %s, which the port does not define" % (name, sym)
        if entry["status"] == "dead":
            assert total[name] == 0, name


def test_the_ledger_cannot_shadow_real_code() -> None:
    """No entry may claim a routine the automatic pass already resolves.

    If it could, the ledger would keep asserting a routine was fine
    after the code implementing it was deleted -- the failure mode that
    makes hand-maintained coverage files worthless.
    """
    if not wlanim.ORIG.exists():
        return
    ledger = port_coverage.load_ledger()
    code_ids, _prose, _gen = port_coverage.port_symbols()
    for name in ledger:
        assert not port_coverage._boundary_hits(name, code_ids), \
            "%s resolves on its own; the ledger entry is stale" % name


# Routines translated in this port and verified by hand, one at a time,
# against the C that implements them. The resolver is measured against
# this rather than trusted: when it was first written it got 35 of them
# right, and each miss was a real bug in it -- a `SUBRP name:` with a
# trailing colon that the label regex dropped, case-sensitive matching
# that could not see wm_rng_rndrng0 for RNDRNG0, and a suffix rule that
# needed the ledger for names the port spells differently.
VERIFIED_TRANSLATED = (
    "read_switches", "init_life_data", "init_wres_life_data",
    "init_rnd_life_data", "inc_life", "get_health", "clear_lifebar",
    "pal_init", "pal_find", "pal_getf", "pal_set", "pal_transfer",
    "pal_clean", "pal_blacken", "pal_fade", "pal_addb", "do_fade",
    "fade_up", "fade_down", "fade_up_half", "fade_down_half",
    "dec_to_asc", "dec_to_pct", "copy_string", "concat_string",
    "get_string_len", "print_string", "setup_message", "print_message",
    "clear_buffers", "ck_teammate_pin", "ck_live_teammates",
    "ck_any_teammates", "is_a14_behind", "clr_climb", "get_powerups",
    "powerup_check", "round_award", "match_award", "rst_awards",
    "accumulate_awards", "adjust_perfects", "total_icons",
    "get_num_awards", "is_it_a_really_quick_win", "arm_comeback_award",
    "get_stick_val_cur", "get_but_val_cur", "get_all_buttons_cur",
    "get_start_cur", "fall_back_tbl", "RNDRNG0", "square_root",
)

RESOLVED = ("implemented", "renamed", "inlined", "partial")


def test_coverage_resolves_every_routine_known_to_be_translated() -> None:
    """The resolver's own accuracy, measured not assumed.

    A coverage tool that quietly stops recognising work is worse than
    no tool, because it turns into a to-do list of things already
    done -- which is exactly what the previous grep-based measurement
    became.
    """
    if not wlanim.ORIG.exists():
        return
    data = port_coverage.classify()
    status = {r["name"]: r["status"] for r in data["routines"]}
    missed = [(n, status.get(n, "not-a-routine"))
              for n in VERIFIED_TRANSLATED
              if status.get(n) not in RESOLVED]
    assert not missed, missed


def test_coverage_does_not_claim_things_that_are_not_translated() -> None:
    """The other direction. These are named in the port only by
    comments explaining that they are NOT translated, and the earlier
    grep-based measurement counted every one of them as covered."""
    if not wlanim.ORIG.exists():
        return
    data = port_coverage.classify()
    status = {r["name"]: r["status"] for r in data["routines"]}
    for name in ("obj_aniq", "civanic", "CLR_SCRN", "meters", "set_volume"):
        assert status.get(name) not in ("implemented",), (name, status.get(name))


def test_no_emitted_branch_has_a_lost_destination() -> None:
    """Every branch op in the corpus carries a real destination.

    IFROPE and IFNOTROPE registered their fixup correctly and the
    fixup resolved `#fall_back` correctly -- and then `_c_op` wrote -1,
    because it only reads op[1] as the target for ops listed in
    BRANCH_OPS and those two were not in it. Seventy-five branches lost
    their destination on the way out of the emitter.

    In the interpreter `(size_t)-1` fails the `pc < op_count` test and
    falls out of the dispatch loop into `ended = true`, so a wrestler
    standing within 100 of a rope did not fall back -- his animation
    simply stopped. Seven programs, every wrestler's break_neck among
    them.
    """
    out = ROOT / "src" / "generated" / "anim_programs.c"
    if not out.exists():
        return
    text = out.read_text()
    kinds = {"WM_AOP_" + k for k in wlprogram.BRANCH_OPS}
    bad = []
    for m in re.finditer(r"\{ (WM_AOP_\w+), (\d+), (-?\d+),", text):
        if m.group(1) in kinds and int(m.group(3)) < 0:
            bad.append(m.group(1))
    assert not bad, collections.Counter(bad)

    # And the two that were lost really are emitted, resolved, in
    # numbers -- so this cannot pass by them vanishing instead. A floor
    # rather than an equality: the corpus grows when the emitter learns
    # to accept a routine it used to refuse, and it did (75 branches at
    # the time of the fix, 78 once branches into a later SUBR resolved).
    # What must never happen is the count going DOWN.
    assert text.count("WM_AOP_IFROPE,") >= 68
    assert text.count("WM_AOP_IFNOTROPE,") >= 7


def test_every_branch_op_is_listed_as_one() -> None:
    """The root cause, guarded directly: an op that registers a fixup
    must be in BRANCH_OPS, or its resolved target is silently dropped."""
    src = (ROOT / "tools" / "wlprogram.py").read_text()
    # Each `fixups.append((len(ops), ...))` is followed by the
    # ops.append that it annotates; the kind is the first string in it.
    for m in re.finditer(r"fixups\.append\(\(len\(ops\),[^\n]*\n\s*ops\.append\(\(([^\n]*)",
                         src):
        chunk = m.group(1)
        names = re.findall(r'"(\w+)"', chunk)
        for n in names:
            if n in wlprogram.BRANCH_OPS:
                break
        else:
            # A conditional whose kind is chosen at run time (BRANCHES[..])
            # is fine -- every value in that table is in BRANCH_OPS.
            assert "BRANCHES[" in chunk, chunk


def main() -> int:
    test_wlanim()
    test_wlprogram()
    test_wlprogram_roster_wide()
    test_wlprogram_is_deterministic()
    test_body_stop_ends_a_frameless_routine()
    test_linked_files_is_the_game()
    test_bare_label_routines()
    test_slave_targets_all_emit()
    test_truncated_frame_names()
    test_gravity_opcodes()
    test_command_table_ops_keep_their_operands()
    test_roll_tables()
    test_self_contained_command_ops()
    test_per_wrestler_aux_tables()
    test_anim_code_registry_reaches_its_call_sites()
    test_a_local_routine_defined_twice_gets_two_rows()
    test_the_emitter_skips_nothing()
    test_every_ani_code_call_site_resolves()
    test_target_offsets_grid()
    test_target_tables_generate_the_shipped_file()
    test_code_roster_tables_are_read_not_transcribed()
    test_smove_monitor_registry_is_honest()
    test_crowd_tables_come_out_of_the_source()
    test_announce_tables()
    test_announce_tables_generate_the_shipped_file()
    test_announce_calls_are_spelled_as_the_call_sites_spell_them()
    test_wrsnd_tables()
    test_wrsnd_tables_generate_the_shipped_file()
    test_font_tables_are_read_not_transcribed()
    test_font_tables_generate_the_shipped_file()
    test_glyph_metrics_come_out_of_the_artwork()
    test_glyph_metrics_generate_the_shipped_file()
    test_imgpal_palettes_are_self_consistent()
    test_palettes_generate_the_shipped_file()
    test_roster_anim_tables_name_real_routines()
    test_face24_tables_are_two_columns_wide()
    test_a_two_long_row_is_not_always_a_facing_pair()
    test_declarations_are_not_the_end_of_a_table()
    test_a_roster_table_row_gets_a_program()
    test_a_file_local_equate_resolves()
    test_roster_anim_tables_refuse_ambiguous_labels()
    test_roster_anim_tables_generate_the_shipped_file()
    test_per_wrestler_anim_tables_come_out_of_the_dispatch_lists()
    test_per_wrestler_tables_generate_the_shipped_file()
    test_smove_tables_honour_the_finishing_move_switch()
    test_coverage_does_not_count_code_the_assembler_skipped()
    test_the_two_bmod_producers_agree()
    test_sprite_banks_can_be_bundled_per_wrestler()
    test_extracted_cut_content_says_so()
    test_coverage_does_not_count_a_comment_as_an_implementation()
    test_coverage_matches_only_a_suffix_or_a_generated_wrapper()
    test_coverage_only_counts_linked_files()
    test_coverage_dead_means_nothing_names_it()
    test_the_routine_ledger_justifies_every_entry()
    test_the_ledger_cannot_shadow_real_code()
    test_coverage_resolves_every_routine_known_to_be_translated()
    test_coverage_does_not_claim_things_that_are_not_translated()
    test_no_emitted_branch_has_a_lost_destination()
    test_every_branch_op_is_listed_as_one()
    test_digit_leading_local_labels_are_seen()
    test_programs_record_where_they_start()
    test_emitted_programs_hold_together()
    test_every_hard_coded_animation_label_resolves()
    test_the_two_extractors_agree()
    test_an_independent_reading_agrees()
    test_a_label_past_the_last_op_is_not_a_position()
    test_a_branch_can_reach_the_head_of_a_later_routine()
    test_waithitopp_is_a_mode_and_a_frame()
    test_roster_dispatcher_labels_all_emit()
    test_wlprogram_tick_expressions()
    test_wlanim_label_def()
    test_wlattack_audit()
    test_wlattack_frame_indices()
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
