param([switch]$VerifyOnly)
$Grizzly = $true; $Duel = $false; $Gaea = $false
if (([int]$false + [int]$true + [int]$false) -gt 1) { throw "Choose one prepared encounter." }
$ErrorActionPreference = 'Stop'
$coopRoot = $PSScriptRoot
Set-Location -LiteralPath $coopRoot
foreach ($coopKey in @(Get-ChildItem Env: | Where-Object { $_.Name -match '^(PSX_|RNET_|LNG_|MMX5_DIAGNOSTIC_|MMX5_RECOMP_)' })) {
    Remove-Item -LiteralPath ('Env:' + $coopKey.Name)
}
foreach ($coopKey in @('SDL_AUDIODRIVER','SDL_AUDIO_DRIVER')) {
    Remove-Item -LiteralPath ('Env:' + $coopKey) -ErrorAction SilentlyContinue
}
$coopExe = Join-Path $coopRoot 'mmx5-runtime.exe'
$coopSaves = Join-Path $coopRoot 'saves'
$coopRun = $coopRoot
if ($Duel -or $Grizzly -or $Gaea) {
    $coopFixtureName = if ($Grizzly) { 'intro-start' } elseif ($Gaea) { 'gaea-start' } else { 'duel-start' }
    $coopRunName = if ($Grizzly) { 'intro-block-runs' } elseif ($Gaea) { 'gaea-runs' } else { 'duel-runs' }
    $coopSlot = if ($Grizzly) { '5' } elseif ($Gaea) { '9' } else { '7' }
    $coopState = 'state_8005894C_slot0' + $coopSlot + '.pst'
    $coopExpected = '309dced11919e51abfe9f37a6589ee8f1dddf7afcf91fa9d5b506eff7c792601'
    if ((Get-FileHash -LiteralPath $coopExe -Algorithm SHA256).Hash.ToLowerInvariant() -ne $coopExpected) {
        throw 'The prepared snapshot requires the exact executable shipped in this QA package.'
    }
    $coopRun = Join-Path $coopRoot (('qa/' + $coopRunName + '/') + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0,8))
    $coopSaves = Join-Path $coopRun 'saves'
    if (-not $VerifyOnly) {
        [System.IO.Directory]::CreateDirectory((Join-Path $coopSaves 'openbios')) | Out-Null
        $coopFixture = Join-Path $coopRoot ('qa/' + $coopFixtureName)
        Copy-Item -LiteralPath (Join-Path $coopFixture 'card1.mcd') -Destination (Join-Path $coopSaves 'card1.mcd')
        Copy-Item -LiteralPath (Join-Path $coopFixture 'card1.mcd') -Destination (Join-Path $coopSaves 'openbios/card1.mcd')
        Copy-Item -LiteralPath (Join-Path $coopFixture $coopState) -Destination (Join-Path $coopSaves ('openbios/' + $coopState))
    }
    $env:PSX_LOAD_SLOT = $coopSlot
}
$coopScratch = Join-Path $coopRun 'scratch'
$coopPaths = @{
    TEMP='tmp'; TMP='tmp'; TMPDIR='tmp'; LOCALAPPDATA='localappdata'; APPDATA='appdata';
    XDG_CACHE_HOME='cache'; XDG_DATA_HOME='data'; XDG_CONFIG_HOME='config';
    PYTHONPYCACHEPREFIX='cache/python'; MESA_SHADER_CACHE_DIR='shaders'; __GL_SHADER_DISK_CACHE_PATH='shaders'
}
foreach ($coopKey in $coopPaths.Keys) {
    $coopPath = Join-Path $coopScratch $coopPaths[$coopKey]
    if (-not $VerifyOnly) { [System.IO.Directory]::CreateDirectory($coopPath) | Out-Null }
    [Environment]::SetEnvironmentVariable($coopKey,$coopPath,'Process')
}
$env:MMX5_RECOMP_DATA_DIR = $coopRoot
$coopDiscSetting = Join-Path $coopRoot 'disc-path.txt'
$coopDisc = ''
if (Test-Path -LiteralPath $coopDiscSetting) { $coopDisc = (Get-Content -LiteralPath $coopDiscSetting -Raw).Trim() }
if (-not $VerifyOnly -and ([string]::IsNullOrWhiteSpace($coopDisc) -or -not (Test-Path -LiteralPath $coopDisc -PathType Leaf))) {
    Add-Type -AssemblyName System.Windows.Forms
    $coopPicker = New-Object System.Windows.Forms.OpenFileDialog
    $coopPicker.Title = 'Select your Mega Man X5 USA disc'
    $coopPicker.Filter = 'PlayStation disc (*.cue;*.chd)|*.cue;*.chd|All files (*.*)|*.*'
    if ($coopPicker.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) { exit 0 }
    $coopDisc = $coopPicker.FileName
    [System.IO.File]::WriteAllText($coopDiscSetting,$coopDisc)
}
$coopListener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Loopback,0)
$coopListener.Start(); $coopPort = $coopListener.LocalEndpoint.Port; $coopListener.Stop()
$coopArgs = @('--game',(Join-Path $coopRoot 'game.toml'),'--bios',(Join-Path $coopRoot 'bios/openbios.bin'),'--disc',$coopDisc,'--renderer','opengl','--memcard-dir',$coopSaves,'--debug-port',[string]$coopPort)
if ($Duel -or $Grizzly -or $Gaea) { $coopArgs += '--no-launcher' } else { $coopArgs += '--launcher' }
$coopLaunch = [pscustomobject]@{executable=$coopExe;arguments=$coopArgs;working_directory=$coopRoot;saves=$coopSaves;temp=$env:TEMP;localappdata=$env:LOCALAPPDATA;audio_driver=$env:SDL_AUDIODRIVER;duel=$false;grizzly=$true;gaea=$false;load_slot=$env:PSX_LOAD_SLOT}
if ($VerifyOnly) { $coopLaunch | ConvertTo-Json -Depth 4; exit 0 }
[System.IO.Directory]::CreateDirectory($coopSaves) | Out-Null
$coopLaunch | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $coopRun 'owner-launch.json') -Encoding UTF8
& $coopExe @coopArgs
exit $LASTEXITCODE
