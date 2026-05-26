// SPDX-FileCopyrightText: 2026 Mark Pearce (RetroNest)
// SPDX-License-Identifier: GPL-3.0+
//
// Standalone unit test for DolphinLibretro::MemoryMap::BuildDescriptors.
// Manual compile (no Dolphin libs needed):
//
//   cd Source/Core/DolphinLibretro/tools
//   clang++ -std=c++20 -I.. test_memory_map.cpp ../MemoryMap.cpp \
//       -o test_memory_map && ./test_memory_map

#include "../MemoryMap.h"

#include <cstdio>

using namespace DolphinLibretro::MemoryMap;

static int failures = 0;
static void ck(const char* l, bool ok)
{
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", l);
    if (!ok) ++failures;
}

int main()
{
    // Fake host pointers — never dereferenced, only carried into descriptors.
    auto* mem1 = reinterpret_cast<void*>(0x1000);
    auto* mem2 = reinterpret_cast<void*>(0x2000);

    // GameCube: one MEM1 descriptor
    {
        RamLayout gc{};
        gc.is_wii = false; gc.mem1 = mem1; gc.mem1_size = 24u * 1024 * 1024;
        const auto d = BuildDescriptors(gc);
        ck("gc: 1 descriptor", d.size() == 1);
        ck("gc: ptr == mem1", d[0].ptr == mem1);
        ck("gc: start 0x80000000", d[0].start == 0x80000000u);
        ck("gc: len 24MiB", d[0].len == 24u * 1024 * 1024);
        ck("gc: select 0", d[0].select == 0);
        ck("gc: SYSTEM_RAM flag", (d[0].flags & RETRO_MEMDESC_SYSTEM_RAM) != 0);
    }

    // Wii: MEM1 + MEM2
    {
        RamLayout wii{};
        wii.is_wii = true;
        wii.mem1 = mem1; wii.mem1_size = 24u * 1024 * 1024;
        wii.mem2 = mem2; wii.mem2_size = 64u * 1024 * 1024;
        const auto d = BuildDescriptors(wii);
        ck("wii: 2 descriptors", d.size() == 2);
        ck("wii: mem1 @ 0x80000000", d[0].start == 0x80000000u && d[0].ptr == mem1);
        ck("wii: mem2 @ 0x90000000", d[1].start == 0x90000000u && d[1].ptr == mem2);
        ck("wii: mem2 len 64MiB", d[1].len == 64u * 1024 * 1024);
    }

    // Wii with MEM2 not yet allocated — should give 1 descriptor (MEM1 only)
    {
        RamLayout wii_nomem2{};
        wii_nomem2.is_wii = true;
        wii_nomem2.mem1 = mem1; wii_nomem2.mem1_size = 24u * 1024 * 1024;
        // mem2 = nullptr, mem2_size = 0 (defaults)
        const auto d = BuildDescriptors(wii_nomem2);
        ck("wii no mem2: 1 descriptor", d.size() == 1);
        ck("wii no mem2: mem1 @ 0x80000000", d[0].start == 0x80000000u);
    }

    // Guard: no RAM → no descriptors
    {
        RamLayout empty{};
        ck("empty: 0 descriptors", BuildDescriptors(empty).empty());
    }

    std::printf("\n%s (%d failures)\n", failures ? "FAILED" : "OK", failures);
    return failures ? 1 : 0;
}
