# run_mmx5.ps1 - autonomous launcher for the MMX5 recomp dev build.
# Bakes BIOS + disc paths so no interactive picker is ever needed and the
# space-containing .cue path is quoted as a single argument (PS 5.1 does NOT
# auto-quote -ArgumentList array elements -> the path splits at "Mega").
#
# Usage:  powershell -File tools\run_mmx5.ps1 [-BuildDir build-master]
param(
    [string]$BuildDir = "build-master",
    [switch]$NoLauncher   # boot straight into the game for scripted/debug runs
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
# Game-specific exe name (framework sets OUTPUT_NAME from the window title) so an
# X5 run never collides with another PSX title's process (e.g. Tomba 2). Only ever
# stop THIS title's process by name, never the generic "psx-runtime".
$exeName = "MegaManX5Recomp"
$exe  = Join-Path $root "$BuildDir\$exeName.exe"
$game = Join-Path $root "game.toml"
# The mmx5 framework worktree's copy — the main checkout's bios/ dir lost its
# SCPH1001.BIN (2026-07-02), and this project builds against the worktree anyway.
$bios = Join-Path $root "psxrecomp-v4\bios\SCPH1001.BIN"
$disc = Join-Path $root "mmx5\Mega Man X5 (USA).cue"

if (-not (Test-Path $exe))  { throw "exe not found: $exe" }
if (-not (Test-Path $bios)) { throw "bios not found: $bios" }
if (-not (Test-Path $disc)) { throw "disc not found: $disc" }

# Pre-seed the runtime's path cache so even a bare launch resolves correctly.
Set-Content -Path (Join-Path $root "$BuildDir\bios.cfg") -Value $bios -Encoding utf8 -NoNewline
Set-Content -Path (Join-Path $root "$BuildDir\disc.cfg") -Value $disc -Encoding utf8 -NoNewline

Get-Process $exeName -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 300

# Build a single arg string with explicit quotes around every path.
$argline = "--game `"$game`" --bios `"$bios`" --disc `"$disc`""
if ($NoLauncher) { $argline += " --no-launcher" }
$p = Start-Process -FilePath $exe -ArgumentList $argline -WorkingDirectory $root `
        -RedirectStandardError  (Join-Path $root "_mmx5_stderr.txt") `
        -RedirectStandardOutput (Join-Path $root "_mmx5_stdout.txt") -PassThru
Start-Sleep -Seconds 3
if ($p.HasExited) {
    Write-Output "EXITED code=$($p.ExitCode)"
    Get-Content (Join-Path $root "_mmx5_stderr.txt") -ErrorAction SilentlyContinue
} else {
    Write-Output "RUNNING pid=$($p.Id) ($BuildDir)"
}
