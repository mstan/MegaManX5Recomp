#!/usr/bin/env bash
# Build the Linux x86_64 AppImage release. This is the X5 instance of the
# title-neutral AppImage flow used by X6; title identity lives in app.conf.
set -euo pipefail

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
orig_args=("$@")
version=""; out_dir=""; skip_build=0
build_dir=${BUILD_DIR:-"$root/build-appimage"}
_cores=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)
jobs=${BUILD_JOBS:-$(( _cores > 4 ? _cores - 2 : 2 ))}

while [ $# -gt 0 ]; do
    case "$1" in
        --version) version=$2; shift 2;;
        --out) out_dir=$2; shift 2;;
        --build-dir) build_dir=$2; shift 2;;
        --jobs) jobs=$2; shift 2;;
        --skip-build) skip_build=1; shift;;
        --nice) nice_level=$2; shift 2;;
        -h|--help)
            cat <<'EOF'
Usage: bash tools/package_appimage.sh [options]
  --version VERSION       override packaging/release/VERSION
  --out DIRECTORY         destination for the AppImage
  --build-dir DIRECTORY   CMake build directory (default: build-appimage)
  --jobs COUNT            parallel build jobs
  --skip-build            package an existing Linux build directory
  --nice LEVEL            build priority (default: 10; use 0 to disable)
EOF
            exit 0;;
        *) echo "unknown arg: $1" >&2; exit 2;;
    esac
done

nice_level=${nice_level:-10}
if [ "$nice_level" -gt 0 ] && [ "${RECOMP_APPIMAGE_RENICED:-0}" != "1" ] \
   && command -v nice >/dev/null 2>&1; then
    export RECOMP_APPIMAGE_RENICED=1
    exec nice -n "$nice_level" "$0" ${orig_args[@]+"${orig_args[@]}"}
fi

version=${version:-$(tr -d ' \t\r\n' < "$root/packaging/release/VERSION")}
[ -n "$version" ] || { echo "empty version" >&2; exit 1; }
# shellcheck source=/dev/null
. "$root/packaging/release/app.conf"
ARTIFACT_NAME=${ARTIFACT_NAME:-$EXE_NAME}
for v in APP_NAME EXE_NAME PAYLOAD_DIR DESKTOP_ID ENV_PREFIX ICON_SOURCE FRAMEWORK_DIR; do
    eval "val=\${$v:-}"
    [ -n "$val" ] || { echo "packaging/release/app.conf does not set $v" >&2; exit 1; }
done

to_unix_path() {
    case "$1" in
        [A-Za-z]:[/\\]*)
            command -v wslpath >/dev/null 2>&1 || { echo "wslpath is required for Windows paths" >&2; exit 2; }
            wslpath -u "$1";;
        *) printf '%s\n' "$1";;
    esac
}
[ -n "$out_dir" ] && out_dir=$(to_unix_path "$out_dir")
out_dir=${out_dir:-"$root/release-linux"}
mkdir -p -- "$out_dir"
out_dir=$(CDPATH= cd -- "$out_dir" && pwd)

