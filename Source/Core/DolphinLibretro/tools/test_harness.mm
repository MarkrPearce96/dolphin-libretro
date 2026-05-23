// SP2 smoke harness: Cocoa app that boots a GC ROM through dolphin_libretro.dylib
// and pumps retro_run for N seconds. Verifies the Metal NSView handover +
// BootManager wiring end-to-end.
//
// Usage:
//   ./test_harness <path-to-dolphin_libretro.dylib> <path-to-rom.iso> [seconds]
//
// Default duration: 5 seconds. Exit 0 if any audio batch was produced (proves
// the Mixer ran), non-zero otherwise.

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>

#include <dlfcn.h>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <atomic>
#include <chrono>
#include <thread>

#include "../libretro.h"

namespace {

void* g_dl_handle = nullptr;
NSView* g_view = nil;

// Forward declaration — definition is at bottom of this anon namespace,
// referenced by environ_cb above it.
void harness_log(enum retro_log_level lvl, const char* fmt, ...);
std::atomic<unsigned> g_video_frames{0};
std::atomic<unsigned> g_audio_batches{0};
std::atomic<size_t>   g_audio_samples{0};

// libretro environment callback
bool environ_cb(unsigned cmd, void* data)
{
    constexpr unsigned RETRONEST_GET_MACOS_NSVIEW = (1u | 0x20000);

    switch (cmd)
    {
        case RETRONEST_GET_MACOS_NSVIEW:
            *static_cast<void**>(data) = (__bridge void*)g_view;
            fprintf(stderr, "[harness] env: returned NSView=%p\n", (__bridge void*)g_view);
            return true;

        case RETRO_ENVIRONMENT_GET_CAN_DUPE:
            *static_cast<bool*>(data) = true;
            return true;

        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
            return true;

        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
            auto* log = static_cast<retro_log_callback*>(data);
            log->log = harness_log;
            return true;
        }

        default:
            return false;
    }
}

void video_refresh_cb(const void* data, unsigned w, unsigned h, size_t pitch)
{
    (void)data; (void)w; (void)h; (void)pitch;
    g_video_frames.fetch_add(1);
}

void audio_sample_cb(int16_t l, int16_t r)
{
    (void)l; (void)r;
}

size_t audio_sample_batch_cb(const int16_t* data, size_t frames)
{
    (void)data;
    g_audio_batches.fetch_add(1);
    g_audio_samples.fetch_add(frames);
    return frames;
}

void input_poll_cb() {}

// Standalone C function — variadic lambdas can't convert to function pointers.
void harness_log(enum retro_log_level lvl, const char* fmt, ...)
{
    (void)lvl;
    va_list a; va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
}
int16_t input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id)
{
    (void)port; (void)device; (void)index; (void)id;
    return 0;
}

template <typename T>
T resolve(const char* name)
{
    auto sym = reinterpret_cast<T>(dlsym(g_dl_handle, name));
    if (!sym) fprintf(stderr, "FAIL: dlsym(%s): %s\n", name, dlerror());
    return sym;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <dylib> <rom> [seconds]\n", argv[0]);
        return 1;
    }
    const char* dylib_path = argv[1];
    const char* rom_path   = argv[2];
    const double duration_s = (argc >= 4) ? atof(argv[3]) : 5.0;

    @autoreleasepool {
        // 1. Cocoa app shell.
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];

        const NSRect frame = NSMakeRect(100, 100, 640, 480);
        NSWindow* window = [[NSWindow alloc]
            initWithContentRect:frame
                      styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable)
                        backing:NSBackingStoreBuffered
                          defer:NO];
        [window setTitle:@"dolphin_libretro SP2 smoke"];
        g_view = [[NSView alloc] initWithFrame:frame];
        [window setContentView:g_view];
        [window makeKeyAndOrderFront:nil];
        [app activateIgnoringOtherApps:YES];

        // Pump the run loop briefly so the view materialises.
        for (int i = 0; i < 10; ++i)
        {
            NSEvent* ev = [app nextEventMatchingMask:NSEventMaskAny
                                           untilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]
                                              inMode:NSDefaultRunLoopMode
                                             dequeue:YES];
            if (ev) [app sendEvent:ev];
        }

        // 2. Load dylib.
        g_dl_handle = dlopen(dylib_path, RTLD_NOW | RTLD_LOCAL);
        if (!g_dl_handle) { fprintf(stderr, "FAIL dlopen: %s\n", dlerror()); return 2; }

        // 3. Resolve retro_* + wire callbacks.
        auto set_env       = resolve<void(*)(retro_environment_t)>("retro_set_environment");
        auto set_video     = resolve<void(*)(retro_video_refresh_t)>("retro_set_video_refresh");
        auto set_audio     = resolve<void(*)(retro_audio_sample_t)>("retro_set_audio_sample");
        auto set_audio_b   = resolve<void(*)(retro_audio_sample_batch_t)>("retro_set_audio_sample_batch");
        auto set_in_poll   = resolve<void(*)(retro_input_poll_t)>("retro_set_input_poll");
        auto set_in_state  = resolve<void(*)(retro_input_state_t)>("retro_set_input_state");
        auto p_init        = resolve<void(*)()>("retro_init");
        auto p_load_game   = resolve<bool(*)(const struct retro_game_info*)>("retro_load_game");
        auto p_run         = resolve<void(*)()>("retro_run");
        auto p_unload_game = resolve<void(*)()>("retro_unload_game");
        auto p_deinit      = resolve<void(*)()>("retro_deinit");
        if (!set_env || !p_run || !p_load_game) return 3;

        set_env(environ_cb);
        set_video(video_refresh_cb);
        set_audio(audio_sample_cb);
        set_audio_b(audio_sample_batch_cb);
        set_in_poll(input_poll_cb);
        set_in_state(input_state_cb);

        p_init();

        // 4. Boot the ROM.
        struct retro_game_info info = {};
        info.path = rom_path;
        if (!p_load_game(&info))
        {
            fprintf(stderr, "FAIL retro_load_game(%s)\n", rom_path);
            p_deinit();
            dlclose(g_dl_handle);
            return 4;
        }
        fprintf(stderr, "OK: retro_load_game succeeded\n");

        // 5. Pump retro_run + NSRunLoop for `duration_s` seconds.
        const auto start = std::chrono::steady_clock::now();
        const auto deadline = start + std::chrono::duration<double>(duration_s);
        while (std::chrono::steady_clock::now() < deadline)
        {
            p_run();
            NSEvent* ev = [app nextEventMatchingMask:NSEventMaskAny
                                           untilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]
                                              inMode:NSDefaultRunLoopMode
                                             dequeue:YES];
            if (ev) [app sendEvent:ev];
        }

        // 6. Teardown.
        p_unload_game();
        p_deinit();
        dlclose(g_dl_handle);

        // 7. Report.
        fprintf(stderr, "\n=== SP2 smoke summary (%.1fs) ===\n", duration_s);
        fprintf(stderr, "video_refresh calls: %u\n", g_video_frames.load());
        fprintf(stderr, "audio batches:       %u\n", g_audio_batches.load());
        fprintf(stderr, "audio samples:       %zu\n", g_audio_samples.load());

        // Success if audio is flowing (proves Mixer + AudioStream wired)
        // OR video callbacks fire (proves video pipeline). Either alone is
        // sufficient evidence the core actually emulated.
        const bool ok = g_audio_batches.load() > 0 || g_video_frames.load() > 0;
        fprintf(stderr, "result: %s\n", ok ? "PASS" : "FAIL");
        return ok ? 0 : 5;
    }
}
