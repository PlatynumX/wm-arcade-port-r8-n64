# Historical DK semantic-audit regression reconciled to Combat2DM selective ownership.
from pathlib import Path
import importlib.util, subprocess, sys, tempfile
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('apply_fix39',ROOT/'tools/apply_fix39.py')
m=importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
with tempfile.TemporaryDirectory() as td:
    r=Path(td); (r/'src/core/arcade').mkdir(parents=True); (r/'src/fix39').mkdir(parents=True)
    names=['wm_arcade_combat.c','wm_arcade_drone.c','wm_arcade_bret.c','wmania_attract_core.c','wmania_attract_adapter.c','wm_arcade_roster.c','wmania_ring_geometry.c','wmania_rng.c','wm_fix39_runtime.c']
    for n in names: (r/'src/fix39'/n).write_text('/* fix39 */\n')
    for n in names[:-1]: (r/'src/core/arcade'/n).write_text('/* core */\n')
    cm='add_library(wmcore STATIC\n    src/generated/bret_attacks.c\n'+''.join(f'    src/core/arcade/{n}\n' for n in names[:-1])+')\ntarget_include_directories(wmcore PUBLIC include)\nif(WM_BUILD_TESTS)\n    add_test(NAME wm_core COMMAND wm_tests)\nendif()\n'
    mk='CFLAGS += -I$(CURDIR)/include\nCORE_C := \\\n'+''.join(f'    src/core/arcade/{n} \\\n' for n in names[:-1])+'\nASSET_C := x.c\nC_FILES := $(CORE_C) $(ASSET_C)\n$(BUILD_DIR)/$(ROMNAME).elf: $(OBJS)\n$(ROMNAME).z64: $(BUILD_DIR)/$(ROMNAME).dfs\n'
    (r/'CMakeLists.txt').write_text(cm); (r/'Makefile').write_text(mk)
    m.patch_cmake(r/'CMakeLists.txt',names); m.patch_makefile(r/'Makefile',names)
    audit=ROOT/'tools/fix39_build_graph_audit.py'
    good=subprocess.run([sys.executable,str(audit),str(r)],text=True,capture_output=True)
    assert good.returncode==0,good.stdout+good.stderr
    # Reintroducing a stale core combat owner must fail.
    p=r/'Makefile'; t=p.read_text().replace('CORE_C :=','CORE_C := src/core/arcade/wm_arcade_combat.c ',1); p.write_text(t)
    bad=subprocess.run([sys.executable,str(audit),str(r)],text=True,capture_output=True)
    assert bad.returncode!=0,bad.stdout+bad.stderr
print('Combat2DK historical semantic audit reconciled to DN/DO dependency-closed ownership: PASS')
