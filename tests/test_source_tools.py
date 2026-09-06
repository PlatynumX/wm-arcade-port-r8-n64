#!/usr/bin/env python3
from __future__ import annotations
import importlib.util
import pathlib
import struct
import subprocess
import os
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
wlattack = load("wlattack", ROOT / "tools" / "wlattack.py")
wlcommands = load("wlcommands", ROOT / "tools" / "wlcommands.py")
wlpuppet = load("wlpuppet", ROOT / "tools" / "wlpuppet.py")
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
