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
