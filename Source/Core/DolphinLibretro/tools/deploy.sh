#!/usr/bin/env bash
# SP8: deploy the universal Dolphin libretro core + Sys data into RetroNest.
# Replaces the manual lipo + cp -R dance. Run from anywhere.
#
#   tools/deploy.sh [CORES_DIR] [APP_RESOURCES_DIR]
#
# Defaults match the dev machine layout (see memory/dolphin-libretro-build-setup).
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"  # -> dolphin-libretro/
ARM64_DYLIB="$REPO_ROOT/build-libretro/Source/Core/DolphinLibretro/dolphin_libretro.dylib"
X86_DYLIB="$REPO_ROOT/build-libretro-x86_64/Source/Core/DolphinLibretro/dolphin_libretro.dylib"

CORES_DIR="${1:-$HOME/Documents/RetroNest/emulators/libretro/cores}"
APP_RESOURCES="${2:-$HOME/Documents/Projects/RetroNest-Project/cpp/build-x86_64/RetroNest.app/Contents/Resources}"

for f in "$ARM64_DYLIB" "$X86_DYLIB"; do
    [ -f "$f" ] || { echo "ERROR: missing $f — build both arches first." >&2; exit 1; }
done

mkdir -p "$CORES_DIR"
echo "lipo -> $CORES_DIR/dolphin_libretro.dylib"
lipo -create "$ARM64_DYLIB" "$X86_DYLIB" -output "$CORES_DIR/dolphin_libretro.dylib"

echo "Sys -> $APP_RESOURCES/Sys"
[ -d "$REPO_ROOT/Data/Sys" ] || { echo "ERROR: Data/Sys not found at $REPO_ROOT/Data/Sys — wrong repo root? Invoke as ./tools/deploy.sh or with an absolute path." >&2; exit 1; }
mkdir -p "$APP_RESOURCES"
# Idempotent: remove any prior Sys so cp doesn't nest a Sys/Sys on re-runs.
# The guard above ensures we never delete the installed Sys without a valid source.
rm -rf "$APP_RESOURCES/Sys"
cp -R "$REPO_ROOT/Data/Sys" "$APP_RESOURCES/Sys"

echo "Deployed:"
lipo -info "$CORES_DIR/dolphin_libretro.dylib"
