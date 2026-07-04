// SP2: retro_* C ABI entrypoints — real implementation.
//
// retro_load_game boots a GameCube/Wii ROM via BootManager::BootCore.
// retro_run advances Dolphin by ~1 frame and drains one host frame of
// audio from LibretroAudioStream's mixer through g_audio_batch_cb (SP9:
// synchronous on the libretro thread — no drain thread; the stream
// survives boot via the AudioCommon::InitSoundStream pre-install guard).
// Video is pushed into the host's
// CAMetalLayer by Dolphin's Metal backend — no video_refresh_cb call
// is needed since the libretro frontend (RetroNest) composites the
// layer directly.

// Compile-check the retronest_* export signatures against the contract.
#define RETRONEST_LIBRETRO_CORE
#include "DolphinLibretro/retronest-libretro/retronest_libretro.h"

#include "DolphinLibretro/libretro.h"
#include "DolphinLibretro/EmuThread.h"
#include "DolphinLibretro/LibretroEnvironment.h"
#include "CoreOptions.h"
#include "DolphinLibretro/LibretroMetalContext.h"
#include "DolphinLibretro/LibretroAudioStream.h"
#include "DolphinLibretro/LibretroInputSource.h"

#include "Common/Buffer.h"
#include "Common/FileUtil.h"
#include "Common/Config/Config.h"
#include "Common/Event.h"
#include "Common/Logging/Log.h"
#include "Common/Logging/LogManager.h"
#include "Common/WindowSystemInfo.h"
#include "Core/AchievementManager.h"
#include "Core/Config/MainSettings.h"
#include "Core/Core.h"
#include "Core/HW/Memmap.h"
#include "Core/State.h"
#include "Core/System.h"
#include "DiscIO/Volume.h"
#include "DolphinLibretro/MemoryMap.h"
#include "DolphinLibretro/StateHeader.h"
#include "UICommon/UICommon.h"

#include <chrono>
#include <cstdlib>
#include <dlfcn.h>
#include <cstring>
#include <memory>
#include <span>
#include <string>
#include <thread>

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
bool s_memory_map_emitted = false;

// Emit the RA memory map once RAM is allocated. The host consumes it synchronously
// (rc_libretro_memory_init at beginSession), so the PRIMARY emit happens at the end
// of retro_load_game after waiting for Dolphin's async RAM allocation; this retro_run
// call is a safety belt (no-op once emitted). GameCube also works via the
// retro_get_memory_data(SYSTEM_RAM) fallback; Wii REQUIRES this map (MEM1 + MEM2 are
// separate allocations).
void MaybeEmitMemoryMap()
{
    if (s_memory_map_emitted)
        return;

    auto& system = Core::System::GetInstance();
    auto& memory = system.GetMemory();
    if (!memory.GetRAM())
        return;  // RAM not allocated yet — try again next frame

    DolphinLibretro::MemoryMap::RamLayout layout{};
    layout.is_wii    = system.IsWii();
    layout.mem1      = memory.GetRAM();
    layout.mem1_size = memory.GetRamSizeReal();
    layout.mem2      = memory.GetEXRAM();
    layout.mem2_size = memory.GetExRamSizeReal();

    const auto descriptors = DolphinLibretro::MemoryMap::BuildDescriptors(layout);
    if (descriptors.empty())
        return;

    retro_memory_map mmap{};
    mmap.descriptors     = descriptors.data();
    mmap.num_descriptors = static_cast<unsigned>(descriptors.size());

    auto cb = DolphinLibretro::Environment::GetEnvironmentCallback();
    if (cb && cb(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, &mmap))
    {
        s_memory_map_emitted = true;
        DolphinLibretro::Environment::Log(RETRO_LOG_INFO,
            "[MemoryMap] emitted %u descriptor(s), is_wii=%d, mem1=%u bytes, mem2=%u bytes",
            mmap.num_descriptors, layout.is_wii, layout.mem1_size,
            layout.is_wii ? layout.mem2_size : 0u);
    }
}

// Headroom added to measured state size so a later, slightly-larger state still
// fits the frontend-allocated buffer.
constexpr size_t kSerializePadBytes = 1u << 20;  // 1 MiB

// Cached upper bound for retro_serialize_size. Dolphin states are variable-size;
// libretro wants a stable per-session bound. Grows, never shrinks.
size_t s_serialize_size_cache = 0;

