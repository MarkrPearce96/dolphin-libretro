// Bridges a host-owned NSView into a WindowSystemInfo that Dolphin's
// Metal backend can consume directly. The host (RetroNest, or a SP2 test
// harness) owns the NSView; we own the CAMetalLayer that lives on top
// of it.

#pragma once

struct WindowSystemInfo;

namespace DolphinLibretro::Metal {

// Wraps `nsview` with a fresh CAMetalLayer attached as the view's layer,
// populates `*out` with the layer, surface size, and the macOS-specific
// WindowSystemType. Returns true on success.
//
// Caller is responsible for the NSView's lifetime. The layer is owned by
// the NSView once attached (the NSView retains its layer).
//
// Must be called from the main thread (NSView access).
bool PrepareWindowSystemInfo(void* nsview, WindowSystemInfo* out);

// Tears down the CAMetalLayer attachment. Call when the backend is
// stopped. Safe to call multiple times.
void ReleaseWindowSystemInfo(WindowSystemInfo* wsi);

}  // namespace DolphinLibretro::Metal
