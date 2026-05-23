// RetroNest-private libretro environment extensions.
//
// These extensions are not part of the standard libretro API. They live in
// the RETRO_ENVIRONMENT_PRIVATE range (0x20000+) which the libretro spec
// reserves for frontend↔core private contracts. The numeric values MUST
// match RetroNest's environment_callbacks.h definitions exactly.

#pragma once

#include "DolphinLibretro/libretro.h"

namespace DolphinLibretro::Environment {

// Hand a host-owned NSView* to the core for Metal rendering.
// retro_environment_t cb writes a (void* NSView) into the data ptr.
// Matches RETRONEST_ENVIRONMENT_GET_MACOS_NSVIEW = (1 | RETRO_ENVIRONMENT_PRIVATE).
constexpr unsigned RETRONEST_GET_MACOS_NSVIEW = (1u | RETRO_ENVIRONMENT_PRIVATE);

// Stores the frontend's environ_cb at retro_set_environment time so other
// modules can use it. Caller-friendly wrappers below.
void SetEnvironmentCallback(retro_environment_t cb);
retro_environment_t GetEnvironmentCallback();

// Requests an NSView from the host. Returns nullptr if the env extension
// is unsupported or the host returned no view. Logs via retro_log_cb if
// it's been set.
void* RequestHostNSView();

// Similar accessor for the frontend log callback (set during retro_init).
void SetLogCallback(retro_log_printf_t cb);
void Log(enum retro_log_level level, const char* fmt, ...) __attribute__((format(printf, 2, 3)));

}  // namespace DolphinLibretro::Environment
