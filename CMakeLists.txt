cmake_minimum_required(VERSION 3.20)
project(MegaManX5Recomp C CXX)
include(CTest)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# The co-op title exposes both controller seats and the launcher Host/Join
# flow on a fresh configure, just like X6's co-op branch.
option(PSX_NETPLAY "Build the shared two-player netplay stack" ON)

set(PSXRECOMP_V4_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/psxrecomp-v4" CACHE PATH
    "Path to psxrecomp-v4")

include("${PSXRECOMP_V4_ROOT}/runtime/runtime.cmake")

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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/*.c")

# Delay-sync peers must share the title and framework simulation contract.
file(GLOB MMX5_COOP_CONTRACT_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/mmx5_coop_*")
list(APPEND MMX5_COOP_CONTRACT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/game.toml"
    "${CMAKE_CURRENT_SOURCE_DIR}/mods/development/packages/mmx5.coop/0.0.1/manifest.toml")
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

# X5's two game-owned presentation mods.
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
if(PSX_DEBUG_TOOLS)
    set(MMX5_PRELOADED_MODS "${CMAKE_CURRENT_BINARY_DIR}/development-mods")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/mods/preloaded/"
        DESTINATION "${MMX5_PRELOADED_MODS}")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/mods/development/packages/mmx5.coop"
        DESTINATION "${MMX5_PRELOADED_MODS}/packages")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/mods/development/packages/mmx5.coop/0.0.1/manifest.toml")
endif()

psxrecomp_v4_add_runtime_target(psx-runtime
    GAME_GENERATED_FULL_C "${GAME_GENERATED_FULL}"
    GAME_GENERATED_DISPATCH_C "${GAME_GENERATED_DISPATCH}"
    DEBUG_PORT 4520
    WINDOW_TITLE "MegaManX5Recomp"
    DEFAULT_GAME_CONFIG_PATH "game.toml"
    EXE_NAME "mmx5-runtime"
    LAUNCHER_BOXART "${CMAKE_SOURCE_DIR}/recomp/launcher/boxart.tga"
    PRELOADED_MODS_DIR "${MMX5_PRELOADED_MODS}"
    EXTRAS_SOURCES
        ${GAME_GENERATED_EXTRAS}
        ${MMX5_MOD_PLUGIN_SOURCES}
)

if(BUILD_TESTING)
    add_executable(mmx5_coop_memory_test tests/test_mmx5_coop_memory.c src/mods/mmx5_coop_memory.c)
    target_include_directories(mmx5_coop_memory_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    add_test(NAME mmx5_coop_memory_test COMMAND mmx5_coop_memory_test)
    add_executable(mmx5_coop_views_test tests/test_mmx5_coop_views.c src/mods/mmx5_coop_views.c)
    target_include_directories(mmx5_coop_views_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    add_test(NAME mmx5_coop_views_test COMMAND mmx5_coop_views_test)
    add_executable(mmx5_coop_lifecycle_test
        tests/test_mmx5_coop_lifecycle.c src/mods/mmx5_coop_lifecycle.c)
    target_include_directories(mmx5_coop_lifecycle_test PRIVATE src/mods)
    add_test(NAME mmx5_coop_lifecycle_test COMMAND mmx5_coop_lifecycle_test)
    add_executable(mmx5_coop_assets_test
        tests/test_mmx5_coop_assets.c src/mods/mmx5_coop_assets.c)
    target_include_directories(mmx5_coop_assets_test PRIVATE src/mods)
    add_test(NAME mmx5_coop_assets_test COMMAND mmx5_coop_assets_test)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME mmx5_coop_hooks_test COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mmx5_coop_hooks.py")
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
        COSIM
    )
endif()
