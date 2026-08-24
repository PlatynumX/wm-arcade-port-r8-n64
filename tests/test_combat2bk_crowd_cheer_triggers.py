from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'tools/fix39_crowd_renderer_patch.py').read_text()
for x in ['fix39_crowd_do_crowd_cheer','cheer2_script','cheer1_script','L3PN5B+FR8','R4SW4D+FR3','fix39_crowd_source_frame_event(0','i==20u||i==21u||i==23u','i==19u']:
    assert x in s,x
print('Combat2BL CROWD.ASM DO_CROWD_CHEER source trigger parity: PASS')
