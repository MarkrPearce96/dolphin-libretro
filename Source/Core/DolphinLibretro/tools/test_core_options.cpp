// SPDX-FileCopyrightText: 2026 Mark Pearce (RetroNest)
// SPDX-License-Identifier: GPL-3.0+
//
// Standalone unit test for DolphinLibretro::CoreOptions::Graphics::Parse +
// CoreOptions::BuildDefinitions. Not part of the dolphin_libretro target.
// Manual compile (CORE_OPTIONS_TEST_ONLY gates out Apply + engine deps):
//
//   cd Source/Core/DolphinLibretro/tools
//   clang++ -std=c++20 -I.. -I../../.. -DCORE_OPTIONS_TEST_ONLY \
//       test_core_options.cpp ../CoreOptions.cpp ../CoreOptionsGraphics.cpp \
//       -o test_core_options && ./test_core_options

#include "../CoreOptions.h"
#include "../CoreOptionsGraphics.h"

#include <cstdio>
#include <cstring>
#include <map>
#include <string>

using namespace DolphinLibretro::CoreOptions;

namespace fake {
    std::map<std::string, std::string> vars;
    void reset() { vars.clear(); }
}
static bool fake_cb(unsigned cmd, void* data) {
    if (cmd == RETRO_ENVIRONMENT_GET_VARIABLE) {
        auto* v = static_cast<retro_variable*>(data);
        if (!v || !v->key) return false;
        auto it = fake::vars.find(v->key);
        if (it == fake::vars.end()) { v->value = nullptr; return false; }
        v->value = it->second.c_str();
        return true;
    }
    return false;
}

static int failures = 0;
static void ck_int(const char* l, long got, long want) {
    bool ok = got == want;
    std::printf("[%s] %s: got=%ld want=%ld\n", ok ? "PASS" : "FAIL", l, got, want);
    if (!ok) ++failures;
}
static void ck_bool(const char* l, bool got, bool want) {
    bool ok = got == want;
    std::printf("[%s] %s: got=%d want=%d\n", ok ? "PASS" : "FAIL", l, got, want);
    if (!ok) ++failures;
}
static void ck_str(const char* l, const std::string& got, const char* want) {
    bool ok = got == want;
    std::printf("[%s] %s: got='%s' want='%s'\n", ok ? "PASS" : "FAIL", l, got.c_str(), want);
    if (!ok) ++failures;
}

int main() {
    // ── BuildDefinitions structure ──
    const auto& defs = BuildDefinitions();
    ck_bool("terminator present", defs.back().key == nullptr, true);
    {
        std::map<std::string,int> seen; bool dup = false;
        for (const auto& d : defs) if (d.key) if (++seen[d.key] > 1) dup = true;
        ck_bool("no duplicate keys", dup, false);
    }

    // ── General Parse ──
    fake::reset();
    fake::vars["dolphin_aspect_ratio"]  = "16:9";
    fake::vars["dolphin_vsync"]         = "disabled";
    fake::vars["dolphin_shader_compilation"] = "Skip Drawing";
    Graphics::Values g{};
    Graphics::Parse(&fake_cb, g);
    ck_str ("General aspect",   g.general.aspect_ratio, "16:9");
    ck_bool("General vsync",     g.general.vsync, false);
    ck_str ("General shaderc",   g.general.shader_compilation, "Skip Drawing");
    ck_bool("General pft default (unset)", g.general.precision_frame_timing, true);

    std::printf("\n%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
