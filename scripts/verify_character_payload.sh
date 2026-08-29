#!/usr/bin/env sh
set -eu

ROOT="${1:-filesystem/fix39_chars}"

test -d "$ROOT" || {
    echo "ERROR: missing wrestler payload root: $ROOT" >&2
    exit 1
}

# R37N3 exact source-VM physical WIMP corpus at the pinned historical source.
expected_total=5129
actual_total="$(find "$ROOT" -type f -name '*.bin' | wc -l | tr -d '[:space:]')"

echo "Wrestler payload total: ${actual_total}/${expected_total}"
test "$actual_total" = "$expected_total" || {
    echo "ERROR: source-VM wrestler payload mismatch: ${actual_total}/${expected_total}" >&2
    exit 1
}

for spec in \
    "0:619" \
    "1:647" \
    "2:609" \
    "3:603" \
    "4:659" \
    "5:696" \
    "6:643" \
    "8:653"
do
    rid="${spec%%:*}"
    expected="${spec##*:}"
    dir="$ROOT/$rid"

    test -d "$dir" || {
        echo "ERROR: wrestler directory missing: $rid" >&2
        exit 1
    }

    actual="$(find "$dir" -maxdepth 1 -type f -name '*.bin' | wc -l | tr -d '[:space:]')"
    printf '  wrestler %s: %s/%s\n' "$rid" "$actual" "$expected"

    test "$actual" = "$expected" || {
        echo "ERROR: wrestler $rid source-VM frame count mismatch" >&2
        exit 1
    }
done

dirs="$(find "$ROOT" -mindepth 1 -maxdepth 1 -type d -printf '%f\n' 2>/dev/null | sort -n | tr '\n' ' ' | sed 's/ $//')"
if [ -n "$dirs" ]; then
    test "$dirs" = "0 1 2 3 4 5 6 8" || {
        echo "ERROR: unexpected wrestler directory set: $dirs" >&2
        exit 1
    }
fi

echo "R37N3 source-VM wrestler payload contract: PASS (5129/5129)"
