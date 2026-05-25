// SPDX-FileCopyrightText: 2026 Mark Pearce (RetroNest)
// SPDX-License-Identifier: GPL-3.0+

#include "CoreOptions.h"

#ifdef CORE_OPTIONS_TEST_ONLY
#include <cstdarg>
#include <cstdio>
namespace { void TestLog(enum retro_log_level, const char* fmt, ...) {
    std::va_list ap; va_start(ap, fmt); std::vfprintf(stderr, fmt, ap);
    std::fputc('\n', stderr); va_end(ap);
} }
#define CORE_OPTIONS_LOG(level, ...) TestLog(level, __VA_ARGS__)
#else
#include "LibretroEnvironment.h"   // DolphinLibretro::Environment::Log
#define CORE_OPTIONS_LOG(level, ...) DolphinLibretro::Environment::Log(level, __VA_ARGS__)
#endif

namespace DolphinLibretro::CoreOptions
{

const std::vector<retro_core_option_v2_definition>& BuildDefinitions()
{
    static const std::vector<retro_core_option_v2_definition> kAll = [] {
        std::vector<retro_core_option_v2_definition> v;
        v.reserve(64);  // ~50 Graphics + terminator + headroom for SP7
        Graphics::AppendDefinitions(v);
        // libretro terminator — must be the final entry. Only the first
        // values[] element is named; the rest of the array is zero-init'd
        // by aggregate rules, which is the required all-null terminator.
        v.push_back({
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            {{nullptr, nullptr}},
            nullptr
        });
        return v;
    }();
    return kAll;
}

bool EmitCoreOptionsV2(retro_environment_t cb)
{
    if (!cb) return false;
    retro_core_options_v2 opts{};
    opts.categories  = nullptr;  // uncategorized; host adapter groups via SettingDef.category
    opts.definitions = const_cast<retro_core_option_v2_definition*>(
        BuildDefinitions().data());
    const bool ok = cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &opts);
    if (!ok) {
        CORE_OPTIONS_LOG(RETRO_LOG_WARN,
            "[CoreOptions] Host does not support core-option categories "
            "(options still registered; GET_VARIABLE will work)");
    }
    return ok;
}

Resolved ReadResolved(retro_environment_t cb)
{
    Resolved r{};
    if (!cb) return r;
    Graphics::Parse(cb, r.graphics);
    return r;
}

} // namespace DolphinLibretro::CoreOptions
