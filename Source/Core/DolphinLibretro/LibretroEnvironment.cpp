#include "DolphinLibretro/LibretroEnvironment.h"

#include <cstdarg>
#include <cstdio>

namespace DolphinLibretro::Environment {

namespace {
retro_environment_t  s_environ_cb = nullptr;
retro_log_printf_t   s_log_cb     = nullptr;
}  // namespace

void SetEnvironmentCallback(retro_environment_t cb) { s_environ_cb = cb; }
retro_environment_t GetEnvironmentCallback()        { return s_environ_cb; }

void SetLogCallback(retro_log_printf_t cb) { s_log_cb = cb; }

void Log(enum retro_log_level level, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (s_log_cb)
    {
        // retro_log_printf_t takes (level, fmt, ...) — no vprintf variant
        // in the standard libretro header, so format locally and forward.
        char buf[1024];
        vsnprintf(buf, sizeof(buf), fmt, args);
        s_log_cb(level, "%s", buf);
    }
    else
    {
        // Fallback so smoke tests without retro_log still see output.
        vfprintf(stderr, fmt, args);
        fputc('\n', stderr);
    }
    va_end(args);
}

void* RequestHostNSView()
{
    if (!s_environ_cb)
    {
        Log(RETRO_LOG_ERROR, "[Environment] no environ_cb registered");
        return nullptr;
    }
    void* nsview = nullptr;
    if (!s_environ_cb(RETRONEST_GET_MACOS_NSVIEW, &nsview) || !nsview)
    {
        Log(RETRO_LOG_ERROR,
            "[Environment] host did not provide NSView via RETRONEST_GET_MACOS_NSVIEW (0x%x)",
            RETRONEST_GET_MACOS_NSVIEW);
        return nullptr;
    }
    Log(RETRO_LOG_INFO, "[Environment] got NSView=%p", nsview);
    return nsview;
}

}  // namespace DolphinLibretro::Environment
