param(
    [string]$Version = "v0.0.2-alpha",
    [string]$BuildDir = "build-release",
    # Where your accumulated overlay cache lives (the dir compile_overlays.py
    # writes to, per game.toml overlay_autocompile_cmd --out-dir). Bundled as a
    # head start; optional. X5's cache lives at build-release/cache/SLUS-01334.
    [string]$CacheBuildDir = "build-release",
    [switch]$SkipRegen
)

$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
# All framework paths go through the psxrecomp-v4 junction at $Root so this
# game's framework pin is honored (see the regen note below).
$FrameworkRoot = Join-Path $Root "psxrecomp-v4"
if (-not (Test-Path -LiteralPath (Join-Path $FrameworkRoot "tools\release_overlay_stage.ps1"))) {
    throw ("No psxrecomp framework checkout at $FrameworkRoot " +
           "(expected tools\release_overlay_stage.ps1). Run " +
           "'git submodule update --init psxrecomp-v4'.")
}
$BuildPath = Join-Path $Root $BuildDir
$StageRoot = Join-Path $Root "release-stage"
$Stage = Join-Path $StageRoot "MegaManX5Recomp-windows-x64"
$ZipPath = Join-Path $Root ("MegaManX5Recomp-{0}-windows-x64.zip" -f $Version)
$MingwBin = "C:\msys64\mingw64\bin"

$env:PATH = "$MingwBin;$env:PATH"

# Regenerate the game's C BEFORE building. The runtime build below just compiles
# generated/*.c, so a stale generated/ would ship the wrong code.
# cmake writes benign warnings (e.g. freetype's cmake_minimum_required
# deprecation) to STDERR. Under $ErrorActionPreference='Stop', PowerShell 5.1
# promotes a native command's stderr write to a TERMINATING error, aborting the
# release for a non-error. Run the native cmake invocations with the preference
# relaxed and gate on the real signal -- $LASTEXITCODE -- instead.
function Invoke-Native {
    param([scriptblock]$Cmd, [string]$What)
    $old = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & $Cmd 2>&1 | Out-Host
    $code = $LASTEXITCODE
    $ErrorActionPreference = $old
    if ($code -ne 0) { throw "$What failed (exit $code)" }
}

function Get-TomlScalar {
    param(
        [Parameter(Mandatory)][string]$GameToml,
        [Parameter(Mandatory)][string]$Table,
        [Parameter(Mandatory)][string]$Key
    )
    $section = ""
    foreach ($raw in (Get-Content -LiteralPath $GameToml)) {
        $line = $raw.Trim()
        if (-not $line -or $line.StartsWith("#")) { continue }
        if ($line -match '^\[\[?([^\]]+)\]\]?$') { $section = $Matches[1].Trim(); continue }
        if ($section -ne $Table) { continue }
        if ($line -match ('^' + [regex]::Escape($Key) + '\s*=\s*(.+?)\s*(?:#.*)?$')) {
            return $Matches[1].Trim().Trim('"').Trim("'")
        }
    }
    return $null
}

