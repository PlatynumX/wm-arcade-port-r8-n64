#!/usr/bin/env bash
set -euo pipefail
CC=${CC:-gcc}
CFLAGS=${CFLAGS:--std=c11 -Wall -Wextra -Werror -pedantic -O2}
BUILD=${BUILD:-.test-build}
rm -rf "$BUILD"
mkdir -p "$BUILD"
for f in wm_arcade_*.c; do
  "$CC" $CFLAGS -c "$f" -o "$BUILD/${f%.c}.o"
done
ar rcs "$BUILD/libwmcombat.a" "$BUILD"/*.o
for n in $(seq 1 25); do
  "$CC" $CFLAGS "test_combat_stage${n}.c" "$BUILD/libwmcombat.a" -o "$BUILD/test${n}"
  "$BUILD/test${n}"
done
