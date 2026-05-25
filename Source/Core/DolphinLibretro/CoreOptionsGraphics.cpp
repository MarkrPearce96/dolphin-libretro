// SPDX-FileCopyrightText: 2026 Mark Pearce (RetroNest)
// SPDX-License-Identifier: GPL-3.0+

#include "CoreOptionsGraphics.h"

#include <cstdlib>
#include <cstring>

#ifndef CORE_OPTIONS_TEST_ONLY
#include "Common/Config/Config.h"
#include "Core/Config/GraphicsSettings.h"
#include "Core/Config/MainSettings.h"
#include "VideoCommon/VideoConfig.h"
#endif

namespace DolphinLibretro::CoreOptions::Graphics
{

void AppendDefinitions(std::vector<retro_core_option_v2_definition>& /*out*/)
{
    // Tasks 3-7 push one literal block per option here.
}

void Parse(retro_environment_t cb, Values& out)
{
    if (!cb) return;
    // Tasks 3-7 add per-option query() reads here.
    (void)out;
}

#ifndef CORE_OPTIONS_TEST_ONLY
void Apply(const Values& /*v*/)
{
    // Tasks 3-7 add Config::SetCurrent(...) calls here.
}
#endif

} // namespace DolphinLibretro::CoreOptions::Graphics
