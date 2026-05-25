// SPDX-FileCopyrightText: 2026 Mark Pearce (RetroNest)
// SPDX-License-Identifier: GPL-3.0+
//
// SP7: Audio-category libretro core options for dolphin-libretro. Same
// shape as CoreOptionsGraphics: primitive Values (no engine dep), Parse
// (string -> Values, incl. the DSP-engine fan-out), and Apply (guarded
// out of the CORE_OPTIONS_TEST_ONLY build).

#pragma once

#include "libretro.h"
#include <string>
#include <vector>

namespace DolphinLibretro::CoreOptions::Audio
{

// Resolved Audio values. Member-initializers are the defaults applied when
// an option is unset / the host returns NULL — they mirror Dolphin's own
// Config defaults.
struct Values
{
    // dolphin_dsp_engine fan-out -> MAIN_DSP_HLE + MAIN_DSP_JIT.
    bool dsp_hle            = true;   // HLE default
    bool dsp_jit            = true;   // LLE-Recompiler vs Interpreter; ignored when HLE
    int  latency            = 20;     // MAIN_AUDIO_LATENCY (ms)
    bool dpl2_decoder       = false;  // MAIN_DPL2_DECODER
    int  dpl2_quality       = 2;      // MAIN_DPL2_QUALITY (AudioCommon::DPL2Quality, 0-3; High=2)
    int  buffer_size        = 80;     // MAIN_AUDIO_BUFFER_SIZE (ms)
    bool fill_gaps          = true;   // MAIN_AUDIO_FILL_GAPS
    bool preserve_pitch     = false;  // MAIN_AUDIO_PRESERVE_PITCH
    bool mute_on_unthrottle = false;  // MAIN_AUDIO_MUTE_ON_DISABLED_SPEED_LIMIT
    int  volume             = 100;    // MAIN_AUDIO_VOLUME (0-100)
};

void AppendDefinitions(std::vector<retro_core_option_v2_definition>& out);
void Parse(retro_environment_t cb, Values& out);
#ifndef CORE_OPTIONS_TEST_ONLY
void Apply(const Values& v);
#endif

} // namespace DolphinLibretro::CoreOptions::Audio
