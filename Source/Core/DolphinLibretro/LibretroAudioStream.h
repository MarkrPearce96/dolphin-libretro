// Copyright 2024 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AudioCommon/SoundStream.h"
#include "DolphinLibretro/libretro.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <vector>

namespace DolphinLibretro {

// Libretro audio bridge: Dolphin's CPU thread pushes into this stream's Mixer
// (like any SoundStream backend); retro_run drains one host-frame's worth per
// call via DrainToFrontend() on the libretro thread — the only thread the
// libretro contract allows audio callbacks from.
//
// Survival: retro_load_game installs this stream BEFORE boot, and
// AudioCommon::InitSoundStream (SP9 guard) keeps a pre-installed stream
// instead of replacing it with a platform backend (Cubeb). Without that guard
// this object was destroyed during boot and Dolphin audio silently went
// Cubeb→OS, bypassing RetroNest's AudioSink, volume and mute entirely.
//
// Pacing: no thread, no sleeps. Each DrainToFrontend() call mixes and pushes
// the wall-clock time elapsed since the previous call (clamped), so the
// delivered rate is exactly kSampleRate regardless of the retro_run cadence.
// A fixed per-call amount is NOT enough: retro_run blocks in WaitForFrame,
// and when the game renders below 60 fps (30 fps cutscenes — e.g. Mario
// Sunshine's intro story) the retro_run rate drops with it, which starved
// the sink and crackled. Mixer::Mix gap-fills if the CPU thread starves it.
// Backpressure: frames the batch callback doesn't accept are stashed and
// retried first on the next drain (bounded; oldest dropped beyond the cap).
class LibretroAudioStream final : public SoundStream
{
public:
  // The rate the Mixer resamples everything to, and the rate reported in
  // retro_get_system_av_info. These MUST match — the previous code reported
  // 32000 while the base-class Mixer ran at 48000, a 1.5x pitch error waiting
  // to happen the moment the stream actually got used.
  static constexpr unsigned int kSampleRate = 48000;
  // Nominal drain at 60 fps; actual per-call amount is wall-clock based.
  static constexpr std::size_t kFramesPerRun = kSampleRate / 60;
  // Upper bound per drain (100 ms): caps the catch-up burst after a pause or
  // scheduler hiccup so a single call can't flood the sink.
  static constexpr std::size_t kMaxFramesPerDrain = kSampleRate / 10;

  LibretroAudioStream();
  ~LibretroAudioStream() override = default;

  bool Init() override;
  bool SetRunning(bool running) override;
  void SetVolume(int volume) override;

  // Called from retro_run (libretro thread) once per frame. Mixes the
  // wall-clock elapsed frames since the last call, applies the Dolphin
  // volume, pushes them through batch_cb, and stashes any unaccepted tail.
  void DrainToFrontend(retro_audio_sample_batch_t batch_cb);

private:
  // Push as much of m_pending as the frontend accepts; keep the rest.
  // Returns true if the stash fully drained.
  bool FlushPending(retro_audio_sample_batch_t batch_cb);

  std::atomic<bool> m_running{false};
  std::atomic<int> m_volume{100};  // 0..100, AudioCommon::UpdateSoundStream units
  std::vector<int16_t> m_mix_buffer;  // interleaved stereo scratch
  std::vector<int16_t> m_pending;     // frames the frontend refused (backpressure)
  std::chrono::steady_clock::time_point m_last_drain{};  // zero until first drain
};

}  // namespace DolphinLibretro
