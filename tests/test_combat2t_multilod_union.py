#!/usr/bin/env python3
import importlib.util, pathlib, tempfile, sys
root=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('gen', root/'tools/fix39_character_assets.py')
gen=importlib.util.module_from_spec(spec); sys.modules[spec.name]=gen; spec.loader.exec_module(gen)
class BM:
    @staticmethod
    def parse_lod(p):
        return {'A':'ONE.IMG'} if p.name.lower()=='one.lod' else {'B':'TWO.IMG'}
gen.bret_manifest=BM
with tempfile.TemporaryDirectory() as td:
    d=pathlib.Path(td); img=d/'IMG'; img.mkdir(); (img/'ONE.LOD').write_text('x'); (img/'two.lod').write_text('x')
    used,m=gen.find_lod(d,['A','B'])
    assert m['A']=='ONE.IMG' and m['B']=='TWO.IMG'
    assert len(used)==2
print('Combat2T IMG/ multi-LOD union regression: PASS')


def test_historical_lods_are_scanned_from_img():
    import inspect
    src = inspect.getsource(gen.find_lod)
    assert "root / 'IMG'" in src
    assert "loddir.glob('*.LOD')" in src


class _Im:
    def __init__(self,name): self.name=name
class WI:
    @staticmethod
    def parse_file(p):
        if p.name.lower()=='extra.img':
            return b'',None,[_Im('C')],[]
        raise ValueError('not WIMP')
gen.wimpimg=WI
with tempfile.TemporaryDirectory() as td:
    d=pathlib.Path(td); img=d/'IMG'; img.mkdir()
    (img/'ONE.LOD').write_text('x'); (img/'EXTRA.IMG').write_bytes(b'x')
    used,m=gen.find_lod(d,['A','C'])
    assert m['A']=='ONE.IMG' and m['C']=='EXTRA.IMG'
print('Combat2T direct-IMG fallback regression: PASS')
