// SPDX-FileCopyrightText: 2026 Mark Pearce (RetroNest)
// SPDX-License-Identifier: GPL-3.0+

#include "CoreOptionsGraphics.h"

#include <cstdlib>
#include <cstring>
#include <map>

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
    // ── Enhancements sub-tab ──
    out.push_back({
        "dolphin_internal_resolution", "Internal Resolution", nullptr,
        "Render scale relative to native (1x = 640x528; 6x is roughly 4K).",
        nullptr, nullptr,
        {
            { "Auto", "Auto (Window Size)" },
            { "1x", "Native (1x)" }, { "2x", "2x" }, { "3x", "3x" },
            { "4x", "4x" }, { "5x", "5x" }, { "6x", "6x (4K)" },
            { "7x", "7x" }, { "8x", "8x" },
            { nullptr, nullptr },
        },
        "1x",
    });
    out.push_back({
        "dolphin_antialiasing", "Anti-Aliasing", nullptr,
        "Reduces aliasing on edges. SSAA is far more demanding than MSAA "
        "but also anti-aliases shader effects.",
        nullptr, nullptr,
        {
            { "None", "None" },
            { "2x MSAA", "2x MSAA" }, { "4x MSAA", "4x MSAA" }, { "8x MSAA", "8x MSAA" },
            { "2x SSAA", "2x SSAA" }, { "4x SSAA", "4x SSAA" }, { "8x SSAA", "8x SSAA" },
            { nullptr, nullptr },
        },
        "None",
    });
    out.push_back({
        "dolphin_texture_filtering", "Texture Filtering", nullptr,
        "Sharpens distant textures (anisotropic) and optionally forces a "
        "fixed magnification filter.",
        nullptr, nullptr,
        {
            { "Default", "Default" },
            { "1x Anisotropic", "1x Anisotropic" },
            { "2x Anisotropic", "2x Anisotropic" },
            { "4x Anisotropic", "4x Anisotropic" },
            { "8x Anisotropic", "8x Anisotropic" },
            { "16x Anisotropic", "16x Anisotropic" },
            { "Force Nearest and 1x Anisotropic", "Force Nearest and 1x Anisotropic" },
            { "Force Linear and 1x Anisotropic", "Force Linear and 1x Anisotropic" },
            { "Force Linear and 2x Anisotropic", "Force Linear and 2x Anisotropic" },
            { "Force Linear and 4x Anisotropic", "Force Linear and 4x Anisotropic" },
            { "Force Linear and 8x Anisotropic", "Force Linear and 8x Anisotropic" },
            { "Force Linear and 16x Anisotropic", "Force Linear and 16x Anisotropic" },
            { nullptr, nullptr },
        },
        "Default",
    });
    out.push_back({
        "dolphin_output_resampling", "Output Resampling", nullptr,
        "Algorithm used to resample the rendered image to the window size.",
        nullptr, nullptr,
        {
            { "Default", "Default" },
            { "Bilinear", "Bilinear" },
            { "Bicubic B-Spline", "Bicubic: B-Spline" },
            { "Bicubic Mitchell-Netravali", "Bicubic: Mitchell-Netravali" },
            { "Bicubic Catmull-Rom", "Bicubic: Catmull-Rom" },
            { "Sharp Bilinear", "Sharp Bilinear" },
            { "Area Sampling", "Area Sampling" },
            { nullptr, nullptr },
        },
        "Default",
    });
    out.push_back({
        "dolphin_stereo_mode", "Stereoscopic 3D Mode", nullptr,
        "3D-stereoscopic rendering mode. Off disables stereo entirely.",
        nullptr, nullptr,
        {
            { "Off", "Off" },
            { "Side-by-Side", "Side-by-Side" },
            { "Top-and-Bottom", "Top-and-Bottom" },
            { "Anaglyph", "Anaglyph" },
            { "HDMI 3D", "HDMI 3D" },
            { "Passive", "Passive" },
            { nullptr, nullptr },
        },
        "Off",
    });
    out.push_back({
        "dolphin_scaled_efb_copy", "Scaled EFB Copy", nullptr,
        "Resize EFB copies to match the rendering scale. Required for high internal resolutions to look right.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "enabled",
    });
    out.push_back({
        "dolphin_per_pixel_lighting", "Per-Pixel Lighting", nullptr,
        "Higher-quality lighting at a small performance cost.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_widescreen_hack", "Widescreen Hack", nullptr,
        "Force 4:3 games to render in widescreen by hacking the projection matrix. Can produce artifacts.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_force_true_color", "Force 24-Bit Color", nullptr,
        "Force higher-precision color output. Reduces banding on gradients.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "enabled",
    });
    out.push_back({
        "dolphin_disable_fog", "Disable Fog", nullptr,
        "Skip rendering fog effects.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_arbitrary_mipmap_detection", "Arbitrary Mipmap Detection", nullptr,
        "Detect when a game uses mipmaps as separate images rather than true LODs.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "enabled",
    });
    out.push_back({
        "dolphin_disable_copy_filter", "Disable Copy Filter", nullptr,
        "Disable the post-process copy-filter pass. Reduces blur some games apply.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_hdr_output", "HDR Post-Processing", nullptr,
        "Output in HDR when the display supports it.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_stereo_swap_eyes", "Swap Eyes", nullptr,
        "Swap the left and right eye images.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_stereo_per_eye_full", "Full Resolution Per Eye", nullptr,
        "Render each eye at the full internal resolution instead of half. Doubles GPU cost.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
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
    auto parse_int = [](const char* s, int fb) {
        if (!s) return fb; char* e = nullptr; long n = std::strtol(s, &e, 10);
        return e == s ? fb : static_cast<int>(n);
    };

    // ── General ──
    if (const char* v = query("dolphin_aspect_ratio"))  out.general.aspect_ratio = v;
    if (const char* v = query("dolphin_vsync"))          out.general.vsync = parse_bool(v);
    if (const char* v = query("dolphin_precision_frame_timing"))
        out.general.precision_frame_timing = parse_bool(v);
    if (const char* v = query("dolphin_shader_compilation")) out.general.shader_compilation = v;
    if (const char* v = query("dolphin_wait_for_shaders"))   out.general.wait_for_shaders = parse_bool(v);

    // ── Enhancements ──
    if (const char* v = query("dolphin_internal_resolution"))
        out.enhancements.internal_resolution = (std::strcmp(v, "Auto") == 0) ? 0 : parse_int(v, 1);
    if (const char* v = query("dolphin_antialiasing")) {
        struct AA { int msaa; bool ssaa; };
        const std::map<std::string, AA> t = {
            {"None",{1,false}}, {"2x MSAA",{2,false}}, {"4x MSAA",{4,false}}, {"8x MSAA",{8,false}},
            {"2x SSAA",{2,true}}, {"4x SSAA",{4,true}}, {"8x SSAA",{8,true}},
        };
        auto it = t.find(v); if (it == t.end()) it = t.find("None");
        out.enhancements.msaa = it->second.msaa;
        out.enhancements.ssaa = it->second.ssaa;
    }
    if (const char* v = query("dolphin_texture_filtering")) {
        struct TF { int aniso; int force; };
        const std::map<std::string, TF> t = {
            {"Default",{-1,0}},
            {"1x Anisotropic",{0,0}}, {"2x Anisotropic",{1,0}}, {"4x Anisotropic",{2,0}},
            {"8x Anisotropic",{3,0}}, {"16x Anisotropic",{4,0}},
            {"Force Nearest and 1x Anisotropic",{0,1}},
            {"Force Linear and 1x Anisotropic",{0,2}}, {"Force Linear and 2x Anisotropic",{1,2}},
            {"Force Linear and 4x Anisotropic",{2,2}}, {"Force Linear and 8x Anisotropic",{3,2}},
            {"Force Linear and 16x Anisotropic",{4,2}},
        };
        auto it = t.find(v); if (it == t.end()) it = t.find("Default");
        out.enhancements.aniso = it->second.aniso;
        out.enhancements.force_filter = it->second.force;
    }
    if (const char* v = query("dolphin_output_resampling")) {
        const std::map<std::string,int> t = {
            {"Default",0},{"Bilinear",1},{"Bicubic B-Spline",2},
            {"Bicubic Mitchell-Netravali",3},{"Bicubic Catmull-Rom",4},
            {"Sharp Bilinear",5},{"Area Sampling",6},
        };
        auto it = t.find(v); out.enhancements.output_resampling = (it == t.end()) ? 0 : it->second;
    }
    if (const char* v = query("dolphin_scaled_efb_copy"))   out.enhancements.scaled_efb_copy = parse_bool(v);
    if (const char* v = query("dolphin_per_pixel_lighting")) out.enhancements.per_pixel_lighting = parse_bool(v);
    if (const char* v = query("dolphin_widescreen_hack"))    out.enhancements.widescreen_hack = parse_bool(v);
    if (const char* v = query("dolphin_force_true_color"))   out.enhancements.force_true_color = parse_bool(v);
    if (const char* v = query("dolphin_disable_fog"))        out.enhancements.disable_fog = parse_bool(v);
    if (const char* v = query("dolphin_arbitrary_mipmap_detection")) out.enhancements.arbitrary_mipmap_detection = parse_bool(v);
    if (const char* v = query("dolphin_disable_copy_filter")) out.enhancements.disable_copy_filter = parse_bool(v);
    if (const char* v = query("dolphin_hdr_output"))         out.enhancements.hdr_output = parse_bool(v);
    if (const char* v = query("dolphin_stereo_mode")) {
        const std::map<std::string,int> t = {
            {"Off",0},{"Side-by-Side",1},{"Top-and-Bottom",2},{"Anaglyph",3},{"HDMI 3D",4},{"Passive",5},
        };
        auto it = t.find(v); out.enhancements.stereo_mode = (it == t.end()) ? 0 : it->second;
    }
    if (const char* v = query("dolphin_stereo_swap_eyes"))   out.enhancements.stereo_swap_eyes = parse_bool(v);
    if (const char* v = query("dolphin_stereo_per_eye_full")) out.enhancements.stereo_per_eye_full = parse_bool(v);
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

    // ── Enhancements ──
    Config::SetCurrent(Config::GFX_EFB_SCALE, v.enhancements.internal_resolution);
    Config::SetCurrent(Config::GFX_MSAA, static_cast<u32>(v.enhancements.msaa));
    Config::SetCurrent(Config::GFX_SSAA, v.enhancements.ssaa);
    Config::SetCurrent(Config::GFX_ENHANCE_MAX_ANISOTROPY,
                       static_cast<AnisotropicFilteringMode>(v.enhancements.aniso));
    Config::SetCurrent(Config::GFX_ENHANCE_FORCE_TEXTURE_FILTERING,
                       static_cast<TextureFilteringMode>(v.enhancements.force_filter));
    Config::SetCurrent(Config::GFX_ENHANCE_OUTPUT_RESAMPLING,
                       static_cast<OutputResamplingMode>(v.enhancements.output_resampling));
    Config::SetCurrent(Config::GFX_HACK_COPY_EFB_SCALED, v.enhancements.scaled_efb_copy);
    Config::SetCurrent(Config::GFX_ENABLE_PIXEL_LIGHTING, v.enhancements.per_pixel_lighting);
    Config::SetCurrent(Config::GFX_WIDESCREEN_HACK, v.enhancements.widescreen_hack);
    Config::SetCurrent(Config::GFX_ENHANCE_FORCE_TRUE_COLOR, v.enhancements.force_true_color);
    Config::SetCurrent(Config::GFX_DISABLE_FOG, v.enhancements.disable_fog);
    Config::SetCurrent(Config::GFX_ENHANCE_ARBITRARY_MIPMAP_DETECTION, v.enhancements.arbitrary_mipmap_detection);
    Config::SetCurrent(Config::GFX_ENHANCE_DISABLE_COPY_FILTER, v.enhancements.disable_copy_filter);
    Config::SetCurrent(Config::GFX_ENHANCE_HDR_OUTPUT, v.enhancements.hdr_output);
    Config::SetCurrent(Config::GFX_STEREO_MODE, static_cast<StereoMode>(v.enhancements.stereo_mode));
    Config::SetCurrent(Config::GFX_STEREO_SWAP_EYES, v.enhancements.stereo_swap_eyes);
    Config::SetCurrent(Config::GFX_STEREO_PER_EYE_RESOLUTION_FULL, v.enhancements.stereo_per_eye_full);
}
#endif

} // namespace DolphinLibretro::CoreOptions::Graphics
