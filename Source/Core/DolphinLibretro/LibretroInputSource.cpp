// Copyright 2024 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinLibretro/LibretroInputSource.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

#include "Common/FileUtil.h"
#include "Common/IniFile.h"
#include "InputCommon/ControllerInterface/ControllerInterface.h"
#include "InputCommon/ControllerInterface/CoreDevice.h"

// --------------------------------------------------------------------------
// Constants
// --------------------------------------------------------------------------

static constexpr int LIBRETRO_NUM_PORTS = 4;

// Digital buttons exposed per port (indices match RETRO_DEVICE_ID_JOYPAD_*)
static constexpr unsigned BUTTON_IDS[] = {
    RETRO_DEVICE_ID_JOYPAD_B,
    RETRO_DEVICE_ID_JOYPAD_Y,
    RETRO_DEVICE_ID_JOYPAD_SELECT,
    RETRO_DEVICE_ID_JOYPAD_START,
    RETRO_DEVICE_ID_JOYPAD_UP,
    RETRO_DEVICE_ID_JOYPAD_DOWN,
    RETRO_DEVICE_ID_JOYPAD_LEFT,
    RETRO_DEVICE_ID_JOYPAD_RIGHT,
    RETRO_DEVICE_ID_JOYPAD_A,
    RETRO_DEVICE_ID_JOYPAD_X,
    RETRO_DEVICE_ID_JOYPAD_L,
    RETRO_DEVICE_ID_JOYPAD_R,
    RETRO_DEVICE_ID_JOYPAD_L2,
    RETRO_DEVICE_ID_JOYPAD_R2,
    RETRO_DEVICE_ID_JOYPAD_L3,
    RETRO_DEVICE_ID_JOYPAD_R3,
};
static constexpr const char* BUTTON_NAMES[] = {
    "B", "Y", "Select", "Start",
    "Up", "Down", "Left", "Right",
    "A", "X", "L", "R",
    "L2", "R2", "L3", "R3",
};
static constexpr int NUM_BUTTONS = static_cast<int>(std::size(BUTTON_IDS));

// Analog sticks: left stick X/Y, right stick X/Y
// Each maps to a (index, axis_id) pair
struct AnalogDef {
    unsigned index;
    unsigned axis_id;
    const char* name_plus;   // e.g. "LX+"
    const char* name_minus;  // e.g. "LX-"
};
static constexpr AnalogDef ANALOG_DEFS[] = {
    { RETRO_DEVICE_INDEX_ANALOG_LEFT,  RETRO_DEVICE_ID_ANALOG_X, "LX+", "LX-" },
    { RETRO_DEVICE_INDEX_ANALOG_LEFT,  RETRO_DEVICE_ID_ANALOG_Y, "LY+", "LY-" },
    { RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "RX+", "RX-" },
    { RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "RY+", "RY-" },
};
static constexpr int NUM_ANALOGS = static_cast<int>(std::size(ANALOG_DEFS));

// --------------------------------------------------------------------------
// Shared atomic state — written by PollFromFrontend(), read by GetState()
// --------------------------------------------------------------------------

namespace {

// button_state[port][button_id] — 1 if pressed, 0 if not
std::atomic<int16_t> s_button_state[LIBRETRO_NUM_PORTS][NUM_BUTTONS];

// analog_state[port][analog_idx] — raw signed value from input_state_cb
// Range: -32768..32767
std::atomic<int16_t> s_analog_state[LIBRETRO_NUM_PORTS][NUM_ANALOGS];

retro_input_state_t s_input_state_cb = nullptr;

// --------------------------------------------------------------------------
// LibretroDevice — one per port
// --------------------------------------------------------------------------

class LibretroDevice final : public ciface::Core::Device
{
public:
    explicit LibretroDevice(int port) : m_port(port)
    {
        // Add digital button inputs
        for (int i = 0; i < NUM_BUTTONS; ++i)
            AddInput(new ButtonInput(port, i, BUTTON_NAMES[i]));

        // Add axis inputs — each axis gets a + and - direction
        for (int i = 0; i < NUM_ANALOGS; ++i)
        {
            AddInput(new AxisInput(port, i, ANALOG_DEFS[i].name_plus,  true));
            AddInput(new AxisInput(port, i, ANALOG_DEFS[i].name_minus, false));
        }
    }

    std::string GetName() const override
    {
        return std::to_string(m_port);
    }

    std::string GetSource() const override
    {
        return "Libretro";
    }

    bool IsVirtualDevice() const override { return true; }

    // Sort priority: higher than 0 so this device appears first and becomes
    // the default when Dolphin auto-assigns a controller profile.
    int GetSortPriority() const override { return 10; }

private:
    // -------------------------------------------------------------------
    // ButtonInput — digital button
    // -------------------------------------------------------------------
    class ButtonInput final : public Input
    {
    public:
        ButtonInput(int port, int btn_idx, const char* name)
            : m_port(port), m_btn_idx(btn_idx), m_name(name) {}

        std::string GetName() const override { return m_name; }

        ControlState GetState() const override
        {
            return s_button_state[m_port][m_btn_idx].load(std::memory_order_relaxed) ? 1.0 : 0.0;
        }

