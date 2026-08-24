# Historical DI regression, reconciled by Combat2DM: convergence is selective,
# not basename-global. Keep this filename because older build scripts reference it.
from pathlib import Path
import importlib.util, tempfile
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('apply_fix39', ROOT/'tools/apply_fix39.py')
m=importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
with tempfile.TemporaryDirectory() as td:
    r=Path(td); (r/'src/core/arcade').mkdir(parents=True); (r/'src/fix39').mkdir(parents=True)
    names=['wm_arcade_combat.c','wm_arcade_drone.c','wm_arcade_react1_core.c','wm_arcade_bret.c','wmania_attract_core.c','wm_arcade_roster.c','wm_fix39_runtime.c']
    for n in names: (r/'src/fix39'/n).write_text('/* fix39 */\n')
    for n in names[:-1]: (r/'src/core/arcade'/n).write_text('/* core */\n')
    cm='add_library(wmcore STATIC\n    src/core/app.c\n    src/generated/bret_attacks.c\n'+''.join(f'    src/core/arcade/{n}\n' for n in names[:-1])+')\ntarget_include_directories(wmcore PUBLIC include)\nif(WM_BUILD_TESTS)\n    add_test(NAME wm_core COMMAND wm_tests)\nendif()\n'
    mk='CFLAGS += -I$(CURDIR)/include\nCORE_C := src/core/app.c \\\n'+''.join(f'    src/core/arcade/{n} \\\n' for n in names[:-1])+'\nASSET_C := src/generated/foo.c\nC_FILES := $(CORE_C) $(ASSET_C)\n$(BUILD_DIR)/$(ROMNAME).elf: $(OBJS)\n$(ROMNAME).z64: $(BUILD_DIR)/$(ROMNAME).dfs\n'
    (r/'CMakeLists.txt').write_text(cm); (r/'Makefile').write_text(mk)
    m.patch_cmake(r/'CMakeLists.txt',names); m.patch_makefile(r/'Makefile',names)
    cm1=(r/'CMakeLists.txt').read_text(); mk1=(r/'Makefile').read_text()
    m.patch_cmake(r/'CMakeLists.txt',names); m.patch_makefile(r/'Makefile',names)
    assert (r/'CMakeLists.txt').read_text()==cm1 and (r/'Makefile').read_text()==mk1
    for n in ['wm_arcade_combat.c','wm_arcade_drone.c','wm_arcade_react1_core.c','wm_arcade_bret.c']:
        assert f'src/fix39/{n}' in cm1 and f'src/fix39/{n}' in mk1
        assert f'src/core/arcade/{n}' not in cm1 and f'src/core/arcade/{n}' not in mk1
    for n in ['wmania_attract_core.c']:
        assert f'src/fix39/{n}' in cm1 and f'src/fix39/{n}' in mk1
        assert f'src/core/arcade/{n}' not in cm1 and f'src/core/arcade/{n}' not in mk1
    for n in ['wm_arcade_roster.c']:
        assert f'src/core/arcade/{n}' in cm1 and f'src/core/arcade/{n}' in mk1
        assert f'src/fix39/{n}' not in cm1 and f'src/fix39/{n}' not in mk1
print('Combat2DI historical graph regression reconciled to DN dependency-closed ownership: PASS')