# DrvFs does not reliably preserve the symlinks/exec bits required for AppDir.
stage_base=$build_dir
if [ -r /proc/version ] && grep -qiE 'microsoft|wsl' /proc/version; then
    case "$root" in /mnt/*) stage_base=${TMPDIR:-/tmp}/$PAYLOAD_DIR-appimage.$$;; esac
fi
appdir=$stage_base/AppDir
output=$out_dir/$ARTIFACT_NAME-$version-linux-x86_64.AppImage
tools_dir=${RECOMP_APPIMAGE_TOOLS:-"${XDG_CACHE_HOME:-$HOME/.cache}/recomp-appimage-tools"}
cleanup() {
    [ -n "${derived_toml:-}" ] && rm -f -- "$derived_toml"
    case "$stage_base" in /tmp/"$PAYLOAD_DIR"-appimage.*|"${TMPDIR:-/tmp}"/"$PAYLOAD_DIR"-appimage.*) rm -rf -- "$stage_base";; esac
}
trap cleanup EXIT

if [ -z "$(ls "$root"/generated/*_dispatch.c 2>/dev/null)" ]; then
    echo "Missing generated game sources (generated/*_dispatch.c)." >&2
    exit 1
fi

if [ -z "${SOURCE_DATE_EPOCH:-}" ]; then
    SOURCE_DATE_EPOCH=$(git -C "$root" log -1 --format=%ct 2>/dev/null || true)
    [ -n "$SOURCE_DATE_EPOCH" ] || SOURCE_DATE_EPOCH=$(stat -c %Y "$root/packaging/release/VERSION" 2>/dev/null || echo 0)
fi
export SOURCE_DATE_EPOCH
echo "version=$version SOURCE_DATE_EPOCH=$SOURCE_DATE_EPOCH"

fw=$root/$FRAMEWORK_DIR
# Shared release staging surface: tag derivation, cache selection, toolchain
# staging, and mod catalog checks live in psxrecomp, not this title packager.
# shellcheck source=/dev/null
. "$fw/tools/release_overlay_stage.sh"
psx_release_stage_init "$fw"
bios_build=${PSXRECOMP_BIOS_BUILD:-recompiler/build-linux}
# Bundle OpenBIOS without requiring a retail BIOS dump.
if [ -f "$fw/bios/openbios.bin" ] && [ ! -f "$fw/generated/OpenBIOS_dispatch.c" ]; then
    if [ ! -x "$fw/$bios_build/psxrecomp-bios" ]; then
        generator=Ninja; command -v ninja >/dev/null 2>&1 || generator="Unix Makefiles"
        cmake -S "$fw/recompiler" -B "$fw/$bios_build" -G "$generator" -DCMAKE_BUILD_TYPE=Release
        cmake --build "$fw/$bios_build" --target psxrecomp-bios -j "$jobs"
    fi
    (cd "$fw" && PSXRECOMP_BIOS_BUILD="$bios_build" tools/regen_bios.sh --config bios/OpenBIOS.toml)
fi

if [ "$skip_build" = 0 ]; then
    generator=Ninja; command -v ninja >/dev/null 2>&1 || generator="Unix Makefiles"
    cmake -S "$root" -B "$build_dir" -G "$generator" \
        -DCMAKE_BUILD_TYPE=Release -DPSX_SDL_BACKEND=SDL2 -DPSX_DEBUG_TOOLS=OFF \
        -DCMAKE_EXE_LINKER_FLAGS="-Wl,--build-id=none"
    cmake --build "$build_dir" --target psx-runtime -j "$jobs"
fi

elf=$build_dir/$EXE_NAME
[ -f "$elf" ] || elf=$build_dir/psx-runtime
[ -f "$elf" ] || { echo "no runtime ELF under $build_dir" >&2; exit 1; }
file -b "$elf" | grep -q ELF || { echo "$elf is not an ELF binary" >&2; exit 1; }

# X5 keeps the shipping config in the root game.toml. Cut only the audit
# metadata so turbo_loads=false and all runtime settings remain single-sourced.
player_toml=$root/packaging/release/game.toml
derived_toml=""
if [ ! -f "$player_toml" ]; then
    derived_toml=${TMPDIR:-/tmp}/player-game.$$.toml
    awk '/Audit-specific/ { exit } /^\[audit\]/ { exit } { print }' "$root/game.toml" > "$derived_toml"
    sed -i -e :a -e '/^[[:space:]]*$/{$d;N;ba' -e '}' "$derived_toml"
    player_toml=$derived_toml
fi
grep -Eq '^[[:space:]]*turbo_loads[[:space:]]*=[[:space:]]*false([[:space:]]|#|$)' "$player_toml" \
    || { echo "release game.toml must disable turbo_loads" >&2; exit 1; }

game_id=$(sed -n 's/^[[:space:]]*id[[:space:]]*=[[:space:]]*"\([^"]*\)".*/\1/p' "$player_toml" | head -1)
[ "$game_id" = "SLUS-01334" ] || { echo "expected SLUS-01334, got '${game_id:-none}'" >&2; exit 1; }
recompiler_bin=$fw/$bios_build/psxrecomp-game
[ -x "$recompiler_bin" ] || recompiler_bin=$fw/recompiler/build-linux/psxrecomp-game
if [ ! -x "$recompiler_bin" ]; then
    recompiler_build=$(dirname -- "$recompiler_bin")
    gen=Ninja
    command -v ninja >/dev/null 2>&1 || gen="Unix Makefiles"
    cmake -S "$fw/recompiler" -B "$recompiler_build" -G "$gen" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$recompiler_build" --target psxrecomp-game -j "$jobs"
fi
cg_tag=$(psx_overlay_cg_tag \
    --runtime-include "$fw/runtime/include" \
    --recompiler "$recompiler_bin" \
    --game-toml "$player_toml" \
    --flavor-from-build "$build_dir" \
    --runtime-target psx-runtime)
[ -n "$cg_tag" ] || { echo "could not compute codegen tag" >&2; exit 1; }
echo "game=$game_id  codegen tag=$cg_tag"
rm -rf -- "$appdir"
mkdir -p "$appdir/usr/bin" "$appdir/usr/share/$PAYLOAD_DIR"
payload=$appdir/usr/share/$PAYLOAD_DIR
install -m 0755 "$elf" "$appdir/usr/bin/$EXE_NAME"
sed -e "s|@VERSION@|$version|g" -e "s|@APP_NAME@|$APP_NAME|g" \
    -e "s|@EXE_NAME@|$EXE_NAME|g" -e "s|@PAYLOAD_DIR@|$PAYLOAD_DIR|g" \
    -e "s|@ENV_PREFIX@|$ENV_PREFIX|g" -e "s|@ARTIFACT_NAME@|$ARTIFACT_NAME|g" \
    "$root/packaging/linux/AppRun" > "$appdir/AppRun"
