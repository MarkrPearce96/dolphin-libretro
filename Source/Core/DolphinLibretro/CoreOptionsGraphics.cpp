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

void AppendDefinitions(std::vector<retro_core_option_v2_definition>& out)
{
    // ── General sub-tab ──
    out.push_back({
        "dolphin_aspect_ratio",
        "Aspect Ratio",
        nullptr,
        "Display aspect ratio. Auto matches the game's native aspect; "
        "Stretch fills the window.",
        nullptr,
        nullptr,
        {
            { "Auto",    "Auto" },
            { "16:9",    "Force 16:9" },
            { "4:3",     "Force 4:3" },
            { "Stretch", "Stretch to Window" },
            { nullptr,   nullptr },
        },
        "Auto",
    });
    out.push_back({
        "dolphin_vsync",
        "V-Sync",
        nullptr,
        "Synchronizes output to the display refresh rate. Reduces tearing.",
        nullptr,
        nullptr,
        {
            { "enabled",  "Enabled" },
            { "disabled", "Disabled" },
            { nullptr,    nullptr },
        },
        "enabled",
    });
    out.push_back({
        "dolphin_precision_frame_timing",
        "Precision Frame Timing",
        nullptr,
        "Uses high-resolution timers and busy-waiting for improved frame "
        "pacing. Slightly higher power use.",
        nullptr,
        nullptr,
        {
            { "enabled",  "Enabled" },
            { "disabled", "Disabled" },
            { nullptr,    nullptr },
        },
        "enabled",
    });
    out.push_back({
        "dolphin_shader_compilation",
        "Shader Compilation",
        nullptr,
        "How shaders are compiled. Ubershader modes reduce stutter at a GPU "
        "cost; Skip Drawing is for debugging.",
        nullptr,
        nullptr,
        {
            { "Specialized",           "Specialized (Default)" },
            { "Exclusive Ubershaders", "Exclusive Ubershaders" },
            { "Hybrid Ubershaders",    "Hybrid Ubershaders" },
            { "Skip Drawing",          "Skip Drawing" },
            { nullptr,                 nullptr },
        },
        "Specialized",
    });
    out.push_back({
        "dolphin_wait_for_shaders",
        "Compile Shaders Before Starting",
        nullptr,
        "Pre-compile the shader pipeline before launching. Slower start, "
        "smoother first minutes of gameplay.",
        nullptr,
        nullptr,
        {
            { "enabled",  "Enabled" },
            { "disabled", "Disabled" },
            { nullptr,    nullptr },
        },
        "disabled",
    });
}

void Parse(retro_environment_t cb, Values& out)
{
    if (!cb) return;
    auto query = [&cb](const char* key) -> const char* {
        retro_variable var{};
        var.key = key;
        if (cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            return var.value;
        return nullptr;
    };
    auto parse_bool = [](const char* s) { return s && std::strcmp(s, "enabled") == 0; };

    // ── General ──
    if (const char* v = query("dolphin_aspect_ratio"))  out.general.aspect_ratio = v;
    if (const char* v = query("dolphin_vsync"))          out.general.vsync = parse_bool(v);
    if (const char* v = query("dolphin_precision_frame_timing"))
        out.general.precision_frame_timing = parse_bool(v);
    if (const char* v = query("dolphin_shader_compilation")) out.general.shader_compilation = v;
    if (const char* v = query("dolphin_wait_for_shaders"))   out.general.wait_for_shaders = parse_bool(v);
}

#ifndef CORE_OPTIONS_TEST_ONLY
void Apply(const Values& v)
{
    // ── General ──
    {
        const std::string& ar = v.general.aspect_ratio;
        AspectMode mode = AspectMode::Auto;
        if      (ar == "16:9")    mode = AspectMode::ForceWide;
        else if (ar == "4:3")     mode = AspectMode::ForceStandard;
        else if (ar == "Stretch") mode = AspectMode::Stretch;
        Config::SetCurrent(Config::GFX_ASPECT_RATIO, mode);
    }
    Config::SetCurrent(Config::GFX_VSYNC, v.general.vsync);
    Config::SetCurrent(Config::MAIN_PRECISION_FRAME_TIMING, v.general.precision_frame_timing);
    {
        const std::string& sc = v.general.shader_compilation;
        ShaderCompilationMode mode = ShaderCompilationMode::Synchronous;
        if      (sc == "Exclusive Ubershaders") mode = ShaderCompilationMode::SynchronousUberShaders;
        else if (sc == "Hybrid Ubershaders")    mode = ShaderCompilationMode::AsynchronousUberShaders;
        else if (sc == "Skip Drawing")          mode = ShaderCompilationMode::AsynchronousSkipRendering;
        Config::SetCurrent(Config::GFX_SHADER_COMPILATION_MODE, mode);
    }
    Config::SetCurrent(Config::GFX_WAIT_FOR_SHADERS_BEFORE_STARTING, v.general.wait_for_shaders);
}
#endif

} // namespace DolphinLibretro::CoreOptions::Graphics
