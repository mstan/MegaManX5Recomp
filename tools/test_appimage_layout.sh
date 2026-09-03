#!/usr/bin/env bash
# Verify that a built AppImage seeds its writable data directory correctly,
# without launching the game.
set -euo pipefail

appimage=${1:-}
[ -n "$appimage" ] || { echo "usage: $0 <AppImage>" >&2; exit 2; }
[ -x "$appimage" ] || { echo "not executable: $appimage" >&2; exit 1; }

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
expected_version=$(tr -d ' \t\r\n' < "$root/packaging/release/VERSION")
# shellcheck source=/dev/null
. "$root/packaging/release/app.conf"

work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT

data_dir=$(env "${ENV_PREFIX}_DATA_DIR=$work/data" "${ENV_PREFIX}_SEED_ONLY=1" \
    "$appimage" --appimage-extract-and-run)
[ -n "$data_dir" ] || { echo "AppRun printed no data dir" >&2; exit 1; }

fail=0
check_file() { [ -f "$data_dir/$1" ] || { echo "MISSING file: $1" >&2; fail=1; }; }
check_dir()  { [ -d "$data_dir/$1" ] || { echo "MISSING dir:  $1" >&2; fail=1; }; }

for d in saves cache mods mods/bundled assets bios; do check_dir "$d"; done
for f in game.toml input.ini START_HERE.txt LICENSE README.md \
         bios/openbios.bin bios/OpenBIOS.LICENSE .appimage-layout-version; do
    check_file "$f"
done

got_version=$(tr -d ' \t\r\n' < "$data_dir/.appimage-layout-version")
if [ "$got_version" != "$expected_version" ]; then
    echo "version marker mismatch: AppImage says '$got_version', VERSION says '$expected_version'" >&2
    fail=1
fi

legacy_packages=$(find "$data_dir/mods" -path "$data_dir/mods/packages" -type d | wc -l)
if [ "$legacy_packages" -ne 0 ]; then
    echo "seeded legacy mods/packages catalog" >&2
    fail=1
fi
package_dirs=$(find "$data_dir/mods/bundled" -mindepth 1 -maxdepth 1 -type d | wc -l)
manifests=$(find "$data_dir/mods/bundled" -mindepth 2 -maxdepth 2 -name manifest.toml | wc -l)
if [ "$package_dirs" -eq 0 ] || [ "$manifests" -ne "$package_dirs" ]; then
    echo "seeded mod catalog has $package_dirs package dir(s) and $manifests manifest(s)" >&2
    fail=1
fi

# Linux caches must use ELF .so shards under the Linux ABI namespace.
seeded_so=$(find "$data_dir/cache" -name '*.so' 2>/dev/null | wc -l)
if [ "$seeded_so" -eq 0 ] && [ "${ALLOW_NO_CACHE:-0}" != "1" ]; then
    echo "seeded cache holds no .so shards" >&2
    fail=1
fi
stray_dll=$(find "$data_dir/cache" -name '*.dll' | wc -l)
if [ "$stray_dll" -ne 0 ]; then
    echo "seeded cache contains $stray_dll Windows .dll shards; the Linux loader cannot use them" >&2
    fail=1
fi
arch_so=$(find "$data_dir/cache" -path '*/linux-x64/*' -name '*.so' | wc -l)
if [ "$seeded_so" -gt 0 ] && [ "$arch_so" -eq 0 ]; then
    echo "cache .so shards are not under a linux-x64 arch-abi directory" >&2
    fail=1
fi

# A retail BIOS, disc image, and memory card must never be in the payload.
stray=$(find "$data_dir" \( -iname 'SCPH*.BIN' -o -iname '*.cue' -o -iname '*.iso' \
        -o -iname '*.mcd' \) -print 2>/dev/null || true)
if [ -n "$stray" ]; then
    echo "payload contains files that must never ship:" >&2
    printf '  %s\n' $stray >&2
    fail=1
fi

# Seeding must be idempotent and preserve player-owned data.
env "${ENV_PREFIX}_DATA_DIR=$work/data" "${ENV_PREFIX}_SEED_ONLY=1" \
    "$appimage" --appimage-extract-and-run >/dev/null
user_shard=$data_dir/cache/.user-shard-probe
printf 'player-built\n' > "$user_shard"
env "${ENV_PREFIX}_DATA_DIR=$work/data" "${ENV_PREFIX}_SEED_ONLY=1" \
    "$appimage" --appimage-extract-and-run >/dev/null
if [ ! -f "$user_shard" ] || [ "$(cat "$user_shard")" != "player-built" ]; then
    echo "reseed destroyed a player-built cache entry" >&2
    fail=1
fi

echo "; user edit" >> "$data_dir/input.ini"
before=$(sha256sum "$data_dir/input.ini" | awk '{print $1}')
env "${ENV_PREFIX}_DATA_DIR=$work/data" "${ENV_PREFIX}_SEED_ONLY=1" \
    "$appimage" --appimage-extract-and-run >/dev/null
after=$(sha256sum "$data_dir/input.ini" | awk '{print $1}')
if [ "$before" != "$after" ]; then
    echo "reseed clobbered user-owned input.ini" >&2
    fail=1
fi

if [ "$fail" -ne 0 ]; then
    echo "AppImage layout test FAILED" >&2
    exit 1
fi
echo "AppImage layout test passed ($expected_version)"