chmod 0755 "$appdir/AppRun"
install -m 0644 "$root/packaging/linux/$DESKTOP_ID.desktop" "$appdir/$DESKTOP_ID.desktop"

for tree in assets bios; do
    [ -d "$build_dir/$tree" ] || { echo "build did not stage $tree/" >&2; exit 1; }
    cp -a "$build_dir/$tree" "$payload/$tree"
done
psx_add_mod_catalog --build-path "$build_dir" --stage "$payload" \
                    --runtime-target psx-runtime
[ -f "$payload/bios/openbios.bin" ] || { echo "missing bundled OpenBIOS" >&2; exit 1; }
[ -f "$payload/bios/OpenBIOS.LICENSE" ] || { echo "missing OpenBIOS notice" >&2; exit 1; }
mkdir -p "$payload/licenses"
[ ! -f "$fw/runtime/licenses/libchdr-NOTICES.txt" ] || cp "$fw/runtime/licenses/libchdr-NOTICES.txt" "$payload/licenses/"

# --- prebuilt overlay cache + overlay toolchain ---------------------------
cache_src_root=${OVERLAY_CACHE_DIR:-"$root/build-linux-cache/cache"}
case "$cache_src_root" in
    *QUARANTINE*) echo "refusing quarantined overlay cache source: $cache_src_root" >&2; exit 1 ;;
esac
psx_add_overlay_cache --game-id "$game_id" \
                      --cache-src-root "$cache_src_root" \
                      --stage "$payload" \
                      --cg-tag "$cg_tag"
psx_add_overlay_toolchain --stage "$payload" \
                          --recomp-dir "$(dirname -- "$recompiler_bin")" \
                          --recomp-tools "$fw/tools" \
                          --recomp-include "$fw/runtime/include" \
                          --dl-cache "$tools_dir" \
                          --platform linux
cp "$player_toml" "$payload/game.toml"
cp "$root/packaging/release/input.ini" "$root/packaging/release/START_HERE.txt" "$payload/"
cp "$root/LICENSE" "$root/README.md" "$payload/"
ln -s "../share/$PAYLOAD_DIR/assets" "$appdir/usr/bin/assets"

if command -v magick >/dev/null 2>&1; then image_tool=magick
elif command -v convert >/dev/null 2>&1; then image_tool=convert
else echo "ImageMagick is required for the AppImage icon." >&2; exit 1; fi
"$image_tool" "$root/$ICON_SOURCE" -resize 240x240 -background transparent -gravity center -extent 256x256 "$appdir/$DESKTOP_ID.png"
ln -s "$DESKTOP_ID.png" "$appdir/.DirIcon"

linuxdeploy_url=https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
linuxdeploy_sha=421ca71d5c69ea97c6309276232990d43df1dcece0edfaa26bbf926ff96ed12e
appimagetool_url=https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
appimagetool_sha=a6d71e2b6cd66f8e8d16c37ad164658985e0cf5fcaa950c90a482890cb9d13e0
fetch_tool() {
    url=$1; sha=$2; dest=$3
    if [ ! -f "$dest" ] || [ "$(sha256sum "$dest" | awk '{print $1}')" != "$sha" ]; then
        curl -fL --retry 3 "$url" -o "$dest.tmp"
        printf '%s  %s\n' "$sha" "$dest.tmp" | sha256sum -c -
        mv "$dest.tmp" "$dest"
    fi
    chmod 0755 "$dest"
}
mkdir -p "$tools_dir"
linuxdeploy=$tools_dir/linuxdeploy-x86_64.AppImage
appimagetool=$tools_dir/appimagetool-x86_64.AppImage
fetch_tool "$linuxdeploy_url" "$linuxdeploy_sha" "$linuxdeploy"
fetch_tool "$appimagetool_url" "$appimagetool_sha" "$appimagetool"
export NO_STRIP=1
"$linuxdeploy" --appimage-extract-and-run --appdir "$appdir" --executable "$appdir/usr/bin/$EXE_NAME" --desktop-file "$appdir/$DESKTOP_ID.desktop" --icon-file "$appdir/$DESKTOP_ID.png"
find "$appdir" -exec touch -h -d "@$SOURCE_DATE_EPOCH" {} + 2>/dev/null || true
rm -f -- "$output"
ARCH=x86_64 "$appimagetool" --appimage-extract-and-run "$appdir" "$output"
chmod 0755 "$output"
sha256sum "$output"
echo "AppImage: $output"
