// SPDX-FileCopyrightText: 2026 Mark Pearce (RetroNest)
// SPDX-License-Identifier: GPL-3.0+

#include "CoreOptionsCore.h"

#include <cstdlib>
#include <cstring>

#ifndef CORE_OPTIONS_TEST_ONLY
#include "Common/Config/Config.h"
#include "Core/Config/MainSettings.h"
#include "Core/HW/EXI/EXI_Device.h"     // ExpansionInterface::EXIDeviceType
#include "Core/PowerPC/PowerPC.h"       // PowerPC::CPUCore, DefaultCPUCore
#include "DiscIO/Enums.h"               // DiscIO::Region
#endif

namespace DolphinLibretro::CoreOptions::Core
{

void AppendDefinitions(std::vector<retro_core_option_v2_definition>& out)
{
    // ── General ──
    out.push_back({
        "dolphin_cpu_thread", "Dual Core (Speed Hack)", nullptr,
        "Run CPU and GPU emulation on separate threads. Big speed gain; a few "
        "timing-sensitive games may glitch with it on.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_enable_cheats", "Enable Cheats", nullptr,
        "Process AR/Gecko cheat codes. Off by default for safety.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_load_game_into_memory", "Load Whole Game Into Memory", nullptr,
        "Pre-load the entire disc image into RAM at boot. Eliminates disc I/O "
        "stutter; uses more host memory.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_override_region_settings", "Allow Mismatched Region Settings", nullptr,
        "Force a region's settings (language, video mode) regardless of disc region.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_emulation_speed", "Speed Limit", nullptr,
        "Cap on emulated speed relative to native. Unlimited removes the throttle.",
        nullptr, nullptr,
        {
            { "0.000000", "Unlimited" },
            { "0.100000", "10%" }, { "0.200000", "20%" }, { "0.300000", "30%" },
            { "0.400000", "40%" }, { "0.500000", "50%" }, { "0.600000", "60%" },
            { "0.700000", "70%" }, { "0.800000", "80%" }, { "0.900000", "90%" },
            { "1.000000", "100% (Normal Speed)" },
            { "1.100000", "110%" }, { "1.200000", "120%" }, { "1.300000", "130%" },
            { "1.400000", "140%" }, { "1.500000", "150%" }, { "1.600000", "160%" },
            { "1.700000", "170%" }, { "1.800000", "180%" }, { "1.900000", "190%" },
            { "2.000000", "200%" },
            { nullptr, nullptr },
        },
        "1.000000",
    });
    out.push_back({
        "dolphin_fallback_region", "Fallback Region", nullptr,
        "Region used for games whose region can't be auto-detected. Affects "
        "boot timing and the system-menu locale.",
        nullptr, nullptr,
        {
            { "0", "NTSC-J (Japan)" }, { "1", "NTSC-U (Americas)" },
            { "2", "PAL (Europe)" }, { "3", "Region-Free / Unknown" },
            { "4", "NTSC-K (Korea)" },
            { nullptr, nullptr },
        },
        "1",
    });

    // ── Advanced ──
    out.push_back({
        "dolphin_cpu_core", "CPU Emulation Engine", nullptr,
        "The CPU backend. JIT is required for full-speed gameplay; the "
        "interpreters are debug/accuracy fallbacks.",
        nullptr, nullptr,
        {
            { "Interpreter",        "Interpreter (Slowest)" },
            { "Cached Interpreter", "Cached Interpreter (Slow)" },
            { "JIT",                "JIT Recompiler (Recommended)" },
            { nullptr, nullptr },
        },
        "JIT",
    });
    out.push_back({
        "dolphin_mmu", "Enable MMU", nullptr,
        "Emulate the memory management unit. Slower but required by a small "
        "set of games (typically Virtual Console / homebrew).",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_pause_on_panic", "Pause on Panic", nullptr,
        "Pause emulation when Dolphin reports a non-fatal error.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_accurate_cpu_cache", "Enable Write-Back Cache (Slow)", nullptr,
        "Emulate the CPU's L1 cache. Slower but more accurate; needed for a "
        "handful of self-modifying-code games.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_correct_time_drift", "Correct Time Drift", nullptr,
        "Compensate for accumulated frame-pacing drift over long sessions.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_rush_frame_presentation", "Rush Frame Presentation", nullptr,
        "Aggressively present frames as soon as they're ready. Lower latency, "
        "more tearing without V-Sync.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_smooth_early_presentation", "Smooth Early Presentation", nullptr,
        "Smooth pacing for frames that finish ahead of schedule.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_overclock_enable", "Enable CPU Clock Override", nullptr,
        "Allow the multiplier below to scale the emulated CPU clock. Some "
        "games run smoother overclocked; others crash.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_overclock", "CPU Overclock Multiplier", nullptr,
        "Multiplier on the emulated CPU clock when overclocking is enabled. "
        "1x = native.",
        nullptr, nullptr,
        {
            { "1", "1x (Native)" }, { "2", "2x (+100%)" },
            { "3", "3x (+200%)" }, { "4", "4x (+300%)" },
            { nullptr, nullptr },
        },
        "1",
    });
    out.push_back({
        "dolphin_vi_overclock_enable", "Enable VBI Frequency Override", nullptr,
        "Scale the video-interface clock independently of the CPU. Affects "
        "refresh-rate timing for some games.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_vi_overclock", "VI Overclock Multiplier", nullptr,
        "Multiplier on the VI clock when VI overclocking is enabled.",
        nullptr, nullptr,
        {
            { "1", "1x (Native)" }, { "2", "2x" }, { "3", "3x" }, { "4", "4x" },
            { nullptr, nullptr },
        },
        "1",
    });

    // ── GameCube ──
    out.push_back({
        "dolphin_skip_ipl", "Skip Main Menu (IPL)", nullptr,
        "Skip the GameCube boot animation and start the game directly. When "
        "off, requires IPL.bin in the BIOS folder.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "enabled",
    });
    out.push_back({
        "dolphin_gc_language", "System Language", nullptr,
        "System language used by GameCube games that respect it.",
        nullptr, nullptr,
        {
            { "0", "English" }, { "1", "German" }, { "2", "French" },
            { "3", "Spanish" }, { "4", "Italian" }, { "5", "Dutch" },
            { nullptr, nullptr },
        },
        "0",
    });
    out.push_back({
        "dolphin_slot_a", "Slot A", nullptr,
        "Device in the GameCube's left memory-card / EXI slot.",
        nullptr, nullptr,
        {
            { "255", "Nothing" }, { "0", "Dummy" }, { "1", "Memory Card" },
            { "8", "GCI Folder" }, { "7", "USB Gecko" },
            { "9", "Advance Game Port" }, { "4", "Microphone" },
            { nullptr, nullptr },
        },
        "8",
    });
    out.push_back({
        "dolphin_slot_b", "Slot B", nullptr,
        "Device in the GameCube's right memory-card / EXI slot.",
        nullptr, nullptr,
        {
            { "255", "Nothing" }, { "0", "Dummy" }, { "1", "Memory Card" },
            { "8", "GCI Folder" }, { "7", "USB Gecko" },
            { "9", "Advance Game Port" }, { "4", "Microphone" },
            { nullptr, nullptr },
        },
        "255",
    });
    out.push_back({
        "dolphin_serial_port_1", "Serial Port 1 (SP1)", nullptr,
        "Device on the GameCube's serial port — network adapters in "
        "compatible games.",
        nullptr, nullptr,
        {
            { "255", "Nothing" }, { "0", "Dummy" },
            { "5", "Broadband Adapter (TAP)" },
            { "10", "Broadband Adapter (XLink Kai)" },
            { "11", "Broadband Adapter (tapserver)" },
            { "12", "Broadband Adapter (HLE)" },
            { "13", "Modem Adapter (tapserver)" },
            { "6", "Triforce AM-Baseboard" },
            { nullptr, nullptr },
        },
        "255",
    });

    // ── Wii ──
    out.push_back({
        "dolphin_wii_keyboard", "Connect USB Keyboard", nullptr,
        "Make a USB keyboard visible to Wii software.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_enable_wiilink", "Enable WiiConnect24 (WiiLink)", nullptr,
        "Patch the Wii Shop / Channels to use community WiiLink servers. Off "
        "by default to avoid third-party network calls.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_wii_sd_card", "Insert SD Card", nullptr,
        "Make a virtual SD card visible to Wii software. Required for save "
        "imports, channel installs, and SD-using homebrew.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "enabled",
    });
    out.push_back({
        "dolphin_wii_sd_card_writes", "Allow Writes to SD Card", nullptr,
        "When off, the SD card is read-only — protects a shared image from "
        "accidental modification.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "enabled",
    });
    out.push_back({
        "dolphin_wii_sd_card_folder_sync", "Auto-Sync SD with Folder", nullptr,
        "Mirror the SD card image from a host folder.",
        nullptr, nullptr,
        { { "enabled", "Enabled" }, { "disabled", "Disabled" }, { nullptr, nullptr } },
        "disabled",
    });
    out.push_back({
        "dolphin_wii_sd_card_size", "SD Card Size", nullptr,
        "Capacity of the virtual SD card. Auto uses the image file as-is.",
        nullptr, nullptr,
        {
            { "0", "Auto" },
            { "67108864", "64 MiB" }, { "134217728", "128 MiB" },
            { "268435456", "256 MiB" }, { "536870912", "512 MiB" },
            { "1073741824", "1 GiB" }, { "2147483648", "2 GiB" },
            { "4294967296", "4 GiB (SDHC)" }, { "8589934592", "8 GiB (SDHC)" },
            { "17179869184", "16 GiB (SDHC)" }, { "34359738368", "32 GiB (SDHC)" },
            { nullptr, nullptr },
        },
        "0",
    });
}

} // namespace DolphinLibretro::CoreOptions::Core
