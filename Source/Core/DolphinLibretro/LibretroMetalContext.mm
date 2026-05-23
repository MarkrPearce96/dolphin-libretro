#import "DolphinLibretro/LibretroMetalContext.h"
#import "DolphinLibretro/LibretroEnvironment.h"

#import <AppKit/AppKit.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>

#include "Common/WindowSystemInfo.h"

namespace DolphinLibretro::Metal {

bool PrepareWindowSystemInfo(void* nsview_raw, WindowSystemInfo* out)
{
    if (!nsview_raw || !out)
    {
        Environment::Log(RETRO_LOG_ERROR,
            "[Metal] PrepareWindowSystemInfo called with nsview=%p out=%p",
            nsview_raw, static_cast<void*>(out));
        return false;
    }

    NSView* view = (__bridge NSView*)nsview_raw;

    // Dolphin's Metal backend (VideoBackends/Metal/MTLMain.mm:PrepareWindow)
    // expects render_surface to be an NSView*. It does setWantsLayer:YES /
    // setLayer:newLayer itself and then overwrites render_surface with the
    // freshly-created CAMetalLayer*. We just hand it the view and let it do
    // the work — no pre-allocated layer, no double attachment.
    const CGFloat scale = view.window ? view.window.backingScaleFactor : 1.0;

    out->type = WindowSystemType::MacOS;
    out->display_connection = nullptr;
    out->render_window = (__bridge void*)view;
    out->render_surface = (__bridge void*)view;  // NSView, not layer
    out->render_surface_scale = static_cast<float>(scale);

    Environment::Log(RETRO_LOG_INFO,
        "[Metal] WSI ready: view=%p scale=%.2f size=%.0fx%.0f",
        nsview_raw, scale, view.bounds.size.width, view.bounds.size.height);
    return true;
}

void ReleaseWindowSystemInfo(WindowSystemInfo* wsi)
{
    if (!wsi)
        return;

    // PrepareWindow swapped render_surface from view → CAMetalLayer (Dolphin's
    // own layer). That layer is retained by the NSView (view.layer = layer);
    // the NSView is owned by the host. We just clear our pointers.
    wsi->render_surface = nullptr;
    wsi->render_window = nullptr;
}

}  // namespace DolphinLibretro::Metal
