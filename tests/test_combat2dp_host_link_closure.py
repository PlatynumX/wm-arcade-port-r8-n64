from pathlib import Path
import tempfile, subprocess, textwrap
R=Path(__file__).resolve().parents[1]
# Static policy check.
a=(R/'tools/apply_fix39.py').read_text()
for n in ('wmania_attract_visuals.c','wmania_attract_operator.c'):
    assert n in a, n
# Exercise generic audit with an intentionally missing symbol, then provide it.
with tempfile.TemporaryDirectory() as td:
    r=Path(td); (r/'src/fix39').mkdir(parents=True); (r/'tests').mkdir()
    (r/'CMakeLists.txt').write_text('add_library(wmcore STATIC\n    src/fix39/a.c\n)\n')
    (r/'src/fix39/a.c').write_text('int wm_present(void){return 1;}\n')
    (r/'tests/fix39_smoke.c').write_text('int main(void){return wm_present()+wm_missing();}\n')
    q=subprocess.run(['python',str(R/'tools/fix39_host_link_surface_audit.py'),str(r)],capture_output=True,text=True)
    assert q.returncode != 0 and 'wm_missing' in q.stderr
    (r/'src/fix39/a.c').write_text('int wm_present(void){return 1;}\nint wm_missing(void){return 0;}\n')
    q=subprocess.run(['python',str(R/'tools/fix39_host_link_surface_audit.py'),str(r)],capture_output=True,text=True)
    assert q.returncode == 0, q.stderr+q.stdout
print('Combat2DP host-link closure regression: PASS')
