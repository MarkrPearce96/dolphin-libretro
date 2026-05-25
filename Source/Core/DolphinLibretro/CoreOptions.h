// SPDX-FileCopyrightText: 2026 Mark Pearce (RetroNest)
// SPDX-License-Identifier: GPL-3.0+
//
// SP6: libretro core-options aggregator for dolphin-libretro. Mirrors
// pcsx2-libretro/CoreOptions.{h,cpp}. Graphics is the only category in
// SP6; SP7 appends Audio + Core/Advanced modules here with no refactor.

#pragma once

#include "libretro.h"
#include "CoreOptionsGraphics.h"
#include <vector>

namespace DolphinLibretro::CoreOptions
{

struct Resolved
{
    Graphics::Values graphics{};
};

// Build (or return the cached) master option-definitions vector. First
// call concatenates each category's AppendDefinitions + the libretro
// terminator; the storage is a process-lifetime function-local static.
const std::vector<retro_core_option_v2_definition>& BuildDefinitions();

// Emit the schema to the host. Call once from retro_set_environment.
bool EmitCoreOptionsV2(retro_environment_t cb);

// Query the host for current user values. Call once at the top of
// retro_load_game (before BootCore). NULL/unknown values fall back to
// the Values member-initializer defaults.
Resolved ReadResolved(retro_environment_t cb);

} // namespace DolphinLibretro::CoreOptions
