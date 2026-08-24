from pathlib import Path
s = (Path(__file__).resolve().parents[1] / 'termux_fix39_build.sh').read_text()
for token in [
    'LOCAL_LOG="$DOWNLOAD_DIR/fix39-v13e-combat2k-${RUN_STAMP}.log"',
    'exec > >(tee -a "$LOCAL_LOG") 2>&1',
    'trap on_err ERR',
    '=== FIX39 LOCAL BUILD FAILED ===',
    'FULL LOCAL LOG: %s',
]:
    assert token in s, token
print('Combat2e persistent local failure logging: PASS')
