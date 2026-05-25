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
    LibretroEnvironment.cpp
    LibretroMetalContext.mm
    LibretroAudioStream.cpp
    LibretroInputSource.cpp
    HostStubs.cpp
    EmuThread.cpp
    CoreOptions.cpp
    CoreOptionsGraphics.cpp
    CoreOptionsAudio.cpp
    CoreOptionsCore.cpp
)

# Objective-C++ source needs ARC + AppKit/Metal/QuartzCore frameworks.
if(APPLE)
    set_source_files_properties(LibretroMetalContext.mm PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
    )
endif()

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
    target_link_libraries(dolphin_libretro PRIVATE
        "-framework AppKit"
        "-framework Metal"
        "-framework QuartzCore"
    )
endif()

add_subdirectory(tools)

# SP6: schema-fidelity check between this core's CoreOptions*.cpp and
# RetroNest-Project's DolphinLibretroAdapter::settingsSchema(). The two
# trees live in separate repos, so the host path is a cache var defaulting
# to the user's known location; override with -DRETRONEST_DOLPHIN_LIBRETRO_ADAPTER=
# at configure time (e.g. for CI).
#
# Invoke: cmake --build build-libretro --target check_schema_fidelity
set(RETRONEST_DOLPHIN_LIBRETRO_ADAPTER
    "$ENV{HOME}/Documents/Projects/RetroNest-Project/cpp/src/adapters/libretro/dolphin_libretro_adapter.cpp"
    CACHE PATH "Host-side libretro adapter for schema-fidelity check"
)
add_custom_target(check_schema_fidelity
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_schema_fidelity.py
            --core "${CMAKE_CURRENT_SOURCE_DIR}/CoreOptions*.cpp"
            --host "${RETRONEST_DOLPHIN_LIBRETRO_ADAPTER}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Checking core/host SettingDef schema fidelity"
    VERBATIM
)

# Hide all symbols by default — libretro frontends only need the retro_*
# C ABI. The RETRO_API macro in libretro.h adds explicit
# __attribute__((visibility("default"))) per function on non-Windows.
set_target_properties(dolphin_libretro PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
