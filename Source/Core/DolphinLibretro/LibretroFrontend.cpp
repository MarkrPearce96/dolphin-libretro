// SP2: retro_* C ABI entrypoints — real implementation.
//
// retro_load_game boots a GameCube/Wii ROM via BootManager::BootCore.
// retro_run advances Dolphin by ~1 frame and the libretro audio batch
// callback gets samples via LibretroAudioStream's drain thread (which
// reads g_audio_batch_cb directly). Video is pushed into the host's
// CAMetalLayer by Dolphin's Metal backend — no video_refresh_cb call
// is needed since the libretro frontend (RetroNest) composites the
// layer directly.

#include "DolphinLibretro/libretro.h"
#include "DolphinLibretro/EmuThread.h"
#include "DolphinLibretro/LibretroEnvironment.h"
#include "CoreOptions.h"
#include "DolphinLibretro/LibretroMetalContext.h"
#include "DolphinLibretro/LibretroAudioStream.h"
#include "DolphinLibretro/LibretroInputSource.h"

#include "Common/Config/Config.h"
#include "Common/Logging/Log.h"
#include "Common/Logging/LogManager.h"
#include "Common/WindowSystemInfo.h"
#include "Core/Config/MainSettings.h"
#include "Core/Core.h"
#include "Core/System.h"
#include "UICommon/UICommon.h"

#include <cstdlib>
#include <memory>
#include <string>

namespace DolphinLibretro::Frontend {

// Callback storage — accessible to sibling TUs via extern declarations.
retro_environment_t        g_environ_cb         = nullptr;
retro_video_refresh_t      g_video_refresh_cb   = nullptr;
retro_audio_sample_t       g_audio_sample_cb    = nullptr;
retro_audio_sample_batch_t g_audio_batch_cb     = nullptr;
retro_input_poll_t         g_input_poll_cb      = nullptr;
retro_input_state_t        g_input_state_cb     = nullptr;

}  // namespace DolphinLibretro::Frontend

namespace {

std::unique_ptr<DolphinLibretro::EmuThread> s_emu_thread;
WindowSystemInfo                             s_wsi{};

}  // namespace

extern "C" {

RETRO_API unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

RETRO_API void retro_get_system_info(struct retro_system_info* info)
{
    info->library_name     = "Dolphin";
    info->library_version  = "SP2";
    info->valid_extensions = "iso|gcm|gcz|ciso|wbfs|rvz|wad|wia|nkit|m3u|dol|elf|tgc";
    info->need_fullpath    = true;
    info->block_extract    = false;
}

RETRO_API void retro_get_system_av_info(struct retro_system_av_info* info)
{
    info->geometry.base_width   = 640;
    info->geometry.base_height  = 480;
    info->geometry.max_width    = 5120;
    info->geometry.max_height   = 4096;
    info->geometry.aspect_ratio = 4.0f / 3.0f;
    info->timing.fps            = 60.0;
    info->timing.sample_rate    = 32000.0;
}

RETRO_API void retro_set_environment(retro_environment_t cb)
{
    DolphinLibretro::Frontend::g_environ_cb = cb;
    DolphinLibretro::Environment::SetEnvironmentCallback(cb);

    // SP6: declare core options as soon as the env_cb is available — the
    // only legal time per the libretro spec (before retro_init).
    DolphinLibretro::CoreOptions::EmitCoreOptionsV2(cb);

    // Fetch the frontend log callback if exposed.
    retro_log_callback log_cb{};
    if (cb && cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log_cb) && log_cb.log)
        DolphinLibretro::Environment::SetLogCallback(log_cb.log);
}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb)
{
    DolphinLibretro::Frontend::g_video_refresh_cb = cb;
}

RETRO_API void retro_set_audio_sample(retro_audio_sample_t cb)
{
    DolphinLibretro::Frontend::g_audio_sample_cb = cb;
}

RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
    DolphinLibretro::Frontend::g_audio_batch_cb = cb;
}

RETRO_API void retro_set_input_poll(retro_input_poll_t cb)
{
    DolphinLibretro::Frontend::g_input_poll_cb = cb;
}

RETRO_API void retro_set_input_state(retro_input_state_t cb)
{
    DolphinLibretro::Frontend::g_input_state_cb = cb;
}

RETRO_API void retro_init(void)
{
    // Set Dolphin's User directory. SP2 uses a fixed /tmp path; SP3 should
    // route this through RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY so the host
    // can put it under its own data root.
    UICommon::SetUserDirectory("/tmp/dolphin-libretro-user");
    UICommon::Init();

    // Optionally route Dolphin's own LogManager output to stderr at INFO level
    // with every log type enabled — gated behind RETRONEST_DOLPHIN_LOG so it's
    // available for debugging video/audio/boot issues without spamming normal
    // runs. RetroNest captures the core's stderr.
    if (std::getenv("RETRONEST_DOLPHIN_LOG"))
    {
        Config::SetBaseOrCurrent(Common::Log::LOGGER_VERBOSITY, Common::Log::LogLevel::LINFO);
        if (auto* lm = Common::Log::LogManager::GetInstance())
        {
            lm->EnableListener(Common::Log::LogListener::CONSOLE_LISTENER, true);
            for (int t = 0; t < static_cast<int>(Common::Log::LogType::NUMBER_OF_LOGS); ++t)
                lm->SetEnable(static_cast<Common::Log::LogType>(t), true);
        }
    }

    DolphinLibretro::Environment::Log(RETRO_LOG_INFO, "[Frontend] UICommon::Init done");

    // Bind the GameCube pads to our Libretro virtual devices.  Must run before
    // retro_load_game's UICommon::InitControllers (which loads the pad config);
    // Input::Install() then adds the devices and the resulting RefreshDevices
    // re-resolves the bindings.  Without this the pad falls back to keyboard keys.
    DolphinLibretro::Input::WriteDefaultGCPadProfile();
    DolphinLibretro::Input::WriteDefaultWiimoteProfile();
    DolphinLibretro::Environment::Log(RETRO_LOG_INFO, "[Frontend] wrote default GCPad + Wiimote profiles");

    s_emu_thread = std::make_unique<DolphinLibretro::EmuThread>();
}

