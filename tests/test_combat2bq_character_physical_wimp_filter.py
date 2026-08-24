from pathlib import Path
p = Path(__file__).resolve().parents[1] / 'tools' / 'fix39_character_assets.py'
s = p.read_text()
start = s.index('def _available_frame_names(root):')
end = s.index('\ndef _filter_sequence_to_physical_assets', start)
body = s[start:end]
assert "glob('*.LOD')" not in body
assert 'bret_manifest.parse_lod' not in body
assert "path.suffix.lower()!='.img'" in body
assert 'wimpimg.parse_file(path)' in body
assert 'names.update(im.name.upper() for im in imgs)' in body
print('Combat2BQ physical WIMP frame authority regression: PASS')
