// Copyright 2024 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinLibretro/LibretroAudioStream.h"
#include "DolphinLibretro/LibretroEnvironment.h"
#include "DolphinLibretro/libretro.h"

#include "AudioCommon/Mixer.h"

#include <array>
#include <chrono>

namespace DolphinLibretro {

namespace {
// ~5 ms at 32 kHz = 160 frames. Low latency, low callback overhead.
constexpr std::size_t kBatchFrames = 160;
}  // namespace

LibretroAudioStream::LibretroAudioStream() = default;

LibretroAudioStream::~LibretroAudioStream()
{
  SetRunning(false);
}

bool LibretroAudioStream::Init()
{
  Environment::Log(RETRO_LOG_INFO, "[Audio] LibretroAudioStream::Init");
  return true;
}

bool LibretroAudioStream::SetRunning(bool running)
{
  if (running == m_running.exchange(running))
    return true;

  if (running)
  {
    m_should_run.store(true);
    m_drain_thread = std::thread([this] { DrainLoop(); });
    Environment::Log(RETRO_LOG_INFO, "[Audio] drain thread started");
  }
  else
  {
    m_should_run.store(false);
    if (m_drain_thread.joinable())
      m_drain_thread.join();
    Environment::Log(RETRO_LOG_INFO, "[Audio] drain thread stopped");
  }
  return true;
}

void LibretroAudioStream::SetVolume(int volume)
{
  (void)volume;
}

void LibretroAudioStream::DrainLoop()
{
  std::array<int16_t, kBatchFrames * 2> buf{};  // stereo s16
  Mixer* mixer = GetMixer();

  while (m_should_run.load())
  {
    if (!mixer)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      continue;
    }

    mixer->Mix(buf.data(), kBatchFrames);
    // T5 will replace this stub with: Frontend::g_audio_batch_cb(buf.data(), kBatchFrames);
    Environment::Log(RETRO_LOG_DEBUG, "[Audio] would push %zu frames", kBatchFrames);
  }
}

}  // namespace DolphinLibretro
