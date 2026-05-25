// SPDX-FileCopyrightText: 2026 Mark Pearce (RetroNest)
// SPDX-License-Identifier: GPL-3.0+
//
// SP7: Core/system libretro core options for dolphin-libretro (the
// standalone's General / Advanced / GameCube / Wii panes). Same shape as
// CoreOptionsGraphics: primitive Values, Parse, guarded Apply. Nearly all
// keys are Config::MAIN_* (Dolphin.ini [Core]/[DSP]).

#pragma once

#include "libretro.h"
#include <string>
#include <vector>

namespace DolphinLibretro::CoreOptions::Core
{

struct Values
{
    struct General {
        bool        cpu_thread       = false;        // MAIN_CPU_THREAD (Dolphin's own default = false)
        bool        enable_cheats    = false;        // MAIN_ENABLE_CHEATS
        bool        load_into_memory = false;        // MAIN_LOAD_GAME_INTO_MEMORY
        bool        override_region  = false;        // MAIN_OVERRIDE_REGION_SETTINGS
        std::string emulation_speed  = "1.000000";   // MAIN_EMULATION_SPEED (float multiplier)
        int         fallback_region  = 1;            // MAIN_FALLBACK_REGION (DiscIO::Region; 1=NTSC-U).
                                                     // Intentional: Dolphin's own default is locale-dependent
                                                     // (GetDefaultRegion()); we pin NTSC-U for deterministic boots.
    } general;

    struct Advanced {
        std::string cpu_core                  = "JIT";  // MAIN_CPU_CORE ("Interpreter"/"Cached Interpreter"/"JIT")
        bool        mmu                       = false;  // MAIN_MMU
        bool        pause_on_panic            = false;  // MAIN_PAUSE_ON_PANIC
        bool        accurate_cpu_cache        = false;  // MAIN_ACCURATE_CPU_CACHE
        bool        correct_time_drift        = false;  // MAIN_CORRECT_TIME_DRIFT
        bool        rush_frame_presentation   = false;  // MAIN_RUSH_FRAME_PRESENTATION
        bool        smooth_early_presentation = false;  // MAIN_SMOOTH_EARLY_PRESENTATION
        bool        overclock_enable          = false;  // MAIN_OVERCLOCK_ENABLE
        int         overclock                 = 1;      // MAIN_OVERCLOCK (multiplier 1..4 -> float)
        bool        vi_overclock_enable       = false;  // MAIN_VI_OVERCLOCK_ENABLE
        int         vi_overclock              = 1;      // MAIN_VI_OVERCLOCK (multiplier 1..4 -> float)
    } advanced;

    struct GameCube {
        bool skip_ipl      = true;   // MAIN_SKIP_IPL
        int  language      = 0;      // MAIN_GC_LANGUAGE (0=English..5=Dutch)
        int  slot_a        = 8;      // MAIN_SLOT_A (EXIDeviceType int; 8=GCI Folder)
        int  slot_b        = 255;    // MAIN_SLOT_B (255=None)
        int  serial_port_1 = 255;    // MAIN_SERIAL_PORT_1 (255=None)
    } gamecube;

    struct Wii {
        bool               keyboard         = false;  // MAIN_WII_KEYBOARD
        bool               wiilink          = false;  // MAIN_WII_WIILINK_ENABLE
        bool               sd_card          = true;   // MAIN_WII_SD_CARD
        bool               sd_card_writes   = true;   // MAIN_ALLOW_SD_WRITES
        bool               sd_card_folder_sync = false; // MAIN_WII_SD_CARD_ENABLE_FOLDER_SYNC
        unsigned long long sd_card_size     = 0;      // MAIN_WII_SD_CARD_FILESIZE (u64 bytes; 0=Auto)
    } wii;
};

void AppendDefinitions(std::vector<retro_core_option_v2_definition>& out);
void Parse(retro_environment_t cb, Values& out);
#ifndef CORE_OPTIONS_TEST_ONLY
void Apply(const Values& v);
#endif

} // namespace DolphinLibretro::CoreOptions::Core
