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
        v.reserve(96);  // 53 Graphics + 9 Audio + 28 Core + terminator
        Graphics::AppendDefinitions(v);
        Audio::AppendDefinitions(v);
        Core::AppendDefinitions(v);
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
    Audio::Parse(cb, r.audio);
    Core::Parse(cb, r.core);

    // Diagnostic: log the resolved Graphics values applied this boot, so a
    // user can confirm their settings actually reached the core. Routed via
    // the frontend log callback (visible with RETRONEST_DOLPHIN_LOG=1).
    const auto& g = r.graphics;
    CORE_OPTIONS_LOG(RETRO_LOG_INFO,
        "[CoreOptions] resolved graphics: aspect=%s internal_res=%d msaa=%d ssaa=%d "
        "texfilter(aniso=%d,force=%d) output_resampling=%d vsync=%d shader=%s | "
        "hacks: skip_efb_access=%d store_efb_tex=%d store_xfb_tex=%d accuracy=%d "
        "immediate_xfb=%d disable_bbox=%d | adv: backend_mt=%d hires_tex=%d "
        "manual_tex_sampling=%d | osd: messages=%d fps=%d speed=%d",
        g.general.aspect_ratio.c_str(), g.enhancements.internal_resolution,
        g.enhancements.msaa, g.enhancements.ssaa ? 1 : 0,
        g.enhancements.aniso, g.enhancements.force_filter, g.enhancements.output_resampling,
        g.general.vsync ? 1 : 0, g.general.shader_compilation.c_str(),
        g.hacks.skip_efb_access ? 1 : 0, g.hacks.store_efb_to_texture ? 1 : 0,
        g.hacks.store_xfb_to_texture ? 1 : 0, g.hacks.texcache_accuracy,
        g.hacks.immediate_xfb ? 1 : 0, g.hacks.disable_bounding_box ? 1 : 0,
        g.advanced.backend_multithreading ? 1 : 0, g.advanced.load_custom_textures ? 1 : 0,
        g.advanced.manual_texture_sampling ? 1 : 0,
        g.osd.show_messages ? 1 : 0, g.osd.show_fps ? 1 : 0, g.osd.show_speed ? 1 : 0);

    const auto& a = r.audio;
    CORE_OPTIONS_LOG(RETRO_LOG_INFO,
        "[CoreOptions] resolved audio: dsp(hle=%d,jit=%d) latency=%d dpl2(dec=%d,q=%d) "
        "buffer=%d fill_gaps=%d preserve_pitch=%d mute_unthrottle=%d volume=%d",
        a.dsp_hle ? 1 : 0, a.dsp_jit ? 1 : 0, a.latency, a.dpl2_decoder ? 1 : 0,
        a.dpl2_quality, a.buffer_size, a.fill_gaps ? 1 : 0, a.preserve_pitch ? 1 : 0,
        a.mute_on_unthrottle ? 1 : 0, a.volume);

    const auto& c = r.core;
    CORE_OPTIONS_LOG(RETRO_LOG_INFO,
        "[CoreOptions] resolved core: dual_core=%d cpu_core=%s mmu=%d region=%d speed=%s | "
        "overclock(en=%d,%dx) vi_oc(en=%d,%dx) | gc: skip_ipl=%d lang=%d slotA=%d slotB=%d sp1=%d | "
        "wii: kbd=%d wiilink=%d sd(on=%d,wr=%d,sync=%d,size=%llu)",
        c.general.cpu_thread ? 1 : 0, c.advanced.cpu_core.c_str(), c.advanced.mmu ? 1 : 0,
        c.general.fallback_region, c.general.emulation_speed.c_str(),
        c.advanced.overclock_enable ? 1 : 0, c.advanced.overclock,
        c.advanced.vi_overclock_enable ? 1 : 0, c.advanced.vi_overclock,
        c.gamecube.skip_ipl ? 1 : 0, c.gamecube.language, c.gamecube.slot_a,
        c.gamecube.slot_b, c.gamecube.serial_port_1,
        c.wii.keyboard ? 1 : 0, c.wii.wiilink ? 1 : 0, c.wii.sd_card ? 1 : 0,
        c.wii.sd_card_writes ? 1 : 0, c.wii.sd_card_folder_sync ? 1 : 0,
        static_cast<unsigned long long>(c.wii.sd_card_size));

    return r;
}

} // namespace DolphinLibretro::CoreOptions
