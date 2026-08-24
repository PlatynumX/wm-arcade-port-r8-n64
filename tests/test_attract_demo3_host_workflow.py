from pathlib import Path
import importlib.util
import tempfile

ROOT = Path(__file__).resolve().parents[1]
modpath = ROOT / 'tools' / 'apply_fix39.py'
spec = importlib.util.spec_from_file_location('apply_fix39', modpath)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

workflow = '''name: build\njobs:\n  host-verification:\n    runs-on: ubuntu-latest\n    steps:\n      - uses: actions/checkout@v4\n      - name: Configure portable verifier\n        run: cmake -S . -B build-host -DCMAKE_BUILD_TYPE=Release\n      - name: Build portable verifier\n        run: cmake --build build-host --parallel\n'''
with tempfile.TemporaryDirectory() as td:
    p = Path(td) / 'build.yml'
    p.write_text(workflow)
    mod.patch_github_workflow(p)
    once = p.read_text()
    mod.patch_github_workflow(p)
    twice = p.read_text()
    assert once == twice, 'workflow patch must be idempotent'
    assert 'sh ./scripts/prepare_bret_sprites.sh' in once
    assert 'test -s src/generated/bret_sprites.c' in once
    assert once.index('prepare_bret_sprites.sh') < once.index('Configure portable verifier')
print('Fix39 Demo3f GitHub host generated-sprite preparation: PASS')
