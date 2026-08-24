\
#!/usr/bin/env python3
from pathlib import Path
import subprocess, tempfile, sys

tool = Path(sys.argv[1] if len(sys.argv)>1 else
            Path(__file__).resolve().parents[1]/'tools/fix39_strict_runtime_parity_audit.py')

with tempfile.TemporaryDirectory() as td:
    r=Path(td)
    (r/'src/fix39').mkdir(parents=True)
    files={
      'wm_arcade_character_attack_frames_generated.h':'#define X 1\n',
      'wm_arcade_drone_source_ranges_generated.h':'#define X 1\n',
      'wm_arcade_drone_source_scripts_generated.h':'#define WM_FIX39_DRONE_SCRIPT_COUNT 75\n',
      'wm_arcade_drone_source_tables_generated.h':'#define X 1\n',
      'wm_arcade_drone_source_services_generated.h':'#define WM_FIX39_DRONE_SERVICE_COUNT 15\n',
      'wm_arcade_drone_source_bodies_generated.h':'#define WM_FIX39_DRONE_TRANSLATED_BODY_COUNT 0\n',
    }
    for n,t in files.items():
        (r/'src/fix39'/n).write_text(t)

    strict=subprocess.run([sys.executable,str(tool),str(r)],
                          text=True,capture_output=True)
    assert strict.returncode != 0, strict.stdout+strict.stderr
    assert 'translated=0/15' in strict.stdout

    playable=subprocess.run([sys.executable,str(tool),'--playable-lane',str(r)],
                            text=True,capture_output=True)
    assert playable.returncode == 0, playable.stdout+playable.stderr
    assert 'PASS WITH DECLARED DEVELOPMENT GAP' in playable.stdout
    assert 'translated=0/15' in playable.stdout

    # Missing source services must still fail in playable mode.
    (r/'src/fix39/wm_arcade_drone_source_services_generated.h').unlink()
    broken=subprocess.run([sys.executable,str(tool),'--playable-lane',str(r)],
                          text=True,capture_output=True)
    assert broken.returncode != 0

print('Combat2EF-R2 playable/final runtime audit split: PASS')
