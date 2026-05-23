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

    // Attach a CAMetalLayer to the view. setWantsLayer must be set before
    // setLayer (per AppKit docs) so the NSView becomes layer-backed.
    CAMetalLayer* layer = [CAMetalLayer layer];
    id<MTLDevice> mtl_device = MTLCreateSystemDefaultDevice();
    if (!mtl_device)
    {
        Environment::Log(RETRO_LOG_ERROR, "[Metal] no default MTLDevice");
        return false;
    }
    layer.device = mtl_device;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    layer.framebufferOnly = YES;

    dispatch_block_t attach = ^{
        view.wantsLayer = YES;
        view.layer = layer;
        const CGFloat scale = view.window ? view.window.backingScaleFactor : 1.0;
        layer.contentsScale = scale;
        const NSSize size = view.bounds.size;
        layer.drawableSize = NSMakeSize(size.width * scale, size.height * scale);
    };
    if ([NSThread isMainThread])
        attach();
    else
        dispatch_sync(dispatch_get_main_queue(), attach);

    // Populate WSI. The Metal backend reads render_surface as the layer.
    out->type = WindowSystemType::MacOS;
    out->display_connection = nullptr;
    out->render_window = (__bridge void*)view;
    out->render_surface = (__bridge_retained void*)layer;
    const CGFloat scale = view.window ? view.window.backingScaleFactor : 1.0;
    out->render_surface_scale = static_cast<float>(scale);

    Environment::Log(RETRO_LOG_INFO,
        "[Metal] WSI ready: view=%p layer=%p scale=%.2f size=%.0fx%.0f",
        nsview_raw, (__bridge void*)layer, scale,
        view.bounds.size.width, view.bounds.size.height);
    return true;
}

void ReleaseWindowSystemInfo(WindowSystemInfo* wsi)
{
    if (!wsi || !wsi->render_surface)
        return;

    // The layer was retained into render_surface via __bridge_retained;
    // CFRelease balances that.
    CFRelease(wsi->render_surface);
    wsi->render_surface = nullptr;
    wsi->render_window = nullptr;
}

}  // namespace DolphinLibretro::Metal
