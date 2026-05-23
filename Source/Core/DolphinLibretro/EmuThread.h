// SP2 T3: Real EmuThread coordinator.
//
// Wraps BootManager::BootCore / Core::Stop and provides a per-frame gate
// that retro_run can use to synchronise with Dolphin's video output.

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>

#include "Common/HookableEvent.h"
#include "Common/WindowSystemInfo.h"

namespace Core
{
class System;
}

namespace DolphinLibretro {

class EmuThread
{
public:
  EmuThread();
  ~EmuThread();

  EmuThread(const EmuThread&) = delete;
  EmuThread& operator=(const EmuThread&) = delete;

  // Boot a game. Returns false if already running or BootManager::BootCore fails.
  bool StartGame(const std::string& rom_path, const WindowSystemInfo& wsi);

  // Stop the running game. Safe to call when already stopped (idempotent).
  void StopGame();

  // Block until Dolphin fires its after_frame_event, or ~33 ms elapses
  // (one NTSC field period as a safety timeout).
  void WaitForFrame();

  // Map to Core::SetState Paused / Running.
  void SetPaused(bool paused);
  bool IsPaused() const { return m_paused.load(); }

  // True between a successful StartGame() and the completion of StopGame().
  bool IsRunning() const { return m_running.load(); }

private:
  // Registered as an after_frame_event listener while a game is running.
  // Called on the GPU/FIFO thread — must only touch the mutex/cv.
  void OnFrameEnd(Core::System& system);

  std::atomic<bool> m_running{false};
  std::atomic<bool> m_paused{false};

  std::mutex m_frame_mutex;
  std::condition_variable m_frame_cv;
  bool m_frame_ready{false};

  // RAII handle: keeps the after_frame_event listener alive while running.
  Common::EventHook m_frame_hook;
};

}  // namespace DolphinLibretro