function Ensure-BiosBackends {
    param([Parameter(Mandatory)][string]$FrameworkRoot)
    $stems = @()
    if (Test-Path -LiteralPath (Join-Path $FrameworkRoot "bios\OpenBIOS.toml")) {
        $stems += ,@("OpenBIOS", "bios/OpenBIOS.toml")
    }
    if (Test-Path -LiteralPath (Join-Path $FrameworkRoot "bios\SCPH1001.BIN")) {
        $stems += ,@("SCPH1001", "bios/SCPH1001.toml")
    }
    if (-not $stems) { throw "No BIOS profile available under $FrameworkRoot\bios" }

    $missing = @($stems | Where-Object {
        -not (Test-Path -LiteralPath (Join-Path $FrameworkRoot ("generated\{0}_dispatch.c" -f $_[0])))
    })
    if (-not $missing) { return }

    $bash = $null
    foreach ($cand in @("C:\msys64\usr\bin\bash.exe", "C:\msys64\mingw64\bin\bash.exe")) {
        if (Test-Path -LiteralPath $cand) { $bash = $cand; break }
    }
    if (-not $bash) {
        throw ("Missing recompiled BIOS backend(s): {0}. Install MSYS2 or run " +
               "psxrecomp-v4/tools/regen_bios.sh manually." -f (($missing | ForEach-Object { $_[0] }) -join ', '))
    }

    $cygpath = Join-Path (Split-Path -Parent $bash) "cygpath.exe"
    $posixRoot = (& $cygpath -u $FrameworkRoot).Trim()
    $posixMingw = (& $cygpath -u $MingwBin).Trim()
    foreach ($stem in $missing) {
        Write-Host "Generating recompiled BIOS backend: $($stem[0])"
        $biosShellCmd = "export PATH='$posixMingw':`$PATH; cd '$posixRoot' && " +
                        "PSXRECOMP_BIOS_BUILD=recompiler/build tools/regen_bios.sh --config $($stem[1])"
        Invoke-Native { & $bash -c $biosShellCmd } "regen_bios ($($stem[0]))"
    }
}
# X5 builds against its psxrecomp-v4 junction (-> the wt/mmx5 framework
# worktree), NOT the master ..\psxrecomp checkout. All framework paths go
# through the junction at $Root so this game's framework pin is honored.
$RecompDir = Resolve-Path (Join-Path $Root "psxrecomp-v4\recompiler\build")
if (-not $SkipRegen) {
    Invoke-Native { cmake --build $RecompDir --target psxrecomp-game -j $env:NUMBER_OF_PROCESSORS } "recompiler build"
    Ensure-BiosBackends -FrameworkRoot $FrameworkRoot
    & (Join-Path $RecompDir "psxrecomp-game.exe") --config (Join-Path $Root "game.toml")
    if ($LASTEXITCODE -ne 0) { throw "game regen failed" }
} else {
    Ensure-BiosBackends -FrameworkRoot $FrameworkRoot
    Write-Host "Skipping game C regeneration; packaging the existing generated sources"
}

Invoke-Native { cmake -S $Root -B $BuildPath -G Ninja -DCMAKE_BUILD_TYPE=Release -DPSX_DEBUG_TOOLS=OFF } "cmake configure"
Invoke-Native { cmake --build $BuildPath -j $env:NUMBER_OF_PROCESSORS } "cmake build"

if (Test-Path $StageRoot) {
    Remove-Item -Recurse -Force $StageRoot
}
New-Item -ItemType Directory -Force $Stage | Out-Null
New-Item -ItemType Directory -Force (Join-Path $Stage "saves") | Out-Null

# The CMake OUTPUT_NAME may already be MegaManX5Recomp.exe; accept the pre-rename
# per-game name (mmx5-runtime.exe) and the generic psx-runtime.exe too.
$DevExe = Join-Path $BuildPath "MegaManX5Recomp.exe"
if (-not (Test-Path $DevExe)) { $DevExe = Join-Path $BuildPath "mmx5-runtime.exe" }
if (-not (Test-Path $DevExe)) { $DevExe = Join-Path $BuildPath "psx-runtime.exe" }
Copy-Item $DevExe (Join-Path $Stage "MegaManX5Recomp.exe")
Copy-Item (Join-Path $Root "README.md") $Stage
Copy-Item (Join-Path $Root "LICENSE") $Stage
# Mod catalog: staged from the BUILD OUTPUT via the framework's shared
# Add-ModCatalog, not copied out of the source tree.
#
# What used to be here copied <repo>/mods/preloaded straight into the stage.
# Two defects, one of them silent since the catalog existed:
#
#   * the source tree holds only THIS repo's two packages. The framework stages
#     four more (psx.enhancement.cd-speed / fast-loading / pgxp,
#     psx.presentation.bezel) into the build output for every game, so copying
#     the source tree shipped a Mods page missing four entries the dev build
#     shows -- the exact failure MegaManX6's packager comment warns about.
#   * it produced a mods/packages tree, the PRE-SPLIT layout. Framework
#     4cc04be3 moved the staged catalog to mods/bundled, which is what the
#     launcher and every other packager now read (bead beads-eio.3.101).
#
# Add-ModCatalog copies <build>/mods, asserts that every package the SOURCES
# define -- this repo's mods/preloaded/packages and the framework's
# mods/builtin/packages -- survived into mods/bundled, and strips the two
# things under mods/ that belong to this machine (installed/ and state.toml).
# It asserts that invariant rather than a count, which cannot go stale when
# either side gains a mod.
. (Join-Path $FrameworkRoot "tools\release_overlay_stage.ps1")
Add-ModCatalog -BuildPath $BuildPath -Stage $Stage `
               -GameModSource (Join-Path $Root "mods\preloaded") `
               -FrameworkModSource (Join-Path $FrameworkRoot "mods\builtin") | Out-Null
