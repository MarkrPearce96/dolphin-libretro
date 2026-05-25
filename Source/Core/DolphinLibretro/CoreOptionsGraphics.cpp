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
    // ── Hacks sub-tab ──
    out.push_back({
        "dolphin_texcache_accuracy", "Texture Cache Accuracy", nullptr,
        "How aggressively cached textures are validated. Safe = fewest "
        "misses (most accurate), Fast = highest performance.",
        nullptr, nullptr,
        {
            { "Safe", "Safe" },
            { "Default", "Default" },
            { "Fast", "Fast" },
            { nullptr, nullptr },
        },
        "Default",
    });
    out.push_back({
        "dolphin_skip_efb_access", "Skip EFB Access from CPU", nullptr,
        "Ignore CPU reads/writes of the EFB. Speed boost; disables some EFB-based effects.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "enabled",
    });
    out.push_back({
        "dolphin_ignore_format_changes", "Ignore Format Changes", nullptr,
        "Ignore EFB format changes. Speed win for many games; minor defects in a few.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "enabled",
    });
    out.push_back({
        "dolphin_store_efb_to_texture", "Store EFB Copies to Texture Only", nullptr,
        "Keep EFB copies on the GPU, bypassing RAM. Big speed boost; rare defects.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "enabled",
    });
    out.push_back({
        "dolphin_defer_efb_copies", "Defer EFB Copies to RAM", nullptr,
        "Wait for GPU sync before writing EFB copies to RAM. Speed boost.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "enabled",
    });
    out.push_back({
        "dolphin_gpu_texture_decoding", "GPU Texture Decoding", nullptr,
        "Decode textures on the GPU instead of the CPU.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_store_xfb_to_texture", "Store XFB Copies to Texture Only", nullptr,
        "Keep XFB copies on the GPU. Big speed boost; rare defects.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "enabled",
    });
    out.push_back({
        "dolphin_immediate_xfb", "Immediately Present XFB", nullptr,
        "Display the XFB as soon as it's drawn. Lower latency, slight tearing risk.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_skip_duplicate_xfbs", "Skip Presenting Duplicate Frames", nullptr,
        "Detect and skip identical consecutive frames to save GPU work.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "enabled",
    });
    out.push_back({
        "dolphin_fast_depth_calc", "Fast Depth Calculation", nullptr,
        "Use a faster GPU-friendly depth calculation path.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "enabled",
    });
    out.push_back({
        "dolphin_disable_bounding_box", "Disable Bounding Box", nullptr,
        "Disable bounding-box emulation. Big GPU speed-up; a few games need it (e.g. Paper Mario).",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "enabled",
    });
    out.push_back({
        "dolphin_vertex_rounding", "Vertex Rounding", nullptr,
        "Round vertex coordinates to integers. Fixes seams in some games at high resolutions.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_save_texcache_to_state", "Save Texture Cache to State", nullptr,
        "Save the texture cache in save states. Larger states, smoother resume.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "enabled",
    });
    out.push_back({
        "dolphin_vbi_skip", "VBI Skip", nullptr,
        "Skip Vertical Blank Interrupts when lag is detected. Smoother audio off-100%; can freeze.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    // ── Advanced sub-tab ──
    out.push_back({
        "dolphin_load_custom_textures", "Load Custom Textures", nullptr,
        "Load high-resolution texture replacements from the user's Load/Textures folder.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_prefetch_custom_textures", "Prefetch Custom Textures", nullptr,
        "Pre-load all custom textures into VRAM at boot. Eliminates load stutter; uses more memory.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_enable_graphics_mods", "Enable Graphics Mods", nullptr,
        "Load graphics mods from the user's Load/GraphicMods folder.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_crop", "Crop", nullptr,
        "Crop overscan/black borders from the rendered image.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_backend_multithreading", "Backend Multithreading", nullptr,
        "Distribute video-backend work across multiple threads. Recommended on.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "enabled",
    });
    out.push_back({
        "dolphin_prefer_vs_expansion", "Prefer VS for Point/Line Expansion", nullptr,
        "Expand line/point primitives in the vertex shader instead of the geometry shader. Driver workaround.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_cpu_cull", "Cull Vertices on the CPU", nullptr,
        "Cull invisible geometry on the CPU before sending to the GPU. Speeds up some games.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_defer_efb_invalidation", "Defer EFB Cache Invalidation", nullptr,
        "Reduce overhead by deferring EFB-cache invalidations. Speed win; rare glitches.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_manual_texture_sampling", "Manual Texture Sampling", nullptr,
        "Trade some speed for accuracy in the texture sampler. (Checked = manual; disables fast sampling.)",
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

    // ── Hacks ──
    if (const char* v = query("dolphin_skip_efb_access"))       out.hacks.skip_efb_access = parse_bool(v);
    if (const char* v = query("dolphin_ignore_format_changes")) out.hacks.ignore_format_changes = parse_bool(v);
    if (const char* v = query("dolphin_store_efb_to_texture"))  out.hacks.store_efb_to_texture = parse_bool(v);
    if (const char* v = query("dolphin_defer_efb_copies"))      out.hacks.defer_efb_copies = parse_bool(v);
    if (const char* v = query("dolphin_texcache_accuracy")) {
        if      (std::strcmp(v, "Safe") == 0) out.hacks.texcache_accuracy = 0;
        else if (std::strcmp(v, "Fast") == 0) out.hacks.texcache_accuracy = 512;
        else                                  out.hacks.texcache_accuracy = 128;
    }
    if (const char* v = query("dolphin_gpu_texture_decoding"))  out.hacks.gpu_texture_decoding = parse_bool(v);
    if (const char* v = query("dolphin_store_xfb_to_texture"))  out.hacks.store_xfb_to_texture = parse_bool(v);
    if (const char* v = query("dolphin_immediate_xfb"))         out.hacks.immediate_xfb = parse_bool(v);
    if (const char* v = query("dolphin_skip_duplicate_xfbs"))   out.hacks.skip_duplicate_xfbs = parse_bool(v);
    if (const char* v = query("dolphin_fast_depth_calc"))       out.hacks.fast_depth_calc = parse_bool(v);
    if (const char* v = query("dolphin_disable_bounding_box"))  out.hacks.disable_bounding_box = parse_bool(v);
    if (const char* v = query("dolphin_vertex_rounding"))       out.hacks.vertex_rounding = parse_bool(v);
    if (const char* v = query("dolphin_save_texcache_to_state")) out.hacks.save_texcache_to_state = parse_bool(v);
    if (const char* v = query("dolphin_vbi_skip"))              out.hacks.vbi_skip = parse_bool(v);

    // ── Advanced ──
    if (const char* v = query("dolphin_load_custom_textures"))    out.advanced.load_custom_textures = parse_bool(v);
    if (const char* v = query("dolphin_prefetch_custom_textures")) out.advanced.prefetch_custom_textures = parse_bool(v);
    if (const char* v = query("dolphin_enable_graphics_mods"))    out.advanced.enable_graphics_mods = parse_bool(v);
    if (const char* v = query("dolphin_crop"))                    out.advanced.crop = parse_bool(v);
    if (const char* v = query("dolphin_backend_multithreading"))  out.advanced.backend_multithreading = parse_bool(v);
    if (const char* v = query("dolphin_prefer_vs_expansion"))     out.advanced.prefer_vs_expansion = parse_bool(v);
    if (const char* v = query("dolphin_cpu_cull"))                out.advanced.cpu_cull = parse_bool(v);
    if (const char* v = query("dolphin_defer_efb_invalidation"))  out.advanced.defer_efb_invalidation = parse_bool(v);
    if (const char* v = query("dolphin_manual_texture_sampling")) out.advanced.manual_texture_sampling = parse_bool(v);
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

    // ── Hacks ── (the option is the user-facing "skip/ignore/disable"; invert onto the engine bool)
    Config::SetCurrent(Config::GFX_HACK_EFB_ACCESS_ENABLE, !v.hacks.skip_efb_access);
    Config::SetCurrent(Config::GFX_HACK_EFB_EMULATE_FORMAT_CHANGES, !v.hacks.ignore_format_changes);
    Config::SetCurrent(Config::GFX_HACK_SKIP_EFB_COPY_TO_RAM, v.hacks.store_efb_to_texture);
    Config::SetCurrent(Config::GFX_HACK_DEFER_EFB_COPIES, v.hacks.defer_efb_copies);
    Config::SetCurrent(Config::GFX_SAFE_TEXTURE_CACHE_COLOR_SAMPLES, v.hacks.texcache_accuracy);
    Config::SetCurrent(Config::GFX_ENABLE_GPU_TEXTURE_DECODING, v.hacks.gpu_texture_decoding);
    Config::SetCurrent(Config::GFX_HACK_SKIP_XFB_COPY_TO_RAM, v.hacks.store_xfb_to_texture);
    Config::SetCurrent(Config::GFX_HACK_IMMEDIATE_XFB, v.hacks.immediate_xfb);
    Config::SetCurrent(Config::GFX_HACK_SKIP_DUPLICATE_XFBS, v.hacks.skip_duplicate_xfbs);
    Config::SetCurrent(Config::GFX_FAST_DEPTH_CALC, v.hacks.fast_depth_calc);
    Config::SetCurrent(Config::GFX_HACK_BBOX_ENABLE, !v.hacks.disable_bounding_box);
    Config::SetCurrent(Config::GFX_HACK_VERTEX_ROUNDING, v.hacks.vertex_rounding);
    Config::SetCurrent(Config::GFX_SAVE_TEXTURE_CACHE_TO_STATE, v.hacks.save_texcache_to_state);
    Config::SetCurrent(Config::GFX_HACK_VI_SKIP, v.hacks.vbi_skip);

    // ── Advanced ──
    Config::SetCurrent(Config::GFX_HIRES_TEXTURES, v.advanced.load_custom_textures);
    Config::SetCurrent(Config::GFX_CACHE_HIRES_TEXTURES, v.advanced.prefetch_custom_textures);
    Config::SetCurrent(Config::GFX_MODS_ENABLE, v.advanced.enable_graphics_mods);
    Config::SetCurrent(Config::GFX_CROP, v.advanced.crop);
    Config::SetCurrent(Config::GFX_BACKEND_MULTITHREADING, v.advanced.backend_multithreading);
    Config::SetCurrent(Config::GFX_PREFER_VS_FOR_LINE_POINT_EXPANSION, v.advanced.prefer_vs_expansion);
    Config::SetCurrent(Config::GFX_CPU_CULL, v.advanced.cpu_cull);
    Config::SetCurrent(Config::GFX_HACK_EFB_DEFER_INVALIDATION, v.advanced.defer_efb_invalidation);
    // "Manual Texture Sampling" checked = FastTextureSampling OFF.
    Config::SetCurrent(Config::GFX_HACK_FAST_TEXTURE_SAMPLING, !v.advanced.manual_texture_sampling);
}
#endif

} // namespace DolphinLibretro::CoreOptions::Graphics
