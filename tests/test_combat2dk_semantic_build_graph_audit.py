from pathlib import Path
import importlib.util, subprocess, sys, tempfile
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('apply_fix39', ROOT/'tools/apply_fix39.py')
m=importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
fix_names=sorted(p.name for p in (ROOT/'src/fix39').glob('*.c'))
assert len(fix_names) >= 70, len(fix_names)
with tempfile.TemporaryDirectory() as td:
    r=Path(td); (r/'src/core/arcade').mkdir(parents=True); (r/'src/fix39').mkdir(parents=True)
    for n in fix_names:
        (r/'src/fix39'/n).write_text('/* authoritative */\n')
        (r/'src/core/arcade'/n).write_text('/* stale */\n')
    cm='add_library(wmcore STATIC\n    src/core/app.c\n    src/generated/bret_attacks.c\n'+''.join(f'    src/core/arcade/{n}\n' for n in fix_names)+')\ntarget_include_directories(wmcore PUBLIC include)\nif(WM_BUILD_TESTS)\n    add_test(NAME wm_core COMMAND wm_tests)\nendif()\n'
    mk='CFLAGS += -I$(CURDIR)/include\nCORE_C := src/core/app.c \\\n'+''.join(f'    src/core/arcade/{n} \\\n' for n in fix_names)+'\nASSET_C := src/generated/foo.c\nC_FILES := $(CORE_C) $(ASSET_C)\n$(BUILD_DIR)/$(ROMNAME).elf: $(OBJS)\n'
    (r/'CMakeLists.txt').write_text(cm); (r/'Makefile').write_text(mk)
    m.patch_cmake(r/'CMakeLists.txt', fix_names)
    m.patch_makefile(r/'Makefile', fix_names)
    for fn in ['CMakeLists.txt','Makefile']:
        p=r/fn; txt=p.read_text()
        txt='\n'.join(line for line in txt.splitlines() if 'FIX39 SOURCE-DIRECT MERGE' not in line)+'\n'
        p.write_text(txt)
    audit=ROOT/'tools/fix39_build_graph_audit.py'
    good=subprocess.run([sys.executable,str(audit),str(r)],text=True,capture_output=True)
    assert good.returncode == 0, good.stdout+good.stderr
    assert 'PASS' in good.stdout
    victim=fix_names[0]
    p=r/'Makefile'; txt=p.read_text(); txt=txt.replace('CORE_C := src/core/app.c',f'CORE_C := src/core/app.c src/core/arcade/{victim}',1); p.write_text(txt)
    bad=subprocess.run([sys.executable,str(audit),str(r)],text=True,capture_output=True)
    assert bad.returncode != 0, bad.stdout+bad.stderr
    assert 'stale owner' in bad.stdout or 'duplicate core/fix39 owners' in bad.stdout
print('Combat2DK semantic build-graph audit regression: PASS')
