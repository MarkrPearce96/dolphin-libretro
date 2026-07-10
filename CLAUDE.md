# dolphin-libretro — Claude Instructions

Private Dolphin fork with a libretro shell (`Source/Core/DolphinLibretro/`),
loaded in-process by RetroNest. Branch: **`libretro`** (not main), remote
`origin` = `MarkrPearce96/dolphin-libretro` (private, standalone),
`upstream` = `dolphin-emu/dolphin`.

## Build + arch policy
Local build dirs: `build-libretro` (arm64) + `build-libretro-x86_64`.
Local deploys are **universal**; CI releases are x86_64-only (mirrors
pcsx2 while the daily driver is the Rosetta app). x86_64 CMake invocations
MUST use `arch -x86_64 /usr/local/bin/cmake` — bare `arch -x86_64 cmake`
resolves to the arm64 Homebrew cmake and dies with "Bad CPU type". Never
pipe build output (masks the exit status).

## Deploy (one script)
```sh
Source/Core/DolphinLibretro/tools/deploy.sh
```
Lipos both build dirs into a universal `dolphin_libretro.dylib` and stages
it + the `dolphin_libretro_resources/Sys` tree into
`~/Documents/RetroNest/emulators/libretro/cores/`.

## Releases (CI)
`.github/workflows/libretro_release.yml` on tags (`v2026.MM.DD[.n]`).
Builds with vendored fmt (`USE_SYSTEM_FMT=OFF` — brew fmt 12.x breaks
consteval asserts), zips the Sys tree, and is **self-contained**:
dylibbundler copies deps into `dolphin_libretro_libs/` with flat
`@loader_path/<lib>` refs + ad-hoc signing (otool guard enforces no bare
Homebrew paths).

## RetroNest contract package
`Source/Core/DolphinLibretro/retronest-libretro/` is a VENDORED COPY of
`RetroNest-Project/vendor/retronest-libretro/`. NEVER edit it here — edit
the canonical package and run its `sync.sh`; the build fails on checksum
drift (`check-drift.sh` + `MANIFEST.sha256`).

## Settings options
RetroNest renders its Dolphin settings pages FROM this core's declared
options (`CoreOptions*.cpp` → `SET_CORE_OPTIONS_V2`). Changing option
keys/values/defaults here flows into RetroNest automatically after a
rebuild + re-probe; follow `retronest-libretro/docs/option-style-guide.md`.

## Updating from upstream (carries patches — a sync is real work)
Unlike the stock `mgba-libretro` mirror, this fork carries RetroNest source
patches (NSView/Metal handoff, the `RETRONEST_ENVIRONMENT_*` contract,
CoreOptions), so an upstream sync **can and will conflict**. `upstream` =
`dolphin-emu/dolphin`, branch `libretro`, release arch **x86_64**. Full
provenance/recipe: `UPSTREAM-UPDATE.md`.
```sh
git fetch upstream
git merge upstream/master        # onto the 'libretro' branch; resolve conflicts
                                 # where upstream touched our patched code
# if the contract package changed, re-sync from RetroNest-Project:
#   ./vendor/retronest-libretro/sync.sh   (build fails on drift otherwise)
# REBUILD LOCALLY + TEST IN RETRONEST — not just "compiles": confirm rendering
# (NSView handoff), GC+Wii controllers, settings schema still work.
git push origin libretro
git tag v2026.MM.DD && git push origin v2026.MM.DD   # CI rebuilds + republishes
```
Only sync when you actually want an upstream fix/feature — each sync costs
conflict-resolution + a full retest. **Dolphin save states are arch-sensitive**
(arm64 states won't load under x86_64), so make fresh states after arch changes.
