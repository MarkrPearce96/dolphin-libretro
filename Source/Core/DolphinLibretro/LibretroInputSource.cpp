// Copyright 2024 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinLibretro/LibretroInputSource.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

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

}  // namespace DolphinLibretro::Input
