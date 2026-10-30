cmake_minimum_required(VERSION 3.20)
project(MegaManX5Recomp C CXX)
include(CTest)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# The co-op title exposes both controller seats and the launcher Host/Join
# flow on a fresh configure, just like X6's co-op branch.
option(PSX_NETPLAY "Build the shared two-player netplay stack" ON)

# Keep codegen, runtime and selective release PGO on one shared framework.
# Direct title builds retain the established submodule default.
if(DEFINED PSXRECOMP_ROOT)
    set(PSXRECOMP_V4_ROOT "${PSXRECOMP_ROOT}")
else()
    set(PSXRECOMP_V4_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/psxrecomp-v4" CACHE PATH
        "Path to psxrecomp-v4")
endif()

# Dinorex's transparent fire is expensive with one GL draw per primitive.
# Use the existing painter-ordered batch tier for this title; explicit LLE
# configurations remain available as the renderer reference.
set(PSX_GL_BATCH_IMPL "HLE" CACHE STRING "OpenGL textured batching: LLE or HLE")
include("${PSXRECOMP_V4_ROOT}/runtime/runtime.cmake")
option(MMX5_NATIVE_IDLE_WAIT "Select the audited native PsyQ frame wait" ON)
set(MMX5_NATIVE_IDLE_WAIT_SITES "")
if(MMX5_NATIVE_IDLE_WAIT)
    if(NOT COMMAND psxrecomp_native_idle_wait)
        message(FATAL_ERROR "MMX5_NATIVE_IDLE_WAIT needs native wait-policy support")
    endif()
    set(MMX5_NATIVE_IDLE_WAIT_SITES 0x80062FE8 0x80063030)
endif()


option(MMX5_COOP_TEST_CHEATS "Enable replicated X5 qualification cheats (private debug builds only)" OFF)
if(MMX5_COOP_TEST_CHEATS AND NOT PSX_DEBUG_TOOLS)
    message(FATAL_ERROR "MMX5_COOP_TEST_CHEATS requires PSX_DEBUG_TOOLS")
endif()
if(MMX5_COOP_TEST_CHEATS)
    set_property(SOURCE src/mods/mmx5_coop_plugin.c APPEND PROPERTY
        COMPILE_DEFINITIONS MMX5_COOP_TEST_CHEATS=1)
endif()

