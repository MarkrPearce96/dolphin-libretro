# tools/CMakeLists.txt
#
# Standalone smoke test that dlopens dolphin_libretro.dylib and exercises
# the retro_* ABI. Doesn't link any Dolphin libs — independent of the
# core's build success.

add_executable(dolphin_libretro_load_test EXCLUDE_FROM_ALL test_load.cpp)
target_include_directories(dolphin_libretro_load_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
# Ensure the dylib exists before the test is built.
add_dependencies(dolphin_libretro_load_test dolphin_libretro)

if(APPLE)
    add_executable(dolphin_libretro_smoke EXCLUDE_FROM_ALL test_harness.mm)
    target_include_directories(dolphin_libretro_smoke PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
    )
    target_compile_options(dolphin_libretro_smoke PRIVATE -fobjc-arc)
    target_link_libraries(dolphin_libretro_smoke PRIVATE
        "-framework Cocoa"
        "-framework Metal"
        "-framework QuartzCore"
    )
    add_dependencies(dolphin_libretro_smoke dolphin_libretro)
endif()
