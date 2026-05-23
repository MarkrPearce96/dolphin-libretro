// SP1: retro_* C ABI entrypoints — skeleton only.
//
// All entrypoints return sane no-op values. Callback function pointers are
// stored but not yet used. retro_load_game returns false ("no game loaded").
// SP2 wires up actual emulation behind these symbols.
//
// libretro requires C linkage; visibility is exported per-function by the
// RETRO_API macro in libretro.h. The CMakeLists hides everything else.

#include "DolphinLibretro/libretro.h"
#include "DolphinLibretro/EmuThread.h"

#include <memory>

namespace {

// Stored callbacks supplied by the libretro frontend.
retro_environment_t        s_environ_cb         = nullptr;
retro_video_refresh_t      s_video_refresh_cb   = nullptr;
retro_audio_sample_t       s_audio_sample_cb    = nullptr;
retro_audio_sample_batch_t s_audio_batch_cb     = nullptr;
retro_input_poll_t         s_input_poll_cb      = nullptr;
retro_input_state_t        s_input_state_cb     = nullptr;

std::unique_ptr<DolphinLibretro::EmuThread> s_emu_thread;

}  // namespace

extern "C" {

RETRO_API unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

RETRO_API void retro_get_system_info(struct retro_system_info* info)
{
    info->library_name     = "Dolphin";
    info->library_version  = "SP1-skeleton";
    info->valid_extensions = "iso|gcm|gcz|ciso|wbfs|rvz|wad|wia|nkit|m3u|dol|elf|tgc";
    info->need_fullpath    = true;
    info->block_extract    = false;
}

RETRO_API void retro_get_system_av_info(struct retro_system_av_info* info)
{
    // Conservative defaults; SP2 reads actual values from Dolphin's video output.
    info->geometry.base_width   = 640;
    info->geometry.base_height  = 480;
    info->geometry.max_width    = 5120;   // 8x EFB scale upper bound
    info->geometry.max_height   = 4096;
    info->geometry.aspect_ratio = 4.0f / 3.0f;
    info->timing.fps            = 60.0;
    info->timing.sample_rate    = 32000.0;  // DSP HLE default; SP7 settles
}

RETRO_API void retro_set_environment(retro_environment_t cb)        { s_environ_cb       = cb; }
RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb)    { s_video_refresh_cb = cb; }
RETRO_API void retro_set_audio_sample(retro_audio_sample_t cb)      { s_audio_sample_cb  = cb; }
RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
                                                                    { s_audio_batch_cb   = cb; }
RETRO_API void retro_set_input_poll(retro_input_poll_t cb)          { s_input_poll_cb    = cb; }
RETRO_API void retro_set_input_state(retro_input_state_t cb)        { s_input_state_cb   = cb; }

RETRO_API void retro_init(void)
{
    s_emu_thread = std::make_unique<DolphinLibretro::EmuThread>();
}

RETRO_API void retro_deinit(void)
{
    s_emu_thread.reset();
}

RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device)
{
    (void)port;
    (void)device;
}

RETRO_API void retro_reset(void)
{
    // SP2: BootManager::Stop + BootCore the same ROM.
}

RETRO_API void retro_run(void)
{
    // SP2: poll input, advance one frame, drain audio + video.
    if (s_input_poll_cb)
        s_input_poll_cb();
}

RETRO_API size_t retro_serialize_size(void)
{
    return 0;  // SP2: State::GetMaxBufferSize for current STATE_VERSION
}

RETRO_API bool retro_serialize(void* data, size_t size)
{
    (void)data; (void)size;
    return false;
}

RETRO_API bool retro_unserialize(const void* data, size_t size)
{
    (void)data; (void)size;
    return false;
}

RETRO_API void retro_cheat_reset(void) {}
RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char* code)
{
    (void)index; (void)enabled; (void)code;
}

RETRO_API bool retro_load_game(const struct retro_game_info* game)
{
    // SP1: skeleton always returns false ("no game loaded").
    // SP2: BootManager::BootCore(BootParameters::GenerateFromFile(game->path)).
    (void)game;
    return false;
}

RETRO_API bool retro_load_game_special(unsigned game_type,
                                       const struct retro_game_info* info,
                                       size_t num_info)
{
    (void)game_type; (void)info; (void)num_info;
    return false;
}

RETRO_API void retro_unload_game(void)
{
    // SP2: Core::Stop + BootManager::Stop + tear down render context.
}

RETRO_API unsigned retro_get_region(void)
{
    return RETRO_REGION_NTSC;
}

RETRO_API void* retro_get_memory_data(unsigned id)
{
    (void)id;
    return nullptr;
}

RETRO_API size_t retro_get_memory_size(unsigned id)
{
    (void)id;
    return 0;
}

}  // extern "C"