// Cold-resume state load is deferred: the host calls retro_unserialize right
// after retro_load_game, while Dolphin is still mid-async-boot. Applying then
// races the booting emu thread (crash), so we stash the state and apply it on
// the first retro_run frame where the core is fully Running.
Common::UniqueBuffer<u8> s_pending_resume;
bool s_has_pending_resume = false;

// Boot gate for retro_serialize_size / retro_serialize. EmuThread::IsRunning()
// flips true as soon as the ASYNC BootCore request is accepted (EmuThread.cpp),
// while Dolphin is still constructing subsystems — and State::SaveToBuffer
// deliberately skips Dolphin's own validity gates (State.h), so serializing
// mid-boot walks half-initialized state and crashes. Require Core to have
// actually reached Running-or-Paused: Core::IsRunning (Core.h) is exactly
// that predicate, and it's the same readiness test retro_unserialize uses.
// Paused MUST stay serializable — RetroNest pauses via retronest_set_paused
// before Save & Exit, and savestates while paused must keep working.
bool IsSerializableNow()
{
    return s_emu_thread && s_emu_thread->IsRunning() &&
           Core::IsRunning(Core::System::GetInstance());
}

// Run `fn` on the CPU thread (safe state for save/load) and block until it
// completes. Core::RunOnCPUThread queues onto the CPU thread when called from
// another thread (retro_run's thread); the Event makes completion explicit so
// we can safely read by-reference captures after this returns.
template <typename Fn>
void RunStateOpAndWait(Fn&& fn)
{
    Common::Event done;
    Core::RunOnCPUThread(Core::System::GetInstance(), [&] {
        fn();
        done.Set();
    });
    done.Wait();
}

// Apply a deferred cold-resume state once the core has finished booting. Called
// each retro_run frame; no-ops until Core::IsRunning so the load lands on the
// CPU thread at a safe point instead of racing boot.
void MaybeApplyPendingResume()
{
    if (!s_has_pending_resume)
        return;
    if (!Core::IsRunning(Core::System::GetInstance()))
        return;  // still booting — try again next frame

    bool ok = false;
    RunStateOpAndWait([&] {
        std::span<u8> span(s_pending_resume.data(), s_pending_resume.size());
        ok = State::LoadFromBuffer(Core::System::GetInstance(), span);
    });
    DolphinLibretro::Environment::Log(ok ? RETRO_LOG_INFO : RETRO_LOG_ERROR,
        "[Savestate] deferred resume load %s (%zu bytes)",
        ok ? "applied" : "FAILED", s_pending_resume.size());
    s_has_pending_resume = false;
    s_pending_resume.reset();
}

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
    // Must match LibretroAudioStream's mixer rate — the host opens its audio
    // sink at this rate (previously said 32000 while the mixer ran at 48000).
    info->timing.sample_rate    = static_cast<double>(DolphinLibretro::LibretroAudioStream::kSampleRate);
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
    // SP8: root Dolphin's User dir under the host-provided save directory so GC
    // memcards / Wii NAND / SD images / savestates persist across runs. The host
    // maps RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY to a stable per-emulator/per-system
    // dir. env_cb is available here (retro_set_environment runs before retro_init).
    // Fall back to the old /tmp path so boot never depends on the host answering.
    std::string user_dir = "/tmp/dolphin-libretro-user";
    if (auto cb = DolphinLibretro::Environment::GetEnvironmentCallback())
    {
        const char* save_dir = nullptr;
        if (cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir) && save_dir && *save_dir)
            user_dir = std::string(save_dir) + "/dolphin-libretro-user";
    }
    DolphinLibretro::Environment::Log(RETRO_LOG_INFO, "[Frontend] user dir: %s", user_dir.c_str());
    UICommon::SetUserDirectory(user_dir);
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
    {
        MaybeEmitMemoryMap();
        MaybeApplyPendingResume();
        s_emu_thread->WaitForFrame();

        // SP9: drain one host frame of audio, synchronously on this thread —
        // the only thread the libretro contract allows audio callbacks from.
        // The installed stream is always ours during a session (retro_load_game
        // installs it; the InitSoundStream guard preserves it through boot).
        if (auto* stream = dynamic_cast<DolphinLibretro::LibretroAudioStream*>(
                Core::System::GetInstance().GetSoundStream()))
        {
            stream->DrainToFrontend(DolphinLibretro::Frontend::g_audio_batch_cb);
        }
    }
}

