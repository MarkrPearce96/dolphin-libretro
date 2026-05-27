# Upstream rebase + release process

Remotes: `origin` = dolphin-emu/dolphin (upstream, read-only), `fork` =
github.com/MarkrPearce96/dolphin-libretro (this repo). Work + releases live on `libretro`.

## Rebase onto upstream
1. `git fetch origin master`
2. `git rebase origin/master`   # resolve conflicts in Source/Core/DolphinLibretro/* + patched files
3. Rebuild x86_64 (see README) and smoke-test in RetroNest.
4. `git push fork libretro` (force-with-lease if rebased).

## Cut a release (triggers CI -> GitHub Release with the x86_64 dylib)
1. `git tag v2026.MM.DD` (numeric suffix vN for same-day re-cuts)
2. `git push fork v2026.MM.DD`
3. GitHub Actions (`.github/workflows/libretro_release.yml`) builds + publishes
   `dolphin_libretro.dylib.zip`. RetroNest's "Install Dolphin" picks it up.
