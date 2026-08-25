import importlib.util
import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).parents[1]
p = ROOT / 'tools' / 'import_dcssound.py'
spec = importlib.util.spec_from_file_location('wm_import_dcssound', p)
m = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = m
spec.loader.exec_module(m)

# Build a synthetic source with the exact audited structural counts.  This
# exercises strict count checks without redistributing Midway's source file.
parts = ['triple_sndtab']
for i in range(0x303):
    if i == 0:
        parts.append(' .word 0,0 ; 0')
    else:
        parts.append(f' .word sp_smack|17,>80 ; {i:x}')
parts += ['triple_end', '', 'DEFAULT_SOUND_TABLE']
for i in range(59):
    parts.append(f' .word {i & 0xffff:04x}h')
parts.append('MASTER_SOUND_TABLE')
for i in range(59 * 9):
    parts.append(' .word DEFLT')
parts += [' SUBR table_sound', '', '#random_sound_tables']
for i in range(0x43):
    parts.append(f' .long #r{i}')
for i in range(0x43):
    parts += [f'#r{i}', f' .word 1,{i:04x}h,{(i+1):04x}h']

for n, name in enumerate(m.SPEECH_LABELS):
    if name == 'SPECIAL_LAST_STUFF':
        # Exact pinned-source shape: this is an inline SETUP_MOVE subtable,
        # not a normal reset/crowd/header speech table.
        parts += [' .word 6,0010h', name]
        for i in range(7):
            parts.append(f' .word {0x100 + i:03x}h')
    else:
        crowd_ptr = 'CROWD_FAIL' if n == 0 else '0'
        parts += [' .word 0', f' .long {crowd_ptr}', ' .word 0,0010h', name, ' .word 001h']
for name in m.CROWD_LABELS:
    parts += [' .word 0', name, ' .word 0100h,2,3,4']

dcs = '\n'.join(parts) + '\n'
equ = 'DEFLT .equ 8000h\n'
syms = m.parse_equ(equ + '\n' + dcs)
lines = dcs.splitlines()
labels = m.label_positions(lines)

triple = m.parse_triple(lines, labels, syms)
assert len(triple) == 0x303
assert triple[1][0] == ((16 << 8) | 17)
assert triple[1][1] == 0x80

default, master = m.parse_default_master(lines, labels, syms)
assert len(default) == 59
assert len(master) == 59 * 9

random = m.parse_random(lines, labels, syms)
assert len(random) == 0x43 and random[3] == [3, 4]

# Pinned DCSSOUND.ASM contains a source quirk at doink_stomp_v: the max
# index is 2 (so RNDRNG0 can only select three entries) but four WORD values
# physically follow the header.  The importer must preserve source execution
# semantics by importing only the reachable header+1 slice, not reject it.
quirk = [
    '#random_sound_tables',
    *[f' .long #q{i}' for i in range(0x43)],
]
for i in range(0x43):
    if i == 0x1a:
        quirk += [f'#q{i}', ' .word 2,022ch,022dh,0204h,020bh']
    else:
        quirk += [f'#q{i}', f' .word 1,{i:04x}h,{(i+1):04x}h']
qlines = '\n'.join(quirk).splitlines()
qlabels = m.label_positions(qlines)
qsyms = m.parse_equ('')
qrandom = m.parse_random(qlines, qlabels, qsyms)
assert qrandom[0x1a] == [0x22c, 0x22d, 0x204]

speech = m.parse_speech(lines, labels, syms)
assert len(speech) == len(m.SPEECH_LABELS)
assert speech[0].entry_words == 1 and speech[0].entries == [[1]]
assert speech[0].crowd == 'CROWD_FAIL'
special = speech[m.SPEECH_LABELS.index('SPECIAL_LAST_STUFF')]
assert special.reset == 0 and special.crowd is None
assert special.entry_words == 1 and len(special.entries) == 7
assert special.entries[0] == [[0x100]][0]

crowd = m.parse_crowd(lines, labels, syms)
assert len(crowd) == len(m.CROWD_LABELS)
assert crowd[0].entries[0] == (0x100, 2, 3, 4)

with tempfile.TemporaryDirectory() as td:
    td = pathlib.Path(td)
    dcs_path = td / 'DCSSOUND.ASM'
    equ_path = td / 'SOUND.EQU'
    out = td / 'arcade_sound_tables.c'
    dcs_path.write_text(dcs, encoding='latin-1')
    equ_path.write_text(equ, encoding='latin-1')
    rc = m.main if False else None
    generated = m.emit(triple, default, master, random, speech, crowd, dcs_path.name)
    out.write_text(generated, encoding='utf-8')
    subprocess.run([
        'cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
        '-I', str(ROOT / 'include'), '-c', str(out), '-o', str(td / 'tables.o')
    ], check=True)



# R4 regression: DCSSOUND crowd flags are not defined in SOUND.EQU.
# They come from the source's direct GAME.EQU include.  The importer must load
# that include context instead of hard-coding C_LONG/C_RANDOM/etc.
with tempfile.TemporaryDirectory() as td:
    td = pathlib.Path(td)
    game = td / 'GAME.EQU'
    snd = td / 'SOUND.EQU'
    dcs_path = td / 'DCSSOUND.ASM'
    game.write_text(
        'C_LONG .equ 1\n'
        'C_SHORT .equ 0\n'
        'C_OVERIDE .equ 2\n'
        'C_RANDOM .equ 4\n',
        encoding='latin-1'
    )
    snd.write_text('DEFLT .equ 8000h\n', encoding='latin-1')
    dcs_path.write_text(
        '.include "GAME.EQU"\n'
        '.include "SOUND.EQU"\n'
        '.word 0\n'
        'SETUP_TABLE\n'
        '.word 0100h,0002h,C_LONG|C_OVERIDE,0\n',
        encoding='latin-1'
    )
    include_syms = m.load_source_symbols(dcs_path, snd)
    assert include_syms['C_LONG'] == 1
    assert include_syms['C_SHORT'] == 0
    assert include_syms['C_OVERIDE'] == 2
    assert include_syms['C_RANDOM'] == 4
    ilines = dcs_path.read_text(encoding='latin-1').splitlines()
    ilabels = m.label_positions(ilines)
    # Direct expression check mirrors the failing pinned crowd-table field.
    vals = m.directive_values(
        '.word 0100h,0002h,C_LONG|C_OVERIDE,0',
        'word', include_syms
    )
    assert vals == [0x100, 2, 3, 0]

print('importer: PASS')