    private:
        int m_port;
        int m_btn_idx;
        std::string m_name;
    };

    // -------------------------------------------------------------------
    // AxisInput — one direction of an analog axis
    // -------------------------------------------------------------------
    class AxisInput final : public Input
    {
    public:
        AxisInput(int port, int analog_idx, const char* name, bool positive)
            : m_port(port), m_analog_idx(analog_idx), m_name(name), m_positive(positive) {}

        std::string GetName() const override { return m_name; }

        // Returns [0, 1] for the relevant direction, optionally negative to
        // hint to Dolphin's input-detection that the opposite direction is active.
        ControlState GetState() const override
        {
            const int16_t raw = s_analog_state[m_port][m_analog_idx].load(std::memory_order_relaxed);
            // Normalize to [-1, 1]
            const ControlState normalized = raw / 32767.0;

            if (m_positive)
                return std::max(0.0, normalized);
            else
                return std::max(0.0, -normalized);
        }

    private:
        int m_port;
        int m_analog_idx;
        std::string m_name;
        bool m_positive;
    };

    int m_port;
};

// --------------------------------------------------------------------------
// LibretroInputBackend
// --------------------------------------------------------------------------

class LibretroInputBackend final : public ciface::InputBackend
{
public:
    using ciface::InputBackend::InputBackend;

    void PopulateDevices() override
    {
        for (int port = 0; port < LIBRETRO_NUM_PORTS; ++port)
            GetControllerInterface().AddDevice(std::make_shared<LibretroDevice>(port));
    }
};

// Weak handle so Uninstall() can remove only our devices
bool s_installed = false;

}  // namespace

// --------------------------------------------------------------------------
// Public API
// --------------------------------------------------------------------------