// RetroNest-private (resolved by the host via dlsym; mirrors pcsx2-libretro).
// Dolphin runs the game on its own CPU/GPU threads, so the host merely halting
// its retro_run loop (which pauses synchronous cores) does NOT stop Dolphin —
// the in-game menu would leave the game running. The host calls this to actually
// pause/resume Dolphin's emulation (Core::SetState Paused/Running via EmuThread).
RETRO_API void retronest_set_paused(bool paused)
{
    if (s_emu_thread && s_emu_thread->IsRunning())
        s_emu_thread->SetPaused(paused);
}

// RetroNest-private (resolved by the host via dlsym; mirrors pcsx2-libretro).
// Fast-forward: like pause, the host can't speed Dolphin up by pacing its own
// retro_run loop faster — Dolphin self-throttles on its CPU/GPU threads. So the
// host calls this to toggle Dolphin's throttler directly, exactly as Dolphin's
// own Fast Forward hotkey does (Core::SetIsThrottlerTempDisabled).
RETRO_API void retronest_set_fast_forward(bool fast)
{
    if (s_emu_thread && s_emu_thread->IsRunning())
        Core::SetIsThrottlerTempDisabled(fast);
}

RETRO_API size_t retro_serialize_size(void)
{
    if (!IsSerializableNow())
        return 0;

    Common::UniqueBuffer<u8> scratch;
    size_t measured = 0;
    RunStateOpAndWait([&] {
        measured = State::SaveToBuffer(Core::System::GetInstance(), scratch);
    });

    if (measured == 0)
        return s_serialize_size_cache;  // measure failed; keep any prior bound

    // Pad so a later, larger state still fits the frontend-allocated buffer,
    // plus room for the version header retro_serialize prepends.
    const size_t padded = measured + measured / 4 + kSerializePadBytes  // +25% +1 MiB
                          + DolphinLibretro::StateHeader::kHeaderSize;
    if (padded > s_serialize_size_cache)
        s_serialize_size_cache = padded;

    DolphinLibretro::Environment::Log(RETRO_LOG_DEBUG,
        "[Savestate] size measured=%zu reported=%zu", measured, s_serialize_size_cache);
    return s_serialize_size_cache;
}

RETRO_API bool retro_serialize(void* data, size_t size)
{
    if (!data || !IsSerializableNow())
        return false;

    // Buffer layout: [16-byte StateHeader][DoState payload] — see StateHeader.h.
    constexpr size_t kHdr = DolphinLibretro::StateHeader::kHeaderSize;
    if (size < kHdr)
        return false;

    bool ok = false;
    RunStateOpAndWait([&] {
        Common::UniqueBuffer<u8> buffer(size - kHdr);
        const size_t written = State::SaveToBuffer(Core::System::GetInstance(), buffer);
        if (written != 0 && written + kHdr <= size)
        {
            u8* out = static_cast<u8*>(data);
            DolphinLibretro::StateHeader::Pack(out, State::GetSaveStateVersion());
            std::memcpy(out + kHdr, buffer.data(), written);
            ok = true;
            if (written + kHdr + kSerializePadBytes > s_serialize_size_cache)
                s_serialize_size_cache = written + kHdr + kSerializePadBytes;  // keep bound honest
        }
        else
        {
            DolphinLibretro::Environment::Log(RETRO_LOG_ERROR,
                "[Savestate] serialize: state %zu (+%zu header) bytes > buffer %zu",
                written, kHdr, size);
            // Grow the reported bound so a re-query of retro_serialize_size fits
            // the actual state next time (frontends that re-query before each save
            // then self-heal). written==0 means measure failed — leave cache as-is.
            if (written != 0 && written + kHdr + kSerializePadBytes > s_serialize_size_cache)
                s_serialize_size_cache = written + kHdr + kSerializePadBytes;
        }
    });
    return ok;
}