$BundledBiosSrc = Join-Path $BuildPath "bios"
if (!(Test-Path (Join-Path $BundledBiosSrc "openbios.bin")) -or
    (Get-Item (Join-Path $BundledBiosSrc "openbios.bin")).Length -ne 524288 -or
    !(Test-Path (Join-Path $BundledBiosSrc "OpenBIOS.LICENSE"))) {
    throw "Runtime build did not stage OpenBIOS and its MIT notice"
}
$BundledBiosDst = Join-Path $Stage "bios"
New-Item -ItemType Directory -Force $BundledBiosDst | Out-Null
Copy-Item (Join-Path $BundledBiosSrc "openbios.bin") $BundledBiosDst
Copy-Item (Join-Path $BundledBiosSrc "OpenBIOS.LICENSE") $BundledBiosDst
if (Test-Path (Join-Path $Root "RELEASE_NOTES.md")) {
    Copy-Item (Join-Path $Root "RELEASE_NOTES.md") $Stage
}

# Launcher assets: this build ships the shared recomp-ui Dear ImGui launcher
# (RECOMP_LAUNCHER; see main.cpp + recomp-ui/recomp_ui.cmake), which loads from
# <exe>/assets/ (fonts + img TGAs, including this repo's boxart baked in by
# recomp_target_launcher_ui's POST_BUILD).
$AssetsSrc = Join-Path $BuildPath "assets"
if (-not (Test-Path (Join-Path $AssetsSrc "img"))) {
    throw "recomp-ui launcher assets missing at $AssetsSrc -- was the recomp-ui launcher built (recomp-ui junction present)?"
}
Copy-Item -Recurse -Force $AssetsSrc (Join-Path $Stage "assets")
$fontCount = (Get-ChildItem (Join-Path $Stage "assets/fonts") -Filter *.ttf -ErrorAction SilentlyContinue).Count
$imgCount  = (Get-ChildItem (Join-Path $Stage "assets/img")   -Filter *.tga -ErrorAction SilentlyContinue).Count
Write-Host "Bundled recomp-ui launcher assets: $fontCount font(s) + $imgCount image(s)"

# Player-facing game.toml: same effective runtime settings as the dev config,
# minus dev-only sections ([recompiler] inputs beyond the required block, the
# gcc overlay-autocompile command, and the [audit] block). overlay_backend is
# left at the default "auto": with no gcc toolchain on a player box it resolves
# to tcc, which fills overlay gaps via the bundled overlay_toolchain/ (no system
# python or gcc needed). Players can edit [runtime]/[video] post-install.
@"
[game]
name = "Mega Man X5"
id = "SLUS-01334"
exe = "mmx5/SLUS_013.34"
disc = "mmx5/Mega Man X5 (USA).cue"
load_address = "0x80010000"
entry_pc = "0x8005894C"
text_size = "0x00082000"
stack_base = "0x801FFFF0"

