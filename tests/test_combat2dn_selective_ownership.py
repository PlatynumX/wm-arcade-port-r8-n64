from pathlib import Path
import tempfile, importlib.util
R=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('apply_fix39_dn',R/'tools/apply_fix39.py'); m=importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
with tempfile.TemporaryDirectory() as td:
    r=Path(td); (r/'src/core/arcade').mkdir(parents=True); (r/'src/fix39').mkdir(parents=True)
    names=['wm_arcade_combat.c','wm_arcade_drone.c','wm_arcade_bret.c',
           'wmania_attract_core.c','wmania_attract_adapter.c','wm_arcade_roster.c',
           'wmania_ring_geometry.c','wmania_rng.c','wmania_attract_visuals.c','wmania_attract_operator.c','wm_fix39_runtime.c']
    for n in names: (r/'src/fix39'/n).write_text('/* fix */\n')
    for n in names[:-1]: (r/'src/core/arcade'/n).write_text('/* core */\n')
    cm='add_library(wmcore STATIC\n    src/generated/bret_attacks.c\n'+''.join(f'    src/core/arcade/{n}\n' for n in names[:-1])+')\ntarget_include_directories(wmcore PUBLIC include)\nif(WM_BUILD_TESTS)\n    add_test(NAME wm_core COMMAND wm_tests)\nendif()\n'
    mk='CFLAGS += -I$(CURDIR)/include\nCORE_C := \\\n'+''.join(f'    src/core/arcade/{n} \\\n' for n in names[:-1])+'\nASSET_C := foo.c\nC_FILES := $(CORE_C) $(ASSET_C)\n$(BUILD_DIR)/$(ROMNAME).elf: $(OBJS)\n$(ROMNAME).z64: $(BUILD_DIR)/$(ROMNAME).dfs\n'
    (r/'CMakeLists.txt').write_text(cm); (r/'Makefile').write_text(mk)
    src=sorted(p.name for p in (r/'src/fix39').glob('*.c'))
    m.patch_cmake(r/'CMakeLists.txt',src); m.patch_makefile(r/'Makefile',src)
    cm2=(r/'CMakeLists.txt').read_text(); mk2=(r/'Makefile').read_text()
    # Combat plus source-backed shared providers must be Fix39-owned.
    for n in ['wm_arcade_combat.c','wm_arcade_drone.c','wm_arcade_bret.c',
              'wmania_ring_geometry.c','wmania_rng.c','wmania_attract_core.c','wmania_attract_adapter.c','wmania_attract_visuals.c','wmania_attract_operator.c']:
        assert f'src/fix39/{n}' in cm2 and f'src/fix39/{n}' in mk2, n
        assert f'src/core/arcade/{n}' not in cm2 and f'src/core/arcade/{n}' not in mk2, n
    # Presenter/adapter and roster remain on the newer core path.
    for n in ['wm_arcade_roster.c']:
        assert f'src/core/arcade/{n}' in cm2 and f'src/core/arcade/{n}' in mk2, n
        assert f'src/fix39/{n}' not in cm2 and f'src/fix39/{n}' not in mk2, n
print('Combat2DN dependency-closed selective ownership regression: PASS')
