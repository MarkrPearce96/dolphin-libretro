// SPDX-FileCopyrightText: 2026 Mark Pearce (RetroNest)
// SPDX-License-Identifier: GPL-3.0+
//
// SP6: Graphics-category libretro core options for dolphin-libretro.
// Values are stored as PRIMITIVES (int/bool/std::string) so this header
// and Parse() carry no Dolphin engine dependency — the standalone unit
// test compiles CoreOptionsGraphics.cpp with -DCORE_OPTIONS_TEST_ONLY and
// links nothing else. Apply() (the only engine-touching function) is
// guarded out of that build.

#pragma once

#include "libretro.h"
#include <string>
#include <vector>

namespace DolphinLibretro::CoreOptions::Graphics
{

// Resolved Graphics values. Member-initializers are the defaults applied
// when an option is unset / the host returns NULL. They mirror the
// deleted standalone adapter's presented defaults.
struct Values
{
    struct General {
        std::string aspect_ratio        = "Auto";        // AspectMode
        bool        vsync                = true;
        bool        precision_frame_timing = true;       // MAIN_PRECISION_FRAME_TIMING
        std::string shader_compilation   = "Specialized"; // ShaderCompilationMode
        bool        wait_for_shaders     = false;
    } general;

    struct Enhancements {
        int  internal_resolution = 1;     // GFX_EFB_SCALE (0=Auto,1=Native,N=Nx)
        int  msaa                = 1;     // GFX_MSAA raw sample count (1=off)  AA fan-out
        bool ssaa                = false; // GFX_SSAA                            AA fan-out
        int  aniso               = -1;    // AnisotropicFilteringMode int        tex-filter fan-out
        int  force_filter        = 0;     // TextureFilteringMode int            tex-filter fan-out
        int  output_resampling   = 0;     // OutputResamplingMode int
        bool scaled_efb_copy     = true;
        bool per_pixel_lighting  = false;
        bool widescreen_hack     = false;
        bool force_true_color    = true;
        bool disable_fog         = false;
        bool arbitrary_mipmap_detection = true;  // NB: Dolphin's own default is false; standalone presented true
        bool disable_copy_filter = false; // NB: Dolphin's own default is true; standalone presented false
        bool hdr_output          = false;
        int  stereo_mode         = 0;     // StereoMode int
        bool stereo_swap_eyes    = false;
        bool stereo_per_eye_full = false;
    } enhancements;

    struct Hacks {
        bool skip_efb_access      = true;  // -> GFX_HACK_EFB_ACCESS_ENABLE = !v (inverted)
        bool ignore_format_changes = true; // -> GFX_HACK_EFB_EMULATE_FORMAT_CHANGES = !v (inverted)
        bool store_efb_to_texture = true;  // -> GFX_HACK_SKIP_EFB_COPY_TO_RAM
        bool defer_efb_copies     = true;  // -> GFX_HACK_DEFER_EFB_COPIES
        int  texcache_accuracy    = 128;   // -> GFX_SAFE_TEXTURE_CACHE_COLOR_SAMPLES (0/128/512)
        bool gpu_texture_decoding = false; // -> GFX_ENABLE_GPU_TEXTURE_DECODING
        bool store_xfb_to_texture = true;  // -> GFX_HACK_SKIP_XFB_COPY_TO_RAM
        bool immediate_xfb        = false; // -> GFX_HACK_IMMEDIATE_XFB
        bool skip_duplicate_xfbs  = true;  // -> GFX_HACK_SKIP_DUPLICATE_XFBS
        bool fast_depth_calc      = true;  // -> GFX_FAST_DEPTH_CALC
        bool disable_bounding_box = true;  // -> GFX_HACK_BBOX_ENABLE = !v (inverted)
        bool vertex_rounding      = false; // -> GFX_HACK_VERTEX_ROUNDING
        bool save_texcache_to_state = true; // -> GFX_SAVE_TEXTURE_CACHE_TO_STATE
        bool vbi_skip             = false; // -> GFX_HACK_VI_SKIP
    } hacks;

    struct Advanced {
        bool load_custom_textures   = false; // -> GFX_HIRES_TEXTURES
        bool prefetch_custom_textures = false; // -> GFX_CACHE_HIRES_TEXTURES
        bool enable_graphics_mods   = false; // -> GFX_MODS_ENABLE
        bool crop                   = false; // -> GFX_CROP
        bool backend_multithreading = true;  // -> GFX_BACKEND_MULTITHREADING
        bool prefer_vs_expansion    = false; // -> GFX_PREFER_VS_FOR_LINE_POINT_EXPANSION
        bool cpu_cull               = false; // -> GFX_CPU_CULL
        bool defer_efb_invalidation = false; // -> GFX_HACK_EFB_DEFER_INVALIDATION
        bool manual_texture_sampling = false; // -> GFX_HACK_FAST_TEXTURE_SAMPLING = !v (inverted)
    } advanced;

    struct Osd {
        bool show_messages   = true;  // -> MAIN_OSD_MESSAGES
        int  font_size       = 13;    // -> MAIN_OSD_FONT_SIZE (13/18/24/36)
        bool show_fps        = false; // -> GFX_SHOW_FPS
        bool show_ftimes     = false; // -> GFX_SHOW_FTIMES
        bool show_vps        = false; // -> GFX_SHOW_VPS
        bool show_vtimes     = false; // -> GFX_SHOW_VTIMES
        bool show_speed      = false; // -> GFX_SHOW_SPEED
        bool show_graphs     = false; // -> GFX_SHOW_GRAPHS
        bool show_speed_colors = true; // -> GFX_SHOW_SPEED_COLORS
        int  perf_samp_window = 1000;  // -> GFX_PERF_SAMP_WINDOW (250/500/1000/2000/5000)
    } osd;
};

void AppendDefinitions(std::vector<retro_core_option_v2_definition>& out);
void Parse(retro_environment_t cb, Values& out);
// Apply writes into Dolphin's Config:: layer, so it does not exist in the
// standalone unit-test build (which links no engine). The decl is guarded
// to match the definition in the .cpp.
#ifndef CORE_OPTIONS_TEST_ONLY
void Apply(const Values& v);
#endif

} // namespace DolphinLibretro::CoreOptions::Graphics
