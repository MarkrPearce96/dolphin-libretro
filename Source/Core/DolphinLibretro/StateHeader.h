// SPDX-FileCopyrightText: 2026 Mark Pearce (RetroNest)
// SPDX-License-Identifier: GPL-3.0+
//
// 16-byte version header prepended to retro_serialize buffers.
//
// Dolphin's file savestates carry an EXTENDED_HEADER_VERSION + STATE_VERSION
// cookie, but the raw buffer path (State::SaveToBuffer) starts straight at
// DoState — and RetroNest persists retro_serialize buffers as .resume files
// across core updates. Loading a blob produced by a different STATE_VERSION
// walks DoState with mismatched layout and corrupts/crashes the core, so we
// stamp the version up front and reject mismatches in retro_unserialize.
//
// Layout (all fields little-endian u32, independent of host endianness):
//   [0]  magic          'D','O','L','R' byte sequence
//   [4]  header_version kHeaderVersion (bump if this layout ever changes)
//   [8]  state_version  Dolphin's STATE_VERSION at save time
//   [12] reserved       0
//
// Blobs that do NOT start with the magic are legacy headerless states from
// older core builds; callers should load them as-is (grandfathering existing
// .resume files) after logging a warning.
//
// Pure and header-only so tools/test_state_header.cpp compiles standalone.

#pragma once

#include <cstddef>
#include <cstdint>

namespace DolphinLibretro::StateHeader
{

constexpr size_t kHeaderSize = 16;
constexpr uint32_t kMagic = 0x524C4F44u;  // bytes 'D','O','L','R' when stored LE
constexpr uint32_t kHeaderVersion = 1;

struct Header
{
    uint32_t magic = 0;
    uint32_t header_version = 0;
    uint32_t state_version = 0;
    uint32_t reserved = 0;
};

enum class ParseResult
{
    Ok,                    // valid header, state_version matches; payload at data + kHeaderSize
    Legacy,                // no magic: headerless blob, the whole buffer is the payload
    Truncated,             // magic present but buffer < kHeaderSize — corrupt, reject
    BadHeaderVersion,      // header_version != kHeaderVersion — reject
    StateVersionMismatch,  // state_version != current STATE_VERSION — reject
};

inline void PutU32(uint8_t* dst, uint32_t v)
{
    dst[0] = static_cast<uint8_t>(v);
    dst[1] = static_cast<uint8_t>(v >> 8);
    dst[2] = static_cast<uint8_t>(v >> 16);
    dst[3] = static_cast<uint8_t>(v >> 24);
}

inline uint32_t GetU32(const uint8_t* src)
{
    return static_cast<uint32_t>(src[0]) | (static_cast<uint32_t>(src[1]) << 8) |
           (static_cast<uint32_t>(src[2]) << 16) | (static_cast<uint32_t>(src[3]) << 24);
}

// Writes the kHeaderSize-byte header into dst (caller guarantees room).
inline void Pack(uint8_t* dst, uint32_t state_version)
{
    PutU32(dst + 0, kMagic);
    PutU32(dst + 4, kHeaderVersion);
    PutU32(dst + 8, state_version);
    PutU32(dst + 12, 0);  // reserved
}

// Classifies a serialized blob against the running core's STATE_VERSION.
// On any result except Legacy/Truncated, *out (if non-null) holds the decoded
// header so callers can log the offending versions.
inline ParseResult Parse(const uint8_t* data, size_t size, uint32_t current_state_version,
                         Header* out = nullptr)
{
    if (!data || size < 4 || GetU32(data) != kMagic)
        return ParseResult::Legacy;
    if (size < kHeaderSize)
        return ParseResult::Truncated;

    Header h;
    h.magic = GetU32(data + 0);
    h.header_version = GetU32(data + 4);
    h.state_version = GetU32(data + 8);
    h.reserved = GetU32(data + 12);
    if (out)
        *out = h;

    if (h.header_version != kHeaderVersion)
        return ParseResult::BadHeaderVersion;
    if (h.state_version != current_state_version)
        return ParseResult::StateVersionMismatch;
    return ParseResult::Ok;
}

}  // namespace DolphinLibretro::StateHeader
