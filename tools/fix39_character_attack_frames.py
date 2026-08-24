#!/usr/bin/env python3
from __future__ import annotations
import argparse,re
from pathlib import Path
CHARS=[(0,'hrt'),(1,'rzr'),(2,'und'),(3,'yok'),(4,'shn'),(5,'bam'),(6,'dnk'),(8,'lex')]
# Source AMODE_GRAPPLE is a zero-damage grapple contact gate.  The N64 combat
# port represents that semantic with GRABHOLD; do not emit a nonexistent enum.
MODE_MAP={'AMODE_GRAPPLE':'WM_AMODE_GRABHOLD'}
ATTACK_RE=re.compile(r'ANI_ATTACK_ON(_Z)?\s*,\s*(AMODE_[A-Z0-9_]+)\s*,\s*([^;]+)',re.I)
FRAME_RE=re.compile(r'\bWL\s+[^,]+,\s*([A-Z][A-Z0-9_]*\+FR\d+)\b',re.I)
OFF_RE=re.compile(r'ANI_ATTACK_OFF\b',re.I)
SUBR_RE=re.compile(r'^\s*SUBR(?:P)?\s+#?([A-Za-z_][A-Za-z0-9_]*)\b',re.I)
def atom(s):
 s=s.strip();m=re.fullmatch(r'([+-]?)([0-9A-Fa-f]+)h',s)
 if m:
  v=int(m.group(2),16);return -v if m.group(1)=='-' else v
 return int(s,0)
def expr(s):return sum(atom(p) for p in re.findall(r'[+-]?[^+-]+',s.replace(' ','')))
def files_for(root,pfx):
 out=[]
 for p in root.glob('*.ASM'):
  t=p.read_text(errors='replace')
  if re.search(rf'^\s*SUBR(?:P)?\s+#?{re.escape(pfx)}_[A-Za-z0-9_]+',t,re.I|re.M):out.append(p)
 return out
def parse(text):
 rows=[];active=None
 for raw in text.splitlines():
  line=raw.split(';',1)[0].strip();m=ATTACK_RE.search(line)
  if m:
   vals=[x.strip() for x in m.group(3).split(',')];need=6 if m.group(1) else 4
   try: nums=[expr(v) for v in vals[:need]]
   except Exception: active=None;continue
   active=(bool(m.group(1)),m.group(2).upper(),nums);continue
  if OFF_RE.search(line):active=None;continue
  if active:
   fm=FRAME_RE.search(line)
   if fm:rows.append((fm.group(1).upper(),)+active)
 seen=set();out=[]
 for r in rows:
  if r[0] not in seen:seen.add(r[0]);out.append(r)
 return out
def emit(root):
 lines=['/* generated from all historical wrestler sequence ASM; do not edit */','#ifndef WM_ARCADE_CHARACTER_ATTACK_FRAMES_GENERATED_H','#define WM_ARCADE_CHARACTER_ATTACK_FRAMES_GENERATED_H','#define WM_FIX39_CHARACTER_ATTACK_FRAMES_GENERATED 1']
 counts={}
 for rid,pfx in CHARS:
  rows=[]
  for p in files_for(root,pfx): rows.extend(parse(p.read_text(errors='replace')))
  seen=set();rows=[r for r in rows if not (r[0] in seen or seen.add(r[0]))]
  counts[rid]=len(rows);lines.append(f'static const wm_arcade_source_attack_frame_t wm_char_{rid}_attack_frames[]={{')
  for frame,uses_z,mode,nums in rows:
   cm=MODE_MAP.get(mode,'WM_'+mode)
   if uses_z:x,y,z,w,h,d=nums;lines.append(f'{{"{frame}",1,{cm},{x},{y},{z},{w},{h},{d}}},')
   else:x,y,w,h=nums;lines.append(f'{{"{frame}",0,{cm},{x},{y},-40,{w},{h},80}},')
  lines+=['};',f'#define WM_CHAR_{rid}_ATTACK_COUNT {len(rows)}u']
 lines+=['#endif',''];return '\n'.join(lines),counts
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--root',type=Path);ap.add_argument('--out',type=Path);ap.add_argument('--self-test',action='store_true');a=ap.parse_args()
 if a.self_test: assert len(CHARS)==8 and expr('10h+2')==18;print('Fix39 all-character attack generator self-test: PASS');return
 if not a.root or not a.out:ap.error('--root and --out required')
 t,c=emit(a.root);a.out.write_text(t);print('generated character attack frames:',c)
if __name__=='__main__':main()
