// SP1: Skeleton EmuThread implementation. See EmuThread.h for SP2 plans.

#include "DolphinLibretro/EmuThread.h"

namespace DolphinLibretro {

EmuThread::EmuThread() = default;
EmuThread::~EmuThread() = default;

void EmuThread::Start()
{
    // SP2: BootManager::BootCore + spin up Dolphin's CPU/Fifo threads.
}

void EmuThread::Stop()
{
    // SP2: Core::Stop + BootManager::Stop + join threads.
}

void EmuThread::RunFrame()
{
    // SP2: signal the CPU thread to advance one frame, wait for video, return.
}

void EmuThread::SetPaused(bool paused)
{
    m_paused.store(paused);
}

}  // namespace DolphinLibretro
