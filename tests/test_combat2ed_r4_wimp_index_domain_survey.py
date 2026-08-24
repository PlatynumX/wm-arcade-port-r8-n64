#!/usr/bin/env python3
import importlib.util,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("assets",ROOT/"tools/fix39_character_assets.py")
m=importlib.util.module_from_spec(spec); sys.modules[spec.name]=m; spec.loader.exec_module(m)
assert m._classify_palette_index_domain(bytes([0,1,2,63]),64)[0]=="both"
assert m._classify_palette_index_domain(bytes([0,1,64]),64)[0]=="one-based-required"
assert m._classify_palette_index_domain(bytes([0,65]),64)[0]=="neither"
print("Combat2ED-R4 WIMP index-domain survey classifier: PASS")
