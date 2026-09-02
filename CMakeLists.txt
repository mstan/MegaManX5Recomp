cmake_minimum_required(VERSION 3.20)
project(MegaManX5Recomp C CXX)
include(CTest)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

set(PSXRECOMP_V4_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/psxrecomp-v4" CACHE PATH
    "Path to psxrecomp-v4")

include("${PSXRECOMP_V4_ROOT}/runtime/runtime.cmake")

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
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(
        NAME mmx5_preloaded_mods_test
        COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_preloaded_mods.py"
                "${MMX5_PRELOADED_MODS}")
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