# Required block; used only by the developer recompiler tool, not at runtime.
[recompiler]
seeds = "seeds/ghidra_funcs.txt"
out_dir = "generated"

# ---- Player-adjustable options ------------------------------------------
# Edit, save, and restart MegaManX5Recomp.exe to apply.
[runtime]
window_title = "Mega Man X5 Recompiled"
memcard_dir = "saves"

# Disc read speed. "1x" is authentic PlayStation timing and is the safe default:
# speeding up the emulated CD device changes how many frames pass between the
# game's internal steps, which desyncs streamed audio and wedges timing-sensitive
# Mega Man X engine loops. Fast loads instead come from turbo_loads below (which
# fast-forwards the whole machine during a load, preserving timing).
disc_speed = "1x"

# Skip the BIOS shell animation after OpenBIOS initializes and proceed directly
# to the game. This does not replace the BIOS kernel or hardware simulation.
bios_hle = true

# Turbo loads: while a load is in progress, run the machine at full host speed so
# loading finishes much faster, with all game timing preserved. Audio plays
# through normally. On by default. Toggleable in the launcher (Settings -> Turbo
# loads).
turbo_loads = true

# Overlay cache: keeps converted native code for game areas in the cache folder,
# and records newly visited areas into overlay_captures.json so your own cache
# grows as you play. Keep that file private - it contains game code from your
# disc (see README).
overlay_cache = true

# ---- Visual quality -----------------------------------------------------
[video]
# supersampling: render at this multiple of native resolution and downsample,
# for higher detail and anti-aliased edges. 1 = native PSX look, 2 = recommended,
# 3-4 = sharper (needs a faster CPU to hold full speed).
supersampling = 2
# antialiasing: smooth (linear) scaling to the window. false = sharp pixels.
antialiasing  = true
# texture_filtering: "nearest" = native PSX look; "bilinear" = smooths textures.
texture_filtering = "nearest"
# renderer: "opengl" = validated hardware renderer and release default.
# "software" remains available for the native CPU-rendered look; the current
# framework also exposes Vulkan.
renderer = "opengl"
# auto_skip_fmv: skip full-motion videos (the CAPLOGO / X5OP opening movies).
# Off by default so you see the now-working intro cutscene. When on, a video is
# skipped the instant it starts. Toggleable in the launcher (Settings -> "Skip
# FMVs").
auto_skip_fmv = false
# Widescreen and frame interpolation are game-owned Mods rather than generic
# display preferences. Their activation plugins apply values after the mod plan
# commits; old persisted display values are ignored.
aspect_ratio = "4:3"
offer_frame_interpolation = false

# ---- Controller ---------------------------------------------------------
# default_analog: MMX5 will not poll buttons until it detects an analog pad, so
# present a DualShock by default. Per-player toggle in the launcher. deadzone:
# analog stick dead-band (0..32767; ~12000 = 37%), also adjustable in the launcher.
[controller]
default_analog = true
deadzone = 12000
# MMX5 requires a DualShock, so the launcher hides the "Hybrid" pad mode and
# offers only Analog / D-Pad.
allow_hybrid = false

# ---- Widescreen ---------------------------------------------------------
# The hooks are compiled in but identity at 4:3. The default-off widescreen mod
# activates 16:9; generic launcher aspect controls stay hidden.
[widescreen]
full_2d = true
offer = false
offer_ultrawide = false
nw_left_hud_packet_lo = "0x000E8500"
nw_left_hud_packet_hi = "0x000E9300"

[widescreen.bg2d]
count_site        = "0x80028218"
startcol_site     = "0x80028040"
startx_site       = "0x80028058"
stream_left_site  = "0x800282B8"
stream_right_site = "0x800282CC"
cap_site          = "0x8002810C"
layer_base        = "0x8009A1F8"
ring_base         = "0x800A51A8"
map_size_addr     = "0x800D1DBC"
layer_stride_addr = "0x80091D58"
ring_cols = 32
layer_count = 3
layer_struct_stride = 84
packet_cap = 1024