RETRO_API void retro_deinit(void)
{
    s_emu_thread.reset();
    UICommon::Shutdown();
}

RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device)
{
    (void)port; (void)device;
}

RETRO_API void retro_reset(void)
{
    // SP2: not yet supported. SP6/7 cleanup can extend EmuThread with Reset().
}

RETRO_API void retro_run(void)
{
    if (DolphinLibretro::Frontend::g_input_poll_cb)
        DolphinLibretro::Frontend::g_input_poll_cb();

    DolphinLibretro::Input::PollFromFrontend();

    // Dolphin queues host-side jobs (config changes, async results) that need
    // the main thread to dispatch — DolphinNoGUI/PlatformHeadless::MainLoop
    // calls this every iteration. Without it, emulation stalls after boot.
    Core::HostDispatchJobs(Core::System::GetInstance());

    if (s_emu_thread && s_emu_thread->IsRunning())
        s_emu_thread->WaitForFrame();
}

RETRO_API size_t retro_serialize_size(void) { return 0; }
RETRO_API bool   retro_serialize(void*, size_t) { return false; }
RETRO_API bool   retro_unserialize(const void*, size_t) { return false; }

RETRO_API void retro_cheat_reset(void) {}
RETRO_API void retro_cheat_set(unsigned, bool, const char*) {}

RETRO_API bool retro_load_game(const struct retro_game_info* game)
{
    if (!game || !game->path)
    {
        DolphinLibretro::Environment::Log(RETRO_LOG_ERROR,
            "[retro_load_game] no game / no path");
        return false;
    }

    // 1. Get NSView from host + prepare WSI.
    void* nsview = DolphinLibretro::Environment::RequestHostNSView();
    if (!nsview)
        return false;
    if (!DolphinLibretro::Metal::PrepareWindowSystemInfo(nsview, &s_wsi))
        return false;

    // 2. Force Metal as the GFX backend. Without this, Dolphin's default may
    //    be OGL or Software; the WSI we built has MacOS NSView ready for Metal.
    Config::SetCurrent(Config::MAIN_GFX_BACKEND, std::string("Metal"));

    // SP6: read user-tweaked Graphics options and write them into Dolphin's
    // CurrentRun config layer. UICommon::Init (in retro_init) already loaded
    // the Base layer from disk; CurrentRun has higher read priority and is
    // NOT reloaded by BootCore, so these win and are read by the video
    // backend at boot. Read once — options take effect on next launch only.
    const auto resolved = DolphinLibretro::CoreOptions::ReadResolved(
        DolphinLibretro::Environment::GetEnvironmentCallback());
    DolphinLibretro::CoreOptions::Graphics::Apply(resolved.graphics);
    DolphinLibretro::CoreOptions::Audio::Apply(resolved.audio);
    DolphinLibretro::CoreOptions::Core::Apply(resolved.core);

    // 3. Init Dolphin's controllers (needs the WSI for SDL video subsystem etc.).
    //    Mirrors DolphinNoGUI/MainNoGUI.cpp:273. Must come BEFORE BootCore.
    UICommon::InitControllers(s_wsi);
    DolphinLibretro::Environment::Log(RETRO_LOG_INFO, "[Frontend] UICommon::InitControllers done");

    // 3. Install our SoundStream into Dolphin's Core::System (ownership transferred).
    Core::System::GetInstance().SetSoundStream(
        std::make_unique<DolphinLibretro::LibretroAudioStream>());

    // 4. Install our InputBackend with the ControllerInterface (after UICommon::InitControllers
    //    so g_controller_interface is initialized).
    DolphinLibretro::Input::Install(DolphinLibretro::Frontend::g_input_state_cb);

    // 5. Boot via EmuThread.
    return s_emu_thread->StartGame(game->path, s_wsi);
}

RETRO_API bool retro_load_game_special(unsigned, const struct retro_game_info*, size_t)
{
    return false;
}

RETRO_API void retro_unload_game(void)
{
    if (s_emu_thread)
        s_emu_thread->StopGame();
    DolphinLibretro::Input::Uninstall();
    UICommon::ShutdownControllers();
    DolphinLibretro::Metal::ReleaseWindowSystemInfo(&s_wsi);
}

RETRO_API unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }
RETRO_API void*    retro_get_memory_data(unsigned) { return nullptr; }
RETRO_API size_t   retro_get_memory_size(unsigned) { return 0; }

}  // extern "C"