RETRO_API bool retro_unserialize(const void* data, size_t size)
{
    // Note: RetroAchievements hardcore-mode load-blocking is enforced host-side by
    // RetroNest's shared rcheevos runtime (this libretro build does not use Dolphin's
    // native AchievementManager), so retro_unserialize intentionally does not gate on it.
    if (!data || !s_emu_thread || !s_emu_thread->IsRunning())
        return false;

    // Version header check FIRST — before the deferred-resume stash — so both
    // the immediate and deferred paths see the same validated, header-stripped
    // payload (MaybeApplyPendingResume then never needs to know about headers).
    // A version-mismatched blob is rejected here without touching core state.
    const u8* payload = static_cast<const u8*>(data);
    size_t payload_size = size;
    {
        namespace SH = DolphinLibretro::StateHeader;
        SH::Header hdr{};
        switch (SH::Parse(payload, payload_size, State::GetSaveStateVersion(), &hdr))
        {
        case SH::ParseResult::Ok:
            payload += SH::kHeaderSize;
            payload_size -= SH::kHeaderSize;
            break;
        case SH::ParseResult::Legacy:
            // Headerless blob from an older core build (e.g. a persisted .resume
            // file) — grandfathered: load the whole buffer exactly as before.
            DolphinLibretro::Environment::Log(RETRO_LOG_WARN,
                "[Savestate] legacy headerless state (%zu bytes) — loading as-is", size);
            break;
        case SH::ParseResult::Truncated:
            DolphinLibretro::Environment::Log(RETRO_LOG_ERROR,
                "[Savestate] rejected: header magic but only %zu bytes — corrupt", size);
            return false;
        case SH::ParseResult::BadHeaderVersion:
            DolphinLibretro::Environment::Log(RETRO_LOG_ERROR,
                "[Savestate] rejected: header version %u, this core supports %u",
                hdr.header_version, SH::kHeaderVersion);
            return false;
        case SH::ParseResult::StateVersionMismatch:
            DolphinLibretro::Environment::Log(RETRO_LOG_ERROR,
                "[Savestate] rejected: state version %u != current %u — "
                "savestate is from a different core build",
                hdr.state_version, State::GetSaveStateVersion());
            return false;
        }
    }

    // Cold resume: the host calls this right after retro_load_game, while Dolphin
    // is still mid-async-boot. Loading then races the booting emu thread and
    // crashes, so stash the state and apply it on the first fully-Running
    // retro_run frame (MaybeApplyPendingResume). In-session loads (core already
    // Running) fall through and apply immediately.
    if (!Core::IsRunning(Core::System::GetInstance()))
    {
        s_pending_resume.reset(payload_size);
        std::memcpy(s_pending_resume.data(), payload, payload_size);
        s_has_pending_resume = true;
        DolphinLibretro::Environment::Log(RETRO_LOG_INFO,
            "[Savestate] resume deferred until boot completes (%zu bytes)", payload_size);
        return true;
    }

    bool ok = false;
    RunStateOpAndWait([&] {
        // LoadFromBuffer takes a mutable span (PointerWrap in Read mode advances a
        // copied pointer; it does not write to the buffer). Cast away const for the API.
        std::span<u8> span(const_cast<u8*>(payload), payload_size);
        ok = State::LoadFromBuffer(Core::System::GetInstance(), span);
    });
    if (!ok)
        DolphinLibretro::Environment::Log(RETRO_LOG_ERROR, "[Savestate] unserialize failed");
    return ok;
}

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

    // SP9: point Sys at dolphin_libretro_resources/Sys beside the dylib when
    // present (the CI release zip and tools/deploy.sh both ship that layout).
    // Without this, macOS resolves Sys inside the HOST APP's bundle
    // (Contents/Resources/Sys) — which only exists on machines where
    // deploy.sh copied it there, so GitHub-installed cores had no Sys at all.
    // Must run before the first File::GetSysDirectory() call (cached), and
    // only once per process (second retro_load_game: the cache already stuck).
    {
        static bool s_sys_dir_checked = false;
        if (!s_sys_dir_checked)
        {
            s_sys_dir_checked = true;
            Dl_info dl_info{};
            if (dladdr(reinterpret_cast<const void*>(&retro_load_game), &dl_info) &&
                dl_info.dli_fname)
            {
                std::string dylib_dir(dl_info.dli_fname);
                const auto slash = dylib_dir.find_last_of('/');
                if (slash != std::string::npos)
                    dylib_dir.resize(slash);
                const std::string sys = dylib_dir + "/dolphin_libretro_resources/Sys";
                if (File::IsDirectory(sys))
                {
                    File::SetSysDirectory(sys);
                    DolphinLibretro::Environment::Log(RETRO_LOG_INFO,
                        "[Frontend] Sys directory -> %s", sys.c_str());
                }
                else
                {
                    DolphinLibretro::Environment::Log(RETRO_LOG_INFO,
                        "[Frontend] no %s — falling back to the host app bundle Sys",
                        sys.c_str());
                }
            }
        }
    }

    // Compute the RetroAchievements hash + game serial via DiscIO (handles RVZ
    // and every other Dolphin disc format) and hand them to the host BEFORE it
    // identifies the game. The host calls rc_client_begin_load_game with this
    // hash because rcheevos' own path-based identify can't read compressed RVZ;
    // it also lazily stores the serial. Static storage keeps the const char*s
    // valid for the synchronous env_cb call.
    {
        static std::string s_ra_hash;
        static std::string s_serial;
        s_ra_hash.clear();
        s_serial.clear();
        // Only hash/read when the disc actually opens. This guards CalculateHash,
        // which otherwise dereferences a null volume on an unreadable/corrupt file.
        // Safe to call here: retro_load_game runs before StartGame, so no emu-thread
        // rcheevos work can race CalculateHash's process-global filereader registration.
        if (auto volume = DiscIO::CreateVolume(game->path))
        {
            s_serial  = volume->GetGameID();
            s_ra_hash = AchievementManager::CalculateHash(game->path);
            if (s_ra_hash == "0")
                s_ra_hash.clear();
        }

        DolphinLibretro::Environment::RetroNestGameIdentity identity{
            s_ra_hash.c_str(), s_serial.c_str()};
        if (auto cb = DolphinLibretro::Environment::GetEnvironmentCallback())
            cb(DolphinLibretro::Environment::RETRONEST_SET_GAME_IDENTITY, &identity);
        DolphinLibretro::Environment::Log(RETRO_LOG_INFO,
            "[GameIdentity] hash=%s serial=%s",
            s_ra_hash.empty() ? "(none)" : s_ra_hash.c_str(),
            s_serial.empty() ? "(none)" : s_serial.c_str());
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
    if (!s_emu_thread->StartGame(game->path, s_wsi))
        return false;

    // 6. Emit the RA memory map BEFORE returning. The RetroNest host calls
    //    rc_libretro_memory_init synchronously at beginSession — right after
    //    retro_load_game returns and BEFORE the first retro_run — so emitting
    //    only from retro_run is too late, and Wii RA (which needs the separate
    //    MEM1+MEM2 descriptors) would silently fall back to the GameCube-only
    //    SYSTEM_RAM path. BootCore is async, so wait (bounded, ~5s) for Dolphin
    //    to allocate RAM, then emit. The retro_run call is now a safety belt.
    for (int i = 0; i < 500 && !Core::System::GetInstance().GetMemory().GetRAM(); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    MaybeEmitMemoryMap();
    return true;
}

RETRO_API bool retro_load_game_special(unsigned, const struct retro_game_info*, size_t)
{
    return false;
}

RETRO_API void retro_unload_game(void)
{
    if (s_emu_thread)
        s_emu_thread->StopGame();
    s_memory_map_emitted = false;
    s_serialize_size_cache = 0;
    s_has_pending_resume = false;
    s_pending_resume.reset();
    DolphinLibretro::Input::Uninstall();
    UICommon::ShutdownControllers();
    DolphinLibretro::Metal::ReleaseWindowSystemInfo(&s_wsi);
}

RETRO_API unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }

RETRO_API void* retro_get_memory_data(unsigned id)
{
    if (id != RETRO_MEMORY_SYSTEM_RAM)
        return nullptr;
    if (!s_emu_thread || !s_emu_thread->IsRunning())
        return nullptr;
    return Core::System::GetInstance().GetMemory().GetRAM();  // MEM1
}

RETRO_API size_t retro_get_memory_size(unsigned id)
{
    if (id != RETRO_MEMORY_SYSTEM_RAM)
        return 0;
    if (!s_emu_thread || !s_emu_thread->IsRunning())
        return 0;
    return Core::System::GetInstance().GetMemory().GetRamSizeReal();
}

}  // extern "C"
