from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNTIME = (ROOT / "src/fix39/wm_fix39_runtime.c").read_text()
SOURCE = (ROOT / "source_payload/anim/WRESTLE2.ASM").read_text()


def test_midway_authority_is_bundled():
    assert "SUBR\\tck_live_teammates" in SOURCE or "SUBR\tck_live_teammates" in SOURCE
    for token in ("process_ptrs", "PLYR_SIDE", "MODE_DEAD", "setc", "clrc"):
        assert token in SOURCE


def test_live_callback_is_not_the_old_1v1_shortcut():
    assert "live_no_teammates" not in RUNTIME
    assert "current live match path is source 1v1" not in RUNTIME
    assert "live_has_live_teammates" in RUNTIME


def test_live_callback_preserves_ck_live_teammates_predicates():
    body = RUNTIME.split("static int live_has_live_teammates", 1)[1].split("static int live_rndper_hi", 1)[0]
    assert "g.actor_ptrs[i]" in body
    assert "!candidate || !candidate->active" in body
    assert "candidate == victim" in body
    assert "candidate->player_side != victim->player_side" in body
    assert "candidate->player_mode == WM_PMODE_DEAD" in body
    assert "return 1;" in body
    assert "return 0;" in body
