// Copyright 2024 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinLibretro/LibretroAudioStream.h"
#include "DolphinLibretro/LibretroEnvironment.h"

#include "AudioCommon/Mixer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>

namespace DolphinLibretro {

namespace {
// Backpressure stash cap: if the frontend refuses more than ~4 host frames of
// audio, it is genuinely saturated (RetroNest's AudioSink caps its queue at
// 150 ms) — drop the oldest instead of growing without bound. Interleaved
// stereo, so 2 samples per frame.
constexpr std::size_t kMaxPendingSamples = 4 * LibretroAudioStream::kFramesPerRun * 2;
}  // namespace

LibretroAudioStream::LibretroAudioStream()
{
  // Be explicit about the mix rate instead of relying on the base-class
  // default staying 48000 — av_info reports kSampleRate and the two must
  // never drift apart (see header).
  m_mixer = std::make_unique<Mixer>(kSampleRate);
  m_mix_buffer.resize(kMaxFramesPerDrain * 2);
  m_pending.reserve(kMaxPendingSamples);
}

bool LibretroAudioStream::Init()
{
  Environment::Log(RETRO_LOG_INFO, "[Audio] LibretroAudioStream::Init (mix rate %u)",
                   kSampleRate);
  return true;
}

bool LibretroAudioStream::SetRunning(bool running)
{
  // Called by AudioCommon::SetSoundStreamRunning (PostInitSoundStream after
  // boot, ShutdownSoundStream at exit). No thread to start/stop anymore —
  // this only gates DrainToFrontend.
  m_running.store(running);
  // Restart the drain clock so time spent stopped (boot, pause) isn't
  // "caught up" as a burst of stale audio on resume.
  m_last_drain = std::chrono::steady_clock::time_point{};
  Environment::Log(RETRO_LOG_INFO, "[Audio] SetRunning(%s)", running ? "true" : "false");
  return true;
}

void LibretroAudioStream::SetVolume(int volume)
{
  // AudioCommon::UpdateSoundStream passes 0..100 (0 when muted). Applied in
  // DrainToFrontend — the Mixer has no volume of its own; platform backends
  // (Cubeb) apply it in their output layer, so we do the equivalent here.
  // This is what makes the dolphin_audio_volume core option actually work.
  m_volume.store(std::clamp(volume, 0, 100));
}

bool LibretroAudioStream::FlushPending(retro_audio_sample_batch_t batch_cb)
{
  if (m_pending.empty())
    return true;

  const std::size_t pending_frames = m_pending.size() / 2;
  const std::size_t accepted = batch_cb(m_pending.data(), pending_frames);
  if (accepted >= pending_frames)
  {
    m_pending.clear();
    return true;
  }
  // Shift the unaccepted remainder down and try again next drain.
  m_pending.erase(m_pending.begin(), m_pending.begin() + accepted * 2);
  return false;
}

void LibretroAudioStream::DrainToFrontend(retro_audio_sample_batch_t batch_cb)
{
  if (!batch_cb || !m_running.load())
    return;

  // 1. Retry what the frontend refused last time, before mixing anything new
  //    — order must be preserved. If it still won't drain, don't mix more:
  //    the mixer keeps buffering (its FIFO drops oldest internally if we stay
  //    saturated for long, which is the right failure mode).
  if (!FlushPending(batch_cb))
  {
    if (m_pending.size() > kMaxPendingSamples)
      m_pending.erase(m_pending.begin(),
                      m_pending.begin() + (m_pending.size() - kMaxPendingSamples));
    return;
  }

  // 2. Mix the wall-clock elapsed audio since the last drain. retro_run's
  //    cadence is NOT reliable pacing — WaitForFrame drops it below 60 Hz
  //    whenever the game renders slower (30 fps cutscenes) — so a fixed
  //    per-call amount starves the sink and crackles. Mix() always fills the
  //    request (gap-fill on underrun).
  const auto now = std::chrono::steady_clock::now();
  std::size_t frames = kFramesPerRun;
  if (m_last_drain != std::chrono::steady_clock::time_point{})
  {
    const double elapsed = std::chrono::duration<double>(now - m_last_drain).count();
    if (elapsed > 0.0)
      frames = static_cast<std::size_t>(std::lround(elapsed * kSampleRate));
  }
  m_last_drain = now;
  frames = std::clamp<std::size_t>(frames, 0, kMaxFramesPerDrain);
  if (frames == 0)
    return;
  m_mixer->Mix(m_mix_buffer.data(), static_cast<unsigned int>(frames));

  // 3. Apply Dolphin's volume (0..100). 100 is the common case — skip the pass.
  const int volume = m_volume.load();
  if (volume < 100)
  {
    for (std::size_t i = 0; i < frames * 2; ++i)
      m_mix_buffer[i] = static_cast<int16_t>((static_cast<int>(m_mix_buffer[i]) * volume) / 100);
  }

  // 4. Push; stash any unaccepted tail for the next drain.
  const std::size_t accepted = batch_cb(m_mix_buffer.data(), frames);
  if (accepted < frames)
  {
    m_pending.insert(m_pending.end(), m_mix_buffer.begin() + accepted * 2,
                     m_mix_buffer.begin() + frames * 2);
  }
}

}  // namespace DolphinLibretro
