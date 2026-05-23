// SP1: Host_* function stubs for the libretro core.
//
// Dolphin's core/uicommon static libraries reference 19 Host_* functions
// that the frontend (DolphinQt or DolphinNoGUI) is expected to provide.
// The libretro core has no UI to delegate to, so each is a no-op or a
// sensible default. Modeled on Source/Core/DolphinNoGUI/MainNoGUI.cpp's
// implementations of the same symbols.
//
// When SP2 wires up actual emulation, some of these will gain behavior
// (e.g. Host_Message handling WMUserStop to signal the EmuThread).

#include <string>
#include <vector>
#include <memory>

#include "Core/Host.h"

namespace HW::GBA { class Core; }
class GBAHostInterface;

std::vector<std::string> Host_GetPreferredLocales()
{
    return {};
}

void Host_PPCSymbolsChanged()
{
}

void Host_PPCBreakpointsChanged()
{
}

bool Host_UIBlocksControllerState()
{
    return false;
}

void Host_Message(HostMessageID id)
{
    // SP2 wires WMUserStop to signal EmuThread shutdown.
    (void)id;
}

void Host_UpdateTitle(const std::string& title)
{
    (void)title;
}

void Host_UpdateDisasmDialog()
{
}

void Host_JitCacheInvalidation()
{
}

void Host_JitProfileDataWiped()
{
}

void Host_RequestRenderWindowSize(int width, int height)
{
    (void)width;
    (void)height;
}

bool Host_RendererHasFocus()
{
    // Libretro frontend is always considered focused from Dolphin's POV;
    // pause-on-focus-loss is handled by RetroNest separately.
    return true;
}

bool Host_RendererHasFullFocus()
{
    return true;
}

bool Host_RendererIsFullscreen()
{
    // The host (RetroNest) owns the window; Dolphin shouldn't toggle.
    return false;
}

bool Host_TASInputHasFocus()
{
    return false;
}

void Host_YieldToUI()
{
}

void Host_TitleChanged()
{
}

void Host_UpdateDiscordClientID(const std::string& client_id)
{
    (void)client_id;
}

bool Host_UpdateDiscordPresenceRaw(const std::string& details, const std::string& state,
                                   const std::string& large_image_key,
                                   const std::string& large_image_text,
                                   const std::string& small_image_key,
                                   const std::string& small_image_text,
                                   const int64_t start_timestamp, const int64_t end_timestamp,
                                   const int party_size, const int party_max)
{
    (void)details; (void)state;
    (void)large_image_key; (void)large_image_text;
    (void)small_image_key; (void)small_image_text;
    (void)start_timestamp; (void)end_timestamp;
    (void)party_size; (void)party_max;
    return false;
}

std::unique_ptr<GBAHostInterface> Host_CreateGBAHost(std::weak_ptr<HW::GBA::Core> core)
{
    (void)core;
    return nullptr;
}