namespace DolphinLibretro::Input {

void Install(retro_input_state_t state_cb)
{
    if (s_installed)
        return;

    s_input_state_cb = state_cb;

    // Zero atomic state
    for (int p = 0; p < LIBRETRO_NUM_PORTS; ++p)
    {
        for (int b = 0; b < NUM_BUTTONS; ++b)
            s_button_state[p][b].store(0, std::memory_order_relaxed);
        for (int a = 0; a < NUM_ANALOGS; ++a)
            s_analog_state[p][a].store(0, std::memory_order_relaxed);
    }

    g_controller_interface.AddBackend(
        std::make_unique<LibretroInputBackend>(&g_controller_interface));

    // Trigger device enumeration so PopulateDevices() is called and the
    // LibretroDevice instances appear in the device list.
    g_controller_interface.RefreshDevices();

    s_installed = true;
}

void Uninstall()
{
    if (!s_installed)
        return;

    s_input_state_cb = nullptr;

    // Remove all Libretro-sourced devices.  The backend itself will be
    // destroyed when g_controller_interface.Shutdown() clears m_input_backends.
    g_controller_interface.RemoveDevice(
        [](const ciface::Core::Device* dev) { return dev->GetSource() == "Libretro"; });

    s_installed = false;
}

void PollFromFrontend()
{
    if (!s_input_state_cb || !s_installed)
        return;

    for (int port = 0; port < LIBRETRO_NUM_PORTS; ++port)
    {
        // Digital buttons
        for (int i = 0; i < NUM_BUTTONS; ++i)
        {
            const int16_t val = s_input_state_cb(
                static_cast<unsigned>(port),
                RETRO_DEVICE_JOYPAD,
                0,
                BUTTON_IDS[i]);
            s_button_state[port][i].store(val, std::memory_order_relaxed);
        }

        // Analog axes
        for (int i = 0; i < NUM_ANALOGS; ++i)
        {
            const int16_t val = s_input_state_cb(
                static_cast<unsigned>(port),
                RETRO_DEVICE_ANALOG,
                ANALOG_DEFS[i].index,
                ANALOG_DEFS[i].axis_id);
            s_analog_state[port][i].store(val, std::memory_order_relaxed);
        }
    }
}

void WriteDefaultGCPadProfile()
{
    // The libretro architecture fixes the GameCube-pad <-> RetroPad binding:
    // RetroNest (the host) owns user remapping on the physical-input <-> RetroPad
    // -slot side, and the core's GC pad always reads the RetroPad inputs exposed
    // by our Libretro/0/N virtual device.  So we (re)write this profile every boot
    // rather than seeding a one-time default — it isn't user-editable here, and the
    // /tmp user dir is recreated each session anyway.
    //
    // GC <-> RetroPad convention (must match RetroNest's DolphinLibretroAdapter
    // BindingDefs): A<->B, B<->A, X<->Y, Y<->X; Z<->R3; Start<->Start; D-Pad 1:1;
    // Main Stick <-> left analog, C-Stick <-> right analog; L(digital)<->L,
    // L-Analog<->L2, R(digital)<->R, R-Analog<->R2.  RetroPad analog polarity:
    // up = -Y, down = +Y, left = -X, right = +X.
    const std::string config_dir = File::GetUserPath(D_CONFIG_IDX);
    File::CreateFullPath(config_dir);
    const std::string ini_path = config_dir + "GCPadNew.ini";

    Common::IniFile ini;
    ini.Load(ini_path);  // keep any pre-existing unrelated sections

    for (int port = 0; port < LIBRETRO_NUM_PORTS; ++port)
    {
        const std::string device = "Libretro/0/" + std::to_string(port);
        auto* s = ini.GetOrCreateSection("GCPad" + std::to_string(port + 1));

        // Section::Set has a templated overload that, given a string *literal*,
        // decays the const char* to bool and writes "True".  Route every value
        // through this lambda so the std::string arg selects the string overload.
        const auto set = [s](const char* key, const std::string& expr) { s->Set(key, expr); };

        set("Device", device);

        // No face swap: RetroNest seeds RetroPad slot A=south, B=east, X=west,
        // Y=north (controls.ini), so GC button <- same-letter RetroPad input puts
        // south(cross)->GC A, etc.  GC Z uses RetroPad Select because that is the
        // only spare slot RetroNest seeds a default physical binding for (Back).
        set("Buttons/A", "`A`");
        set("Buttons/B", "`B`");
        set("Buttons/X", "`X`");
        set("Buttons/Y", "`Y`");
        set("Buttons/Z", "`Select`");
        set("Buttons/Start", "`Start`");

        set("Main Stick/Up", "`LY-`");
        set("Main Stick/Down", "`LY+`");
        set("Main Stick/Left", "`LX-`");
        set("Main Stick/Right", "`LX+`");

        set("C-Stick/Up", "`RY-`");
        set("C-Stick/Down", "`RY+`");
        set("C-Stick/Left", "`RX-`");
        set("C-Stick/Right", "`RX+`");

        // L/R analog reuse the digital shoulder inputs (full press when held);
        // the virtual device exposes L2/R2 only as digital, and RetroNest does
        // not seed default bindings for them.
        set("Triggers/L", "`L`");
        set("Triggers/R", "`R`");
        set("Triggers/L-Analog", "`L`");
        set("Triggers/R-Analog", "`R`");

        set("D-Pad/Up", "`Up`");
        set("D-Pad/Down", "`Down`");
        set("D-Pad/Left", "`Left`");
        set("D-Pad/Right", "`Right`");
    }

    ini.Save(ini_path);
}

void WriteDefaultWiimoteProfile()
{
    // Emulated Wiimote + Classic Controller extension bound to Libretro/0/N,
    // mirroring WriteDefaultGCPadProfile.  RetroPad slot <-> Classic control
    // (must match RetroNest's feed + wiiClassicBindings): ZL<-L2, ZR<-Select,
    // -<-R2, +<-Start, Home<-R3; L/R analog<-L/R; sticks<-LX/LY,RX/RY; faces +
    // D-Pad straight through.  Only the Classic extension is bound — the virtual
    // device exposes no IR pointer or accelerometer for the bare Wii Remote.
    const std::string config_dir = File::GetUserPath(D_CONFIG_IDX);
    File::CreateFullPath(config_dir);
    const std::string ini_path = config_dir + "WiimoteNew.ini";

    Common::IniFile ini;
    ini.Load(ini_path);

    for (int port = 0; port < LIBRETRO_NUM_PORTS; ++port)
    {
        const std::string device = "Libretro/0/" + std::to_string(port);
        auto* s = ini.GetOrCreateSection("Wiimote" + std::to_string(port + 1));
        const auto set = [s](const char* key, const std::string& expr) { s->Set(key, expr); };

        set("Source", "1");  // 1 = Emulated
        set("Device", device);
        set("Extension", "Classic");

        set("Classic/Buttons/A", "`A`");
        set("Classic/Buttons/B", "`B`");
        set("Classic/Buttons/X", "`X`");
        set("Classic/Buttons/Y", "`Y`");
        set("Classic/Buttons/ZL", "`L2`");
        set("Classic/Buttons/ZR", "`Select`");
        set("Classic/Buttons/-", "`R2`");
        set("Classic/Buttons/+", "`Start`");
        set("Classic/Buttons/Home", "`R3`");

        set("Classic/D-Pad/Up", "`Up`");
        set("Classic/D-Pad/Down", "`Down`");
        set("Classic/D-Pad/Left", "`Left`");
        set("Classic/D-Pad/Right", "`Right`");

        set("Classic/Left Stick/Up", "`LY-`");
        set("Classic/Left Stick/Down", "`LY+`");
        set("Classic/Left Stick/Left", "`LX-`");
        set("Classic/Left Stick/Right", "`LX+`");

        set("Classic/Right Stick/Up", "`RY-`");
        set("Classic/Right Stick/Down", "`RY+`");
        set("Classic/Right Stick/Left", "`RX-`");
        set("Classic/Right Stick/Right", "`RX+`");

        set("Classic/Triggers/L", "`L`");
        set("Classic/Triggers/R", "`R`");
        set("Classic/Triggers/L-Analog", "`L`");
        set("Classic/Triggers/R-Analog", "`R`");
    }

    ini.Save(ini_path);
}

}  // namespace DolphinLibretro::Input