# The recompiler emits the game body as SHARDS (SLUS_013.34_full_00.c ...) --
# a single SLUS_013.34_full.c has not been produced for some time. This file
# only knew the monolithic name, so a fresh regen built nothing and the
# require-generated guard reported the game C as MISSING even though 31 shards
# were sitting in generated/. Prefer shards, fall back to the old single file.
# Same shape MegaManX6Recomp already uses.
file(GLOB GAME_GENERATED_FULL_SHARDS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/generated/SLUS_013.34_full_*.c")
if(GAME_GENERATED_FULL_SHARDS)
    set(GAME_GENERATED_FULL ${GAME_GENERATED_FULL_SHARDS})
    list(POP_FRONT GAME_GENERATED_FULL GAME_GENERATED_PRIMARY)
    set(GAME_GENERATED_EXTRAS ${GAME_GENERATED_FULL})
    set(GAME_GENERATED_FULL ${GAME_GENERATED_PRIMARY})
else()
    set(GAME_GENERATED_FULL
        "${CMAKE_CURRENT_SOURCE_DIR}/generated/SLUS_013.34_full.c")
endif()
set(GAME_GENERATED_DISPATCH
    "${CMAKE_CURRENT_SOURCE_DIR}/generated/SLUS_013.34_dispatch.c")

# Trusted, statically linked activation plugins. The package manifests select
# them by stable id; mod packages never carry or load native host code.
file(GLOB MMX5_MOD_PLUGIN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/*.cpp")
list(REMOVE_ITEM MMX5_MOD_PLUGIN_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/mmx5_resident.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/mmx5_resident_reference.c")

# Delay-sync peers must share the title and framework simulation contract.
file(GLOB MMX5_COOP_CONTRACT_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/mmx5_coop_*"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/mmx5_diagnostic*"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/mmx5_alia*"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/mmx5_widescreen*")
list(APPEND MMX5_COOP_CONTRACT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/game.toml"
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/preloaded/packages/mmx5.enhancement.skip-alia/1.0.0/manifest.toml"
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/preloaded/packages/mmx5.cheat.ultimate-armor/1.0.0/manifest.toml"
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/preloaded/packages/mmx5.cheat.black-zero/1.0.0/manifest.toml"
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/preloaded/packages/mmx5.coop/0.0.1/manifest.toml")
foreach(MMX5_FRAMEWORK_FILE IN ITEMS runtime/src/mod_netplay.c runtime/include/mod_netplay.h
    runtime/src/mod_runtime.cpp runtime/src/mod_texture_banks.c runtime/include/mod_plugins.h
    runtime/src/psx_cycles.c runtime/src/gpu.c runtime/src/render_pass.c
    runtime/include/psx_cycle_freeze.h runtime/include/mod_capcom_bg2d.h
    runtime/include/mod_visible_placements.h runtime/src/main.cpp runtime/include/mod_runtime.h)
    list(APPEND MMX5_COOP_CONTRACT_FILES "${PSXRECOMP_V4_ROOT}/${MMX5_FRAMEWORK_FILE}")
endforeach()
set(MMX5_COOP_CONTRACT "mmx5-coop-delay-v1\n")
string(APPEND MMX5_COOP_CONTRACT "qualification-cheats=${MMX5_COOP_TEST_CHEATS}\n")
foreach(MMX5_COOP_FILE IN LISTS MMX5_COOP_CONTRACT_FILES)
    file(SHA256 "${MMX5_COOP_FILE}" MMX5_COOP_FILE_HASH)
    string(APPEND MMX5_COOP_CONTRACT "${MMX5_COOP_FILE_HASH}\n")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${MMX5_COOP_FILE}")
endforeach()
string(SHA256 MMX5_COOP_CONTRACT_HASH "${MMX5_COOP_CONTRACT}")
string(SUBSTRING "${MMX5_COOP_CONTRACT_HASH}" 0 40 MMX5_COOP_CONTRACT_SHORT_HASH)
set_property(SOURCE src/mods/mmx5_coop_plugin.c APPEND PROPERTY COMPILE_DEFINITIONS
    "MMX5_COOP_NETPLAY_COMPATIBILITY=\"mmx5-coop-delay-v1:${MMX5_COOP_CONTRACT_SHORT_HASH}\"")
set_property(SOURCE src/mods/mmx5_diagnostics.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "MMX5_DIAGNOSTIC_BUILD_ID=\"mmx5-coop-delay-v1:${MMX5_COOP_CONTRACT_SHORT_HASH}\"")
# Match the shared runtime's game-version selection, including local dev builds.
set(MMX5_DIAGNOSTIC_RELEASE_VERSION "dev")
if(DEFINED PSX_GAME_VERSION AND NOT PSX_GAME_VERSION STREQUAL "")
    set(MMX5_DIAGNOSTIC_RELEASE_VERSION "${PSX_GAME_VERSION}")
endif()
set_property(SOURCE src/mods/mmx5_diagnostics.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "MMX5_DIAGNOSTIC_RELEASE_VERSION=\"${MMX5_DIAGNOSTIC_RELEASE_VERSION}\"")
set_property(SOURCE src/mods/mmx5_startup_log.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "MMX5_STARTUP_BUILD_ID=\"mmx5-coop-delay-v1:${MMX5_COOP_CONTRACT_SHORT_HASH}\"")

# X5's game-owned enhancement packages.
#
# The FRAMEWORK stages them: PRELOADED_MODS_DIR below hands the directory to
# psxrecomp_add_runtime_target(), which wipes <exe-dir>/mods/bundled and copies
# both its own mods/builtin/packages and these packages into it, plus
# mods/preloaded/README.md -> mods/README.md. This repo must NOT copy the tree
# itself. It used to, with a hand-written POST_BUILD copy_directory into
# <exe-dir>/mods, and because that named the destination as a STRING, framework
# commit 4cc04be3's rename of the staged catalog from mods/packages to
# mods/bundled broke it silently -- nothing a configure, compile or link step
# can see. The two mmx5.* packages went on landing in mods/packages, which
# neither the launcher nor the release packager's Add-ModCatalog reads.
# Bead beads-eio.3.101.
set(MMX5_PRELOADED_MODS "${CMAKE_CURRENT_SOURCE_DIR}/mods/preloaded")

psxrecomp_v4_add_runtime_target(psx-runtime
    NATIVE_IDLE_WAIT_SITES ${MMX5_NATIVE_IDLE_WAIT_SITES}
    GAME_GENERATED_FULL_C "${GAME_GENERATED_FULL}"
    GAME_GENERATED_DISPATCH_C "${GAME_GENERATED_DISPATCH}"
    DEBUG_PORT 4520
    WINDOW_TITLE "MegaManX5Recomp"
    DEFAULT_GAME_CONFIG_PATH "game.toml"
    EXE_NAME "mmx5-runtime"
    LAUNCHER_BOXART "${CMAKE_SOURCE_DIR}/recomp/launcher/boxart.tga"
    PRELOADED_MODS_DIR "${MMX5_PRELOADED_MODS}"
    # Loading policy belongs to this title; generic acceleration is retired.
    EXCLUDE_BUILTIN_MODS
        psx.enhancement.fast-loading
        psx.enhancement.cd-speed
    EXTRAS_SOURCES
        ${GAME_GENERATED_EXTRAS}
        ${MMX5_MOD_PLUGIN_SOURCES}
)
psxrecomp_add_implementation(psx-runtime
    NAME resident-loading CONTRACT slus01334-archive-callback-v1
    HLE_SOURCES src/mods/mmx5_resident.cpp
    LLE_SOURCES src/mods/mmx5_resident_reference.c
    CONTRACT_FILES src/mods/mmx5_resident_catalog.h
        "${PSXRECOMP_V4_ROOT}/runtime/include/mod_resident_sector.hpp")
add_custom_command(TARGET psx-runtime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/game.toml"
        "$<TARGET_FILE_DIR:psx-runtime>/game.toml"
    VERBATIM)

if(BUILD_TESTING)
    add_executable(mmx5_coop_armor_test tests/test_mmx5_coop_armor.c src/mods/mmx5_coop_armor.c)
    target_include_directories(mmx5_coop_armor_test PRIVATE src/mods)
    add_test(NAME mmx5_coop_armor_test COMMAND mmx5_coop_armor_test)
    find_package(Threads REQUIRED)
    add_executable(mmx5_diagnostics_test tests/test_mmx5_diagnostics.cpp)
    target_include_directories(mmx5_diagnostics_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    target_link_libraries(mmx5_diagnostics_test PRIVATE Threads::Threads)
    add_test(NAME mmx5_diagnostics_test COMMAND mmx5_diagnostics_test "${CMAKE_CURRENT_BINARY_DIR}/diagnostics-qa")
    add_executable(mmx5_diagnostic_writer_test tests/test_mmx5_diagnostic_writer.cpp)
    target_include_directories(mmx5_diagnostic_writer_test PRIVATE src/mods)
    target_link_libraries(mmx5_diagnostic_writer_test PRIVATE Threads::Threads)
    add_test(NAME mmx5_diagnostic_writer_test COMMAND mmx5_diagnostic_writer_test "${CMAKE_CURRENT_BINARY_DIR}/writer-qa")
    set_tests_properties(mmx5_diagnostic_writer_test PROPERTIES TIMEOUT 15)
    add_executable(mmx5_startup_log_test tests/test_mmx5_startup_log.cpp src/mods/mmx5_startup_log.cpp)
    target_include_directories(mmx5_startup_log_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    target_compile_definitions(mmx5_startup_log_test PRIVATE MMX5_STARTUP_LOG_TEST=1)
    target_link_libraries(mmx5_startup_log_test PRIVATE Threads::Threads)
    add_test(NAME mmx5_startup_log_test COMMAND mmx5_startup_log_test "${CMAKE_CURRENT_BINARY_DIR}/startup-qa")
    add_test(NAME mmx5_startup_log_no_console_test COMMAND mmx5_startup_log_test "${CMAKE_CURRENT_BINARY_DIR}/startup-no-console-qa" closed)
    add_test(NAME mmx5_startup_log_unavailable_test COMMAND mmx5_startup_log_test "${CMAKE_CURRENT_BINARY_DIR}/startup-unavailable-qa" unavailable)
    add_test(NAME mmx5_startup_log_pointer_unavailable_test COMMAND mmx5_startup_log_test "${CMAKE_CURRENT_BINARY_DIR}/startup-pointer-unavailable-qa" pointer-unavailable)
    add_executable(mmx5_coop_memory_test tests/test_mmx5_coop_memory.c src/mods/mmx5_coop_memory.c)
    target_include_directories(mmx5_coop_memory_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    add_test(NAME mmx5_coop_memory_test COMMAND mmx5_coop_memory_test)
    add_executable(mmx5_coop_views_test tests/test_mmx5_coop_views.c src/mods/mmx5_coop_views.c)
    target_include_directories(mmx5_coop_views_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    add_test(NAME mmx5_coop_views_test COMMAND mmx5_coop_views_test)
    add_executable(mmx5_coop_texture_cache_test
        tests/test_mmx5_coop_texture_cache.c src/mods/mmx5_coop_texture_cache.c)
    target_include_directories(mmx5_coop_texture_cache_test PRIVATE src/mods)
    add_test(NAME mmx5_coop_texture_cache_test COMMAND mmx5_coop_texture_cache_test)
    add_executable(mmx5_coop_audio_test tests/test_mmx5_coop_audio.c
        src/mods/mmx5_coop_audio.c src/mods/mmx5_coop_assets.c)
    target_include_directories(mmx5_coop_audio_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    add_test(NAME mmx5_coop_audio_test COMMAND mmx5_coop_audio_test)
    add_executable(mmx5_coop_pixel_codec_test tests/test_mmx5_coop_pixel_codec.c
        src/mods/mmx5_coop_pixel_codec.c)
    target_include_directories(mmx5_coop_pixel_codec_test PRIVATE src/mods)
    add_test(NAME mmx5_coop_pixel_codec_test COMMAND mmx5_coop_pixel_codec_test)
    add_executable(mmx5_coop_snapshot_test tests/test_mmx5_coop_snapshot.c
        src/mods/mmx5_coop_pixel_codec.c src/mods/mmx5_coop_damage.c)
    target_include_directories(mmx5_coop_snapshot_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    if(MSVC)
        target_compile_options(mmx5_coop_snapshot_test PRIVATE /Gy /Gw)
        target_link_options(mmx5_coop_snapshot_test PRIVATE /OPT:REF)
    else()
        target_compile_options(mmx5_coop_snapshot_test PRIVATE -ffunction-sections -fdata-sections)
        if(APPLE)
            target_link_options(mmx5_coop_snapshot_test PRIVATE -Wl,-dead_strip)
        else()
            target_link_options(mmx5_coop_snapshot_test PRIVATE -Wl,--gc-sections)
        endif()
    endif()
    add_test(NAME mmx5_coop_snapshot_test COMMAND mmx5_coop_snapshot_test)
    add_executable(mmx5_coop_lifecycle_test
        tests/test_mmx5_coop_lifecycle.c src/mods/mmx5_coop_lifecycle.c)
    target_include_directories(mmx5_coop_lifecycle_test PRIVATE src/mods)
    add_test(NAME mmx5_coop_lifecycle_test COMMAND mmx5_coop_lifecycle_test)
    add_executable(mmx5_coop_damage_test
        tests/test_mmx5_coop_damage.c src/mods/mmx5_coop_damage.c)
    target_include_directories(mmx5_coop_damage_test PRIVATE src/mods)
    add_test(NAME mmx5_coop_damage_test COMMAND mmx5_coop_damage_test)
    add_executable(mmx5_coop_duel_damage_test
        tests/test_mmx5_coop_duel_damage.c src/mods/mmx5_coop_duel_damage.c)
    target_include_directories(mmx5_coop_duel_damage_test PRIVATE src/mods)
    add_test(NAME mmx5_coop_duel_damage_test COMMAND mmx5_coop_duel_damage_test)
    add_executable(mmx5_coop_duel_appearance_test
        tests/test_mmx5_coop_duel_appearance.c src/mods/mmx5_coop_duel_appearance.c)
    target_include_directories(mmx5_coop_duel_appearance_test PRIVATE src/mods)
    add_test(NAME mmx5_coop_duel_appearance_test COMMAND mmx5_coop_duel_appearance_test)
    add_executable(mmx5_coop_balance_test
        tests/test_mmx5_coop_balance.c src/mods/mmx5_coop_damage.c)
    target_include_directories(mmx5_coop_balance_test PRIVATE src/mods)
    add_test(NAME mmx5_coop_balance_test COMMAND mmx5_coop_balance_test)
    add_executable(mmx5_coop_assets_test
        tests/test_mmx5_coop_assets.c src/mods/mmx5_coop_assets.c)
    target_include_directories(mmx5_coop_assets_test PRIVATE src/mods)
    add_test(NAME mmx5_coop_assets_test COMMAND mmx5_coop_assets_test)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME mmx5_vwait_contract
        COMMAND "${Python3_EXECUTABLE}"
            "${PSXRECOMP_V4_ROOT}/runtime/tests/native_wait_caller.py"
            --framework "${PSXRECOMP_V4_ROOT}" --cc "${CMAKE_C_COMPILER}"
            --generated "${CMAKE_CURRENT_SOURCE_DIR}/generated"
            --contract "${CMAKE_CURRENT_SOURCE_DIR}/tests/vwait_contract.json")
    set_tests_properties(mmx5_vwait_contract PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE)

    add_test(NAME mmx5_coop_hooks_test COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mmx5_coop_hooks.py")
    add_test(NAME mmx5_diagnostic_replay_prepare_test COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_replay_mmx5_diagnostics.py")
    add_test(
        NAME mmx5_preloaded_mods_test
        COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_preloaded_mods.py"
                "${CMAKE_CURRENT_SOURCE_DIR}/mods/preloaded")
endif()

# First-divergence co-sim oracle build (COSIM_ORACLE.md): clean, deterministic,
# PSX_NO_DEBUG_TOOLS + PSX_COSIM. Two of these (one normal, one PSX_FORCE_INTERP=1)
# are cycle-lockstepped by tools/cosim.py to find the exact first compiled-vs-interp
# divergence. Only built on explicit request (-DPSX_BUILD_COSIM=ON) to keep normal
# game builds lean.
if(PSX_BUILD_COSIM)
    psxrecomp_v4_add_runtime_target(psx-cosim
        GAME_GENERATED_FULL_C "${GAME_GENERATED_FULL}"
        GAME_GENERATED_DISPATCH_C "${GAME_GENERATED_DISPATCH}"
        WINDOW_TITLE "MMX5-cosim"
        DEFAULT_GAME_CONFIG_PATH "game.toml"
        EXTRAS_SOURCES ${GAME_GENERATED_EXTRAS} ${MMX5_MOD_PLUGIN_SOURCES}
        COSIM
    )
    psxrecomp_add_implementation(psx-cosim
        NAME resident-loading CONTRACT slus01334-archive-callback-v1
        HLE_SOURCES src/mods/mmx5_resident.cpp
        LLE_SOURCES src/mods/mmx5_resident_reference.c
        CONTRACT_FILES src/mods/mmx5_resident_catalog.h
            "${PSXRECOMP_V4_ROOT}/runtime/include/mod_resident_sector.hpp")
endif()
