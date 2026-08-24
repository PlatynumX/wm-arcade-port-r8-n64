#!/usr/bin/env python3
from __future__ import annotations
import hashlib
import importlib.util
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
APPLY = ROOT / 'tools' / 'apply_fix39.py'
spec = importlib.util.spec_from_file_location('apply_fix39', APPLY)
mod = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(mod)

manifest = ROOT / 'assets' / 'fix39_sports_override' / 'SHA256SUMS.txt'
assert manifest.is_file()
expected = {}
for raw in manifest.read_text().splitlines():
    raw = raw.strip()
    if not raw:
        continue
    digest, name = raw.split(None, 1)
    expected[name.strip()] = digest
required = {'SPORTLO8.IMG', 'SPORTBK.IMG', 'SPRTBK.BDD', 'SPRTBK.BDB'}
assert set(expected) == required, (set(expected), required)
for name, want in expected.items():
    p = ROOT / 'assets' / 'fix39_sports_override' / name
    assert p.is_file(), name
    got = hashlib.sha256(p.read_bytes()).hexdigest()
    assert got == want, (name, got, want)

frontend = (
    '#!/usr/bin/env sh\n'
    'set -eu\n'
    'ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)\n'
    'ORIG="$ROOT/original/wwf-wrestlemania"\n'
    'SPORTS_SOURCE="$ORIG/IMG/SPORTLO8.IMG"\n'
    'python3 x \\\n'
    '    --module LADDERBMOD \\\n'
    '    --out y\n'
)
sports = (
    '#!/usr/bin/env sh\n'
    'set -eu\n'
    'ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)\n'
    'ORIG="$ROOT/original/wwf-wrestlemania"\n'
    'SPORTS_BG_SOURCE="$ORIG/IMG/SPORTBK.IMG"\n'
    'SPORTS_FONT_SOURCE="$ORIG/IMG/SGMD8.IMG"\n'
    'SPORTS_BG_OUT="$ROOT/src/generated/sports_background.c"\n'
    'python3 "$ROOT/tools/sports_background_bundle.py" \\\n'
    '    --source "$SPORTS_BG_SOURCE" \\\n'
    '    --out "$SPORTS_BG_OUT"\n'
)

with tempfile.TemporaryDirectory() as td:
    td = Path(td)
    f = td / 'prepare_frontend_assets.sh'
    s = td / 'prepare_sports_source_assets.sh'
    f.write_text(frontend)
    s.write_text(sports)
    mod.patch_frontend_assets_script(f)
    mod.patch_sports_source_assets_script(s)
    ft = f.read_text()
    st = s.read_text()
    assert 'FIX39_SPORTS_SOURCE="$ROOT/assets/fix39_sports_override/SPORTLO8.IMG"' in ft
    assert 'SPORTS_SOURCE="$FIX39_SPORTS_SOURCE"' in ft
    assert '--module slateBMOD' in ft
    assert '# FIX39 SPORTS BACKGROUND REGEN' in ft
    assert 'sh "$ROOT/scripts/prepare_sports_source_assets.sh"' in ft
    assert 'FIX39_SPORTS_BG_SOURCE="$ROOT/assets/fix39_sports_override/SPORTBK.IMG"' in st
    assert 'SPORTS_BG_SOURCE="$FIX39_SPORTS_BG_SOURCE"' in st
    assert 'SPORTS_BG_TOOL="$ROOT/tools/fix39_sports_background_bundle.py"' in st
    assert 'python3 "$SPORTS_BG_TOOL"' in st

    # Idempotence matters because old Fix39 working trees sometimes get re-run.
    before_f, before_s = ft, st
    mod.patch_frontend_assets_script(f)
    mod.patch_sports_source_assets_script(s)
    assert f.read_text() == before_f
    assert s.read_text() == before_s

# C2e hardware diagnosis guard: the bundled MIDWAY/PLATYNUMX object carries a
# replacement 29-color WIMP palette.  The old N64 generator ignored this palette
# and substituted BGNDPAL.ASM, making the small logo effectively black.
import struct
img = (ROOT / 'assets' / 'fix39_sports_override' / 'SPORTBK.IMG').read_bytes()
u16 = lambda off: struct.unpack_from('<H', img, off)[0]
u32 = lambda off: struct.unpack_from('<I', img, off)[0]
count = u16(0)
dir_off = u32(4)
images = []
for i in range(count):
    off = dir_off + i * 0x32
    name = img[off:off+8].split(b'\0', 1)[0].decode('ascii')
    images.append((name, u16(off + 26), u32(off + 28)))
first_pixel = min(x[2] for x in images)
pal_off = dir_off + count * 0x32
palettes = []
while pal_off + 0x1A <= len(img):
    raw = img[pal_off:pal_off+8].split(b'\0', 1)[0]
    try:
        name = raw.decode('ascii')
    except UnicodeDecodeError:
        break
    if not name:
        break
    n = u16(pal_off + 12)
    data_off = u32(pal_off + 14)
    if not (1 <= n <= 256) or data_off < 28 or data_off + n * 2 > first_pixel:
        break
    palettes.append((name, [u16(data_off + j * 2) & 0x7FFF for j in range(n)]))
    pal_off += 0x1A
base = min(x[1] for x in images)
midway = next(x for x in images if x[0] == 'MIDWAY')
midway_pal = palettes[midway[1] - base]
assert midway_pal[0] == 'SPORTBK'
assert len(midway_pal[1]) == 29
assert midway_pal[1][:9] == [0x1108, 0x1108, 0x28CD, 0x24AC, 0x1C8A, 0x1869, 0x1468, 0x0C25, 0x0402]
assert (ROOT / 'tools' / 'fix39_sports_background_bundle.py').is_file()

print('Fix39 V13e-c2e Sports WIMP-palette override regression: PASS')
