// SPDX-FileCopyrightText: 2026 Mark Pearce (RetroNest)
// SPDX-License-Identifier: GPL-3.0+
//
// SP8: pure builder that turns a booted system's RAM layout into the libretro
// memory descriptors rcheevos expects for GameCube (console 16) and Wii (19).
// No Dolphin engine dependencies — unit-testable standalone (see tools/test_memory_map.cpp).

#pragma once

#include <cstdint>
#include <vector>

#include "libretro.h"

namespace DolphinLibretro::MemoryMap
{
// Host pointers + sizes for the booted system's RAM. mem2 is Wii-only.
struct RamLayout
{
    bool        is_wii    = false;
    void*       mem1      = nullptr;  // MEM1 / main RAM (MemoryManager::GetRAM())
    std::uint32_t mem1_size = 0;      // GetRamSizeReal() — 24 MiB
    void*       mem2      = nullptr;  // MEM2 (Wii only, GetEXRAM())
    std::uint32_t mem2_size = 0;      // GetExRamSizeReal() — 64 MiB
};

// MEM1 @ emulated address 0x80000000 always; MEM2 @ 0x90000000 when is_wii.
// select=0 (range-matched by rcheevos). Returns empty if mem1 is null/zero-size.
std::vector<retro_memory_descriptor> BuildDescriptors(const RamLayout& layout);

}  // namespace DolphinLibretro::MemoryMap
