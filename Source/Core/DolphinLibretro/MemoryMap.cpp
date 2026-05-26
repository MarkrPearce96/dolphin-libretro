// SPDX-FileCopyrightText: 2026 Mark Pearce (RetroNest)
// SPDX-License-Identifier: GPL-3.0+

#include "MemoryMap.h"

namespace DolphinLibretro::MemoryMap
{
static retro_memory_descriptor MakeRam(void* ptr, std::uint32_t size, std::size_t start)
{
    retro_memory_descriptor d{};
    d.flags      = RETRO_MEMDESC_SYSTEM_RAM;
    d.ptr        = ptr;
    d.offset     = 0;
    d.start      = start;
    d.select     = 0;  // rcheevos range-matches when select==0 (rc_libretro.c:513)
    d.disconnect = 0;
    d.len        = size;
    d.addrspace  = nullptr;
    return d;
}

std::vector<retro_memory_descriptor> BuildDescriptors(const RamLayout& layout)
{
    std::vector<retro_memory_descriptor> out;
    if (!layout.mem1 || layout.mem1_size == 0)
        return out;  // RAM not allocated yet — caller retries later

    // MEM1 — GameCube + Wii main RAM at emulated 0x80000000.
    out.push_back(MakeRam(layout.mem1, layout.mem1_size, 0x80000000u));

    // MEM2 — Wii extended RAM at emulated 0x90000000.
    if (layout.is_wii && layout.mem2 && layout.mem2_size != 0)
        out.push_back(MakeRam(layout.mem2, layout.mem2_size, 0x90000000u));

    return out;
}

}  // namespace DolphinLibretro::MemoryMap
