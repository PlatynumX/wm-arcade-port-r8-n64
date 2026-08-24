from pathlib import Path
p=Path(__file__).resolve().parents[1]/"tools/fix39_combat_runtime_parity_patch.py"
s=p.read_text()
proto="static int drone_check_combo_go(wm_arcade_actor_t *actor, void *user);"
helper="static int source_check_combo_go_port"
assert proto in s, "missing forward declaration for injected callback wrapper"
assert s.index(proto) < s.index(helper), "prototype must precede wrapper"
print("combat2cx callback prototype regression: PASS")
