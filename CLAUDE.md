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

## Upstream rebase
See `UPSTREAM-UPDATE.md`. Dolphin save states are arch-sensitive (arm64
states won't load under x86_64).
