// SP1: Skeleton EmuThread coordinator.
//
// Owns the pause flag and per-frame signaling primitive that retro_run
// will use to gate Dolphin's CPU/Fifo threads. SP1 keeps it minimal — no
// BootManager integration yet — so the build can prove it links. SP2
// adds the real Start()/Stop() wiring around BootManager::BootCore.

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace DolphinLibretro {

class EmuThread
{
public:
    EmuThread();
    ~EmuThread();

    EmuThread(const EmuThread&) = delete;
    EmuThread& operator=(const EmuThread&) = delete;

    // SP1: stub. SP2 spins up BootManager::BootCore + Dolphin's CPU/Fifo threads.
    void Start();

    // SP1: stub. SP2 calls Core::Stop + BootManager::Stop.
    void Stop();

    // SP1: stub. SP2 signals the CPU thread to advance one frame and waits
    // for the video output before returning.
    void RunFrame();

    void SetPaused(bool paused);
    bool IsPaused() const { return m_paused.load(); }

private:
    std::atomic<bool> m_paused{false};
    std::mutex m_frame_mutex;
    std::condition_variable m_frame_cv;
    bool m_frame_ready{false};
};

}  // namespace DolphinLibretro
