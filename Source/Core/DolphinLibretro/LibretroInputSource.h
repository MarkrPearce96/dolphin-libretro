// Copyright 2024 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Custom InputBackend that exposes a virtual "Libretro/N" device per port.
// Reads libretro's input_state_cb each retro_run and presents values
// through the standard ControlReference resolution path so Dolphin's
// GameCube pad code finds them via expression strings.

#pragma once

#include "DolphinLibretro/libretro.h"

namespace DolphinLibretro::Input {

// Register the backend with the ControllerInterface.  Stores the callback
// to use for per-frame polling.  Must be called after g_controller_interface
// has been initialized (i.e. inside retro_load_game / retro_run setup).
void Install(retro_input_state_t state_cb);

// Tear down — removes devices and drops the backend.  Idempotent.
void Uninstall();

// Pump libretro input_state_cb for every (port x button/axis) the devices
// expose; called once per retro_run before Dolphin reads control state.
void PollFromFrontend();

// (Re)write GCPadNew.ini so every GameCube pad reads from its matching
// Libretro/0/N virtual device.  Must run before UICommon::InitControllers
// (which loads the pad config).  Without it, Dolphin's GCPad::LoadDefaults
// binds the pad to keyboard keys and a real gamepad does nothing.
void WriteDefaultGCPadProfile();

// (Re)write WiimoteNew.ini so every Wiimote is emulated with a Classic
// Controller extension bound to its Libretro/0/N device.  Same timing +
// rationale as WriteDefaultGCPadProfile.  Classic-only: the virtual device
// has no IR/motion, so the bare Wii Remote pointer/tilt is left unbound.
void WriteDefaultWiimoteProfile();

}  // namespace DolphinLibretro::Input
