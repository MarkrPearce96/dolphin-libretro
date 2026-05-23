# Source/Core/DolphinLibretro/CMakeLists.txt
#
# Builds the libretro core (dolphin_libretro.dylib) as a MODULE library —
# loadable plugin, not a regular shared lib. Only the retro_* C ABI is
# exported; everything else is hidden via CXX_VISIBILITY_PRESET hidden.
#
# Built only when -DENABLE_LIBRETRO=ON. Gated at the parent
# Source/Core/CMakeLists.txt level.

add_library(dolphin_libretro MODULE)

target_sources(dolphin_libretro PRIVATE
    LibretroFrontend.cpp
    HostStubs.cpp
    EmuThread.cpp
)

target_include_directories(dolphin_libretro PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/Source/Core"
)

# Link Dolphin's static libs. core + uicommon transitively pull in
# audiocommon, videocommon, inputcommon, discio, videobackends, common.
target_link_libraries(dolphin_libretro PRIVATE
    core
    uicommon
)

# Libretro naming convention on macOS: dolphin_libretro.dylib (no "lib" prefix,
# .dylib extension). CMake's MODULE libraries default to .so on macOS — override.
set_target_properties(dolphin_libretro PROPERTIES
    PREFIX ""
    OUTPUT_NAME "dolphin_libretro"
)
if(APPLE)
    set_target_properties(dolphin_libretro PROPERTIES SUFFIX ".dylib")
endif()

# Hide all symbols by default — libretro frontends only need the retro_*
# C ABI. The RETRO_API macro in libretro.h adds explicit
# __attribute__((visibility("default"))) per function on non-Windows.
set_target_properties(dolphin_libretro PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