[widescreen.cull]
auto_screen_x = true
screen_x_sites = [
  "0x80032D90",
  "0x80032E20",
  "0x80032EB4",
  "0x80032F48",
]
bias_sites = [
  "0x8002D86C",
  "0x8002D908",
  "0x8002DB0C",
]
range_sites = [
  "0x8002D874",
  "0x8002D910",
  "0x8002DB14",
]
a1_sites = [
  "0x8002D948",
  "0x8002D9F4",
]
"@ | Set-Content -Encoding ASCII (Join-Path $Stage "game.toml")

# Prebuilt overlay cache + self-contained overlay toolchain, both staged by the
# shared framework implementation. The cache-required decision is read from the
# STAGED game.toml, since that file is folded into the tag and is the contract
# the released executable actually loads.
$RecompTools = (Resolve-Path (Join-Path $FrameworkRoot "tools")).Path
$RecompInc   = (Resolve-Path (Join-Path $FrameworkRoot "runtime\include")).Path
$StagedGameToml = Join-Path $Stage "game.toml"
$CacheGameId = Get-TomlScalar -GameToml $StagedGameToml -Table "game" -Key "id"
if (-not $CacheGameId) { throw "Could not read [game] id from $StagedGameToml" }

$CacheSrcRoot = if ([System.IO.Path]::IsPathRooted($CacheBuildDir)) {
    $CacheBuildDir
} else {
    Join-Path $Root $CacheBuildDir
}
foreach ($p in @($CacheSrcRoot, (Resolve-Path -LiteralPath $CacheSrcRoot -ErrorAction SilentlyContinue).Path)) {
    if ($p -and $p -match 'QUARANTINE') { throw "Refusing quarantined overlay cache source: $p" }
}
$CacheSrcRoot = Join-Path $CacheSrcRoot "cache"

$CgTag = Get-OverlayCgTag -RecompTools $RecompTools -RecompInc $RecompInc `
                          -GameExe (Join-Path $RecompDir "psxrecomp-game.exe") `
                          -GameToml $StagedGameToml `
                          -BuildPath $BuildPath -RuntimeTarget "psx-runtime"
Write-Host "Release codegen tag: $CgTag (only this cache namespace is shipped)"

$OverlayCacheDeclared =
    ((Get-TomlScalar -GameToml $StagedGameToml -Table "runtime" -Key "overlay_cache") -eq "true")
if ($OverlayCacheDeclared) {
    Write-Host "Staged game.toml declares overlay_cache = true; a shard cache is required"
    Add-OverlayCache -GameId $CacheGameId -CacheSrcRoot $CacheSrcRoot `
                     -Stage $Stage -CgTag $CgTag | Out-Null
} else {
    Write-Host "Staged game.toml does not declare overlay_cache; staging no shard cache"
}
Add-OverlayToolchain -Stage $Stage -RecompDir $RecompDir -RecompTools $RecompTools `
                     -RecompInc $RecompInc -MingwBin $MingwBin `
                     -DlCache (Join-Path $Root "tools\_toolchain_cache") | Out-Null
# The Release build is statically linked (PSX_STATIC_RUNTIME defaults ON for
# MinGW Release), so the exe imports ONLY Windows system DLLs -- nothing to
# bundle. Assert self-containment rather than trust it (mismatched side-by-side
# DLLs were the cause of the 0xc000007b launch crash on other projects).
$objdump = Join-Path $MingwBin "objdump.exe"
$imports = & $objdump -p (Join-Path $Stage "MegaManX5Recomp.exe") |
    Select-String "DLL Name: (.+)" | ForEach-Object { $_.Matches[0].Groups[1].Value.Trim() }
