// SPDX-FileCopyrightText: 2026 Mark Pearce (RetroNest)
// SPDX-License-Identifier: GPL-3.0+

#include "CoreOptionsAudio.h"

#include <cstdlib>
#include <cstring>

#ifndef CORE_OPTIONS_TEST_ONLY
#include "AudioCommon/Enums.h"          // AudioCommon::DPL2Quality
#include "Common/Config/Config.h"
#include "Core/Config/MainSettings.h"
#endif

namespace DolphinLibretro::CoreOptions::Audio
{

void AppendDefinitions(std::vector<retro_core_option_v2_definition>& out)
{
    out.push_back({
        "dolphin_dsp_engine", "DSP Emulation Engine", nullptr,
        "How the audio DSP is emulated. HLE is fast and compatible; LLE is "
        "slower but accurate and required by a few games.",
        nullptr, nullptr,
        {
            { "HLE",             "HLE (Recommended)" },
            { "LLE Recompiler",  "LLE Recompiler (Slow)" },
            { "LLE Interpreter", "LLE Interpreter (Very Slow)" },
            { nullptr, nullptr },
        },
        "HLE",
    });
    out.push_back({
        "dolphin_audio_latency", "Audio Latency", nullptr,
        "Output latency in milliseconds. Lower is tighter but risks dropouts. "
        "Only active with backends that support latency control (OpenAL).",
        nullptr, nullptr,
        {
            { "0", "0 ms" }, { "10", "10 ms" }, { "20", "20 ms" }, { "40", "40 ms" },
            { "60", "60 ms" }, { "80", "80 ms" }, { "100", "100 ms" },
            { "150", "150 ms" }, { "200", "200 ms" },
            { nullptr, nullptr },
        },
        "20",
    });
    out.push_back({
        "dolphin_dpl2_decoder", "Dolby Pro Logic II Decoder", nullptr,
        "Decode the stereo mix into 5.1 surround. Requires a backend that "
        "supports DPL2 (OpenAL) and DSP in LLE mode; otherwise inert.",
        nullptr, nullptr,
        {
            { "enabled", "Enabled" }, { "disabled", "Disabled" },
            { nullptr, nullptr },
        },
        "disabled",
    });
    out.push_back({
        "dolphin_dpl2_quality", "DPL2 Decoding Quality", nullptr,
        "Trade-off between CPU cost and surround-decode accuracy.",
        nullptr, nullptr,
        {
            { "0", "Lowest (Latency ~10 ms)" }, { "1", "Low (Latency ~20 ms)" },
            { "2", "High (Latency ~40 ms)" },   { "3", "Highest (Latency ~80 ms)" },
            { nullptr, nullptr },
        },
        "2",
    });
    out.push_back({
        "dolphin_audio_buffer_size", "Audio Buffer Size", nullptr,
        "Internal mixer buffer in milliseconds. Higher is smoother but adds "
        "delay between picture and sound.",
        nullptr, nullptr,
        {
            { "32", "32 ms" }, { "48", "48 ms" }, { "64", "64 ms" }, { "80", "80 ms" },
            { "96", "96 ms" }, { "128", "128 ms" }, { "160", "160 ms" },
            { "256", "256 ms" }, { "512", "512 ms" },
            { nullptr, nullptr },
        },
        "80",
    });
    out.push_back({
        "dolphin_audio_fill_gaps", "Fill Audio Gaps", nullptr,
        "Synthesize silence when emulation can't keep up. Disable for more "
        "accurate native behaviour; enable for smoothness.",
        nullptr, nullptr,
        {
            { "enabled", "Enabled" }, { "disabled", "Disabled" },
            { nullptr, nullptr },
        },
        "enabled",
    });
    out.push_back({
        "dolphin_audio_preserve_pitch", "Preserve Audio Pitch", nullptr,
        "Time-stretch audio to keep pitch constant when emulation runs off "
        "100%. Useful with fast-forward.",
        nullptr, nullptr,
        {
            { "enabled", "Enabled" }, { "disabled", "Disabled" },
            { nullptr, nullptr },
        },
        "disabled",
    });
    out.push_back({
        "dolphin_audio_mute_on_unthrottle", "Mute When Unthrottled", nullptr,
        "Silence audio while running unthrottled (fast-forward). Avoids "
        "pitch/playback artifacts.",
        nullptr, nullptr,
        {
            { "enabled", "Enabled" }, { "disabled", "Disabled" },
            { nullptr, nullptr },
        },
        "disabled",
    });
    out.push_back({
        "dolphin_volume", "Volume", nullptr,
        "Master output volume.",
        nullptr, nullptr,
        {
            { "0", "0%" }, { "10", "10%" }, { "20", "20%" }, { "30", "30%" },
            { "40", "40%" }, { "50", "50%" }, { "60", "60%" }, { "70", "70%" },
            { "80", "80%" }, { "90", "90%" }, { "100", "100%" },
            { nullptr, nullptr },
        },
        "100",
    });
}

} // namespace DolphinLibretro::CoreOptions::Audio
