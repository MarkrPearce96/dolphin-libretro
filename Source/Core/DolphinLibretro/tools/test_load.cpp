// SP1 smoke test: dlopen the built dolphin_libretro.dylib, verify the
// retro_* ABI is callable, retro_load_game(nullptr) returns false cleanly.
//
// Doesn't link any Dolphin code — purely a libretro-frontend-perspective
// load test. Stand-alone so it can run in CI without needing RetroArch
// installed.
//
// Usage:
//   ./dolphin_libretro_load_test <path-to-dolphin_libretro.dylib>

#include <dlfcn.h>
#include <cstdio>
#include <cstring>

#include "../libretro.h"

namespace {

void* g_handle = nullptr;

template <typename T>
T resolve(const char* name)
{
    T sym = reinterpret_cast<T>(dlsym(g_handle, name));
    if (!sym)
    {
        fprintf(stderr, "FAIL: dlsym(%s): %s\n", name, dlerror());
    }
    return sym;
}

// libretro environment callback the core may invoke during retro_init.
// Returns false for any request — we're not implementing any environment
// extensions in this smoke test.
bool environ_cb(unsigned cmd, void* data)
{
    (void)cmd; (void)data;
    return false;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <path-to-dolphin_libretro.dylib>\n", argv[0]);
        return 1;
    }

    g_handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!g_handle)
    {
        fprintf(stderr, "FAIL: dlopen: %s\n", dlerror());
        return 2;
    }

    // 1. retro_api_version matches our header.
    auto p_retro_api_version = resolve<unsigned(*)(void)>("retro_api_version");
    if (!p_retro_api_version) { dlclose(g_handle); return 3; }
    const unsigned ver = p_retro_api_version();
    if (ver != RETRO_API_VERSION)
    {
        fprintf(stderr, "FAIL: retro_api_version mismatch: got %u, expected %u\n",
                ver, RETRO_API_VERSION);
        dlclose(g_handle);
        return 4;
    }
    printf("OK: retro_api_version = %u\n", ver);

    // 2. retro_get_system_info returns sensible identification.
    auto p_get_sys_info =
        resolve<void(*)(struct retro_system_info*)>("retro_get_system_info");
    if (!p_get_sys_info) { dlclose(g_handle); return 5; }
    struct retro_system_info info = {};
    p_get_sys_info(&info);
    if (!info.library_name || strcmp(info.library_name, "Dolphin") != 0)
    {
        fprintf(stderr, "FAIL: library_name = %s (expected Dolphin)\n",
                info.library_name ? info.library_name : "(null)");
        dlclose(g_handle);
        return 6;
    }
    printf("OK: library = %s v%s\n",
           info.library_name, info.library_version ? info.library_version : "");
    printf("OK: extensions = %s\n", info.valid_extensions ? info.valid_extensions : "");

    // 3. retro_set_environment + retro_init lifecycle works cleanly.
    auto p_set_env = resolve<void(*)(retro_environment_t)>("retro_set_environment");
    auto p_init    = resolve<void(*)(void)>("retro_init");
    auto p_deinit  = resolve<void(*)(void)>("retro_deinit");
    if (!p_set_env || !p_init || !p_deinit) { dlclose(g_handle); return 7; }
    p_set_env(environ_cb);
    p_init();
    printf("OK: retro_init returned cleanly\n");

    // 4. retro_load_game(nullptr) returns false (skeleton behavior).
    auto p_load = resolve<bool(*)(const struct retro_game_info*)>("retro_load_game");
    if (!p_load) { p_deinit(); dlclose(g_handle); return 8; }
    const bool loaded = p_load(nullptr);
    if (loaded)
    {
        fprintf(stderr, "FAIL: retro_load_game(nullptr) returned true (skeleton should return false)\n");
        p_deinit();
        dlclose(g_handle);
        return 9;
    }
    printf("OK: retro_load_game(nullptr) = false (skeleton)\n");

    // 5. Clean shutdown.
    p_deinit();
    printf("OK: retro_deinit returned cleanly\n");

    if (dlclose(g_handle) != 0)
    {
        fprintf(stderr, "FAIL: dlclose: %s\n", dlerror());
        return 10;
    }
    printf("OK: dlclose returned cleanly\n");

    printf("\nSP1 load test: ALL PASS\n");
    return 0;
}