$systemDlls = @("kernel32.dll","user32.dll","gdi32.dll","shell32.dll","msvcrt.dll",
                "advapi32.dll","ws2_32.dll","comdlg32.dll","dbghelp.dll","ole32.dll",
                "oleaut32.dll","winmm.dll","imm32.dll","version.dll","setupapi.dll",
                "dinput8.dll","rpcrt4.dll","hid.dll","cfgmgr32.dll","opengl32.dll")
$nonSystem = $imports | Where-Object { $systemDlls -notcontains $_.ToLower() }
if ($nonSystem) {
    throw "Release exe is NOT self-contained -- imports non-system DLL(s): $($nonSystem -join ', ')"
}
Write-Host "Verified self-contained: imports only system DLLs ($($imports.Count) total)"

@"
; PSXRecomp input mapping. PSX buttons are active when any listed source is pressed.
; Sources use SDL/Xbox names: a,b,x,y,back,start,leftshoulder,rightshoulder,
; lefttrigger,righttrigger,dpup,dpdown,dpleft,dpright,leftx-/leftx+/lefty-/lefty+.

[controller]
enabled = true
device = 0
deadzone = 12000

[mapping]
up = dpup,lefty-
down = dpdown,lefty+
left = dpleft,leftx-
right = dpright,leftx+
cross = a
circle = b
square = x
triangle = y
l1 = leftshoulder
r1 = rightshoulder
l2 = lefttrigger
r2 = righttrigger
start = start
select = back
"@ | Set-Content -Encoding ASCII (Join-Path $Stage "input.ini")

@"
MegaManX5Recomp $Version

Mega Man X5 boots through the bundled OpenBIOS and plays through the opening
(including the intro cutscenes, which now decode and play), into stages, with
working controller input and memory-card save/load. End-to-end completion has
not yet been recertified for this build, so please report regressions.

This package includes the MIT-licensed OpenBIOS from PCSX-Redux and its notice
in bios/OpenBIOS.LICENSE. It does not include the Mega Man X5 disc, a retail
PlayStation BIOS, save data, or game assets. The executable contains recompiled
(machine-translated) builds of the game's code, the same distribution model
used by other static recompilation projects such as N64: Recompiled.

First launch:
1. Run MegaManX5Recomp.exe. A launcher window opens.
2. OpenBIOS is selected automatically. You may optionally browse for your
   legally obtained retail PlayStation BIOS.
3. Set the game disc: select your legally obtained Mega Man X5 (USA,
   SLUS-01334) disc image.
4. Adjust any options you like (renderer, supersampling, screen look,
   controller), then press Launch. Your choices are remembered next time.

Disc image formats:
- .cue + .bin (preferred - pick the .cue)
- .bin
Do NOT convert to a 2048-byte "cooked" .iso - it discards the XA sectors MMX5
streams its FMV/audio from.

Your selections are saved next to the executable. Clearing the BIOS row returns
to OpenBIOS.

Turbo loads, FMV skip, and disc speed live in Settings. Widescreen and frame
interpolation live in the launcher's Mods view.

The cache folder contains pre-converted native code for game areas covered so
far; those run at full speed from your first visit. As you play, newly visited
areas are recorded into overlay_captures.json and your local cache grows
automatically. Do NOT post overlay_captures.json publicly - it contains
snapshots of the game's own code read from your disc. See README.md for details.

Keyboard and Xbox-style controller defaults are documented in README.md.
Controller mappings are configurable in input.ini.

Memory cards are stored in the saves directory; save and load work with standard
PS1 .mcd images.
"@ | Set-Content -Encoding ASCII (Join-Path $Stage "START_HERE.txt")

if (Test-Path $ZipPath) {
    Remove-Item -Force $ZipPath
}
Compress-Archive -Path (Join-Path $Stage "*") -DestinationPath $ZipPath -Force

Write-Host "Wrote $ZipPath"
