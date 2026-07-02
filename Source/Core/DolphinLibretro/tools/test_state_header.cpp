// SPDX-FileCopyrightText: 2026 Mark Pearce (RetroNest)
// SPDX-License-Identifier: GPL-3.0+
//
// Standalone unit test for DolphinLibretro::StateHeader (the 16-byte version
// header prepended to retro_serialize buffers).
// Manual compile (no Dolphin libs needed):
//
//   cd Source/Core/DolphinLibretro/tools
//   clang++ -std=c++20 -I.. test_state_header.cpp \
//       -o test_state_header && ./test_state_header

#include "../StateHeader.h"

#include <cstdio>
#include <cstring>

using namespace DolphinLibretro::StateHeader;

static int failures = 0;
static void ck(const char* l, bool ok)
{
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", l);
    if (!ok) ++failures;
}

int main()
{
    constexpr uint32_t kStateVer = 190;

    // Round-trip: Pack then Parse → Ok, fields decoded, payload after header.
    {
        uint8_t buf[kHeaderSize + 4] = {};
        Pack(buf, kStateVer);
        buf[kHeaderSize] = 0xAB;  // first payload byte survives untouched

        // On-wire bytes are little-endian 'D','O','L','R' regardless of host.
        ck("magic bytes D O L R",
           buf[0] == 'D' && buf[1] == 'O' && buf[2] == 'L' && buf[3] == 'R');

        Header h{};
        ck("round-trip: Ok", Parse(buf, sizeof(buf), kStateVer, &h) == ParseResult::Ok);
        ck("round-trip: magic", h.magic == kMagic);
        ck("round-trip: header_version", h.header_version == kHeaderVersion);
        ck("round-trip: state_version", h.state_version == kStateVer);
        ck("round-trip: reserved 0", h.reserved == 0);
        ck("round-trip: payload intact", buf[kHeaderSize] == 0xAB);

        // Exactly header-sized buffer (empty payload) still parses Ok.
        ck("round-trip: exact 16 bytes Ok", Parse(buf, kHeaderSize, kStateVer) == ParseResult::Ok);
    }

    // Wrong magic → Legacy (whole buffer treated as headerless payload).
    {
        uint8_t buf[64];
        std::memset(buf, 0x5A, sizeof(buf));  // arbitrary non-magic bytes
        ck("wrong magic: Legacy", Parse(buf, sizeof(buf), kStateVer) == ParseResult::Legacy);
    }

    // State version mismatch → reject.
    {
        uint8_t buf[kHeaderSize];
        Pack(buf, kStateVer - 1);  // saved by an older core build
        Header h{};
        ck("older state_version: mismatch",
           Parse(buf, sizeof(buf), kStateVer, &h) == ParseResult::StateVersionMismatch);
        ck("older state_version: decoded for logging", h.state_version == kStateVer - 1);
        Pack(buf, kStateVer + 1);  // saved by a newer core build
        ck("newer state_version: mismatch",
           Parse(buf, sizeof(buf), kStateVer) == ParseResult::StateVersionMismatch);
    }

    // Unknown header layout version → reject.
    {
        uint8_t buf[kHeaderSize];
        Pack(buf, kStateVer);
        PutU32(buf + 4, kHeaderVersion + 1);
        ck("future header_version: BadHeaderVersion",
           Parse(buf, sizeof(buf), kStateVer) == ParseResult::BadHeaderVersion);
    }

    // Short buffers.
    {
        uint8_t buf[kHeaderSize];
        Pack(buf, kStateVer);
        // Magic present but header cut off → corrupt, reject.
        ck("magic + 8 bytes: Truncated", Parse(buf, 8, kStateVer) == ParseResult::Truncated);
        ck("magic + 4 bytes: Truncated", Parse(buf, 4, kStateVer) == ParseResult::Truncated);
        // Too short to even hold the magic → cannot be a headered blob → Legacy.
        ck("3 bytes: Legacy", Parse(buf, 3, kStateVer) == ParseResult::Legacy);
        ck("0 bytes: Legacy", Parse(buf, 0, kStateVer) == ParseResult::Legacy);
        ck("null data: Legacy", Parse(nullptr, 32, kStateVer) == ParseResult::Legacy);
    }

    std::printf("\n%s (%d failures)\n", failures ? "FAILED" : "OK", failures);
    return failures ? 1 : 0;
}
