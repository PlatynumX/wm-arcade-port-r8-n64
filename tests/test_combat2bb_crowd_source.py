from pathlib import Path
import subprocess,tempfile,re
R=Path(__file__).resolve().parents[1]
asm=(R/'source_payload/arena/source/CROWD.ASM').read_text(errors='ignore')
assert re.search(r'NUMCROWD\s+equ\s+30',asm,re.I)
assert 'CROWD_ANIMS' in asm and 'DO_CROWD_CHEER' in asm
with tempfile.TemporaryDirectory() as td:
    td=Path(td); fs=td/'fs'; c=td/'crowd.c'; h=td/'crowd.h'
    subprocess.run(['python',str(R/'tools/fix39_crowd_assets.py'),'--source-pack',str(R/'source_payload/arena'),'--out-fs',str(fs),'--out-c',str(c),'--out-h',str(h)],check=True)
    s=c.read_text(); assert 'static const wm_crowd_person persons[]' in s
    persons=s.split('static const wm_crowd_person persons[]={',1)[1].split('};',1)[0]
    assert persons.count('{')==30
    assert len(list(fs.glob('*.bin')))>=100
print('Combat2BB crowd source translation: PASS')
