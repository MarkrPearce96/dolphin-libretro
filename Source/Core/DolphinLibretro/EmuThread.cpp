// SP2 T3: Real EmuThread implementation.
//
// Key design decisions:
//   - Dolphin's BootManager::BootCore() drives the CPU and FIFO threads
//     internally; we don't launch our own thread.
//   - We hook system.GetVideoEvents().after_frame_event (fires on the GPU /
//     FIFO thread after each XFB copy) to signal WaitForFrame().
//   - StopGame() unregisters the hook (by resetting m_frame_hook) before
//     calling Core::Stop() so no callbacks fire into a destroyed object.
//   - WaitForFrame() has a 33 ms timeout (≈one 30 Hz field) as a safety net
//     in case the hook never fires (e.g. the guest is still starting up).

#include "DolphinLibretro/EmuThread.h"

#include <chrono>

#include "Core/Boot/Boot.h"
#include "Core/BootManager.h"
#include "Core/Core.h"
#include "Core/System.h"
#include "DolphinLibretro/LibretroEnvironment.h"
#include "VideoCommon/VideoEvents.h"

namespace DolphinLibretro {

EmuThread::EmuThread() = default;

EmuThread::~EmuThread()
{
  StopGame();
}

bool EmuThread::StartGame(const std::string& rom_path, const WindowSystemInfo& wsi)
{
  if (m_running.load())
  {
    Environment::Log(RETRO_LOG_WARN, "EmuThread::StartGame: already running, ignoring.");
    return false;
  }

  auto boot_params = BootParameters::GenerateFromFile(rom_path);
  if (!boot_params)
  {
    Environment::Log(RETRO_LOG_ERROR,
                     "EmuThread::StartGame: BootParameters::GenerateFromFile failed for '%s'.",
                     rom_path.c_str());
    return false;
  }

  Core::System& system = Core::System::GetInstance();

  // Register our per-frame hook before booting so we don't miss the very
  // first frame signal.
  m_frame_hook = system.GetVideoEvents().after_frame_event.Register(
      [this](Core::System& sys) { OnFrameEnd(sys); });

  Environment::Log(RETRO_LOG_INFO, "EmuThread::StartGame: booting '%s'.", rom_path.c_str());

  if (!BootManager::BootCore(system, std::move(boot_params), wsi))
  {
    Environment::Log(RETRO_LOG_ERROR, "EmuThread::StartGame: BootManager::BootCore failed.");
    m_frame_hook.reset();
    return false;
  }

  m_running.store(true);
  m_paused.store(false);
  return true;
}

void EmuThread::StopGame()
{
  if (!m_running.load())
    return;

  Environment::Log(RETRO_LOG_INFO, "EmuThread::StopGame: stopping.");

  // Unregister frame hook first so no callbacks fire after we return.
  m_frame_hook.reset();

  // Wake any thread blocked in WaitForFrame() so it can unwind cleanly.
  {
    std::lock_guard<std::mutex> lk(m_frame_mutex);
    m_frame_ready = true;
  }
  m_frame_cv.notify_all();

  Core::System& system = Core::System::GetInstance();
  // Core::Stop only *requests* the stop — it halts the CPU and returns
  // immediately. Dolphin's emulation ("CPU-GPU") thread then unwinds and
  // tears down subsystems (ExpansionInterface, etc.) on its own. We must
  // join that thread before returning, or retro_unload_game's
  // UICommon::ShutdownControllers() and retro_deinit's UICommon::Shutdown()
  // race the still-running teardown → use-after-free crash in
  // ExpansionInterfaceManager::Shutdown(). Core::Shutdown() performs that
  // join (mirrors DolphinNoGUI's MainNoGUI: Core::Stop then Core::Shutdown).
  Core::Stop(system);
  Core::Shutdown(system);

  m_running.store(false);
  m_paused.store(false);

  // Reset the frame gate for the next StartGame() call.
  {
    std::lock_guard<std::mutex> lk(m_frame_mutex);
    m_frame_ready = false;
  }
}

void EmuThread::WaitForFrame()
{
  std::unique_lock<std::mutex> lk(m_frame_mutex);
  // 33 ms ≈ one 30 Hz NTSC field; also covers the case where the game is
  // still starting up and hasn't fired after_frame_event yet.
  m_frame_cv.wait_for(lk, std::chrono::milliseconds(33), [this] { return m_frame_ready; });
  m_frame_ready = false;
}

void EmuThread::SetPaused(bool paused)
{
  if (!m_running.load())
    return;

  Core::System& system = Core::System::GetInstance();
  const Core::State target = paused ? Core::State::Paused : Core::State::Running;
  Core::SetState(system, target);
  m_paused.store(paused);
}

// Called on the GPU / FIFO thread by Dolphin's HookableEvent machinery.
void EmuThread::OnFrameEnd(Core::System& /*system*/)
{
  {
    std::lock_guard<std::mutex> lk(m_frame_mutex);
    m_frame_ready = true;
  }
  m_frame_cv.notify_one();
}

}  // namespace DolphinLibretro
