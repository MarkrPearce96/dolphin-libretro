# Upstream rebase + release process

Remotes (renamed 2026-07-03 to match the pcsx2/duckstation forks): `origin` =
github.com/MarkrPearce96/dolphin-libretro (this repo), `upstream` =
dolphin-emu/dolphin (read-only). Work + releases live on `libretro`.

## Rebase onto upstream
1. `git fetch upstream master`
2. `git rebase upstream/master`   # resolve conflicts in Source/Core/DolphinLibretro/* + patched files
3. Rebuild x86_64 (see README) and smoke-test in RetroNest.
4. `git push origin libretro` (force-with-lease if rebased).

## Cut a release (triggers CI -> GitHub Release with the x86_64 dylib)
1. `git tag v2026.MM.DD` (numeric suffix vN for same-day re-cuts)
2. `git push origin v2026.MM.DD`
3. GitHub Actions (`.github/workflows/libretro_release.yml`) builds + publishes
   `dolphin_libretro.dylib.zip`. RetroNest's "Install Dolphin" picks it up.
