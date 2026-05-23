// Copyright 2024 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AudioCommon/SoundStream.h"

#include <atomic>
#include <thread>

namespace DolphinLibretro {

class LibretroAudioStream final : public SoundStream
{
public:
  LibretroAudioStream();
  ~LibretroAudioStream() override;

  bool Init() override;
  bool SetRunning(bool running) override;
  void SetVolume(int volume) override;

private:
  void DrainLoop();

  std::atomic<bool> m_running{false};
  std::atomic<bool> m_should_run{false};
  std::thread m_drain_thread;
};

}  // namespace DolphinLibretro
