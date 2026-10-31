"""Stage a local co-op playtest with private settings/cards.

Original game assets remain at their existing local paths. Nothing is published.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tomllib
from test_mmx5_coop_netplay import change_config


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build',type=Path,default=Path('build-coop'))
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--cache-source',type=Path,
                        help='Existing overlay cache root; the SDK selects the matching build/config tag')
    parser.add_argument('--no-overlay-cache',action='store_true',
                        help='Use the runtime without an external overlay cache or autocompiler')
    parser.add_argument('--disc',type=Path,help='Use an existing local disc from another non-C: location')
    parser.add_argument('--local-cameras',choices=('unified','split'),default='split')
    parser.add_argument('--door-players',choices=('either','both'),default='either')
    parser.add_argument('--save-card','--stage-access-card',dest='stage_access_card',type=Path,
                        help='Seed a fresh playtest with an unmodified native 128KB PS1 card')
    parser.add_argument('--save-source-url',help='Online source of the downloaded save card')
    parser.add_argument('--save-source-note',help='Local provenance of a game-created or owner-provided card')
    parser.add_argument('--debug-test-build',action='store_true',
                        help='Require both launcher controller seats, debug tools and the netplay lobby')
    args=parser.parse_args()
    root=Path(__file__).resolve().parent.parent
    native=tomllib.loads((root/'game.toml').read_text(encoding='utf-8'))
    args.no_overlay_cache |= not native.get('runtime',{}).get('overlay_cache',False)
    build=args.build.resolve()
    out=args.out.resolve()
    cache_source=args.cache_source.resolve() if args.cache_source else build/'cache'
    if not args.no_overlay_cache and not cache_source.is_dir():
        parser.error(f'Missing overlay cache: {cache_source}; supply --cache-source from a compatible local build')
    if out.exists():
        parser.error('Use a fresh output directory to preserve previous playtest settings/cards')
    card=args.stage_access_card.read_bytes() if args.stage_access_card else None
    if card is not None and (len(card)!=131072 or card[:2]!=b'MC'):
        parser.error('Save card must be a native 128KB PS1 card')
    if card is not None:
        if args.save_source_url and not args.save_source_url.startswith(('https://','http://')):
            parser.error('--save-source-url must be an HTTP(S) URL')
        if not args.save_source_url and not (args.save_source_note or '').strip():
            parser.error('A card requires --save-source-url or --save-source-note for provenance')
        if hashlib.sha256(card).hexdigest()=='4a173ef3b348f61073a89b8e416687f556b44b7475ef60c0e3564a0ac093f3e9':
            parser.error('The synthetic stage-access card is retired; use a native game-created save')
    exe=build/'mmx5-runtime.exe'
    bios=build/'bios/openbios.bin'
    package=build/'mods/bundled/mmx5.coop/0.0.1/manifest.toml'
    for path in (exe,bios,package):
        if not path.is_file():parser.error(f'Missing built input: {path}')
    disc=args.disc.resolve() if args.disc else (root/native['game']['disc']).resolve()
    if not disc.is_file():parser.error(f'Missing original disc: {disc}')
    if native['game'].get('players')!=2:
        parser.error('Co-op launcher must declare game.players=2 for P1/P2 bindings')
    cache_path=build/'CMakeCache.txt'
    cache=cache_path.read_text(encoding='utf-8') if cache_path.exists() else ''
    if args.debug_test_build:
        for flag in ('PSX_DEBUG_TOOLS','PSX_RECOMP_UI','PSX_NETPLAY'):
            if not re.search(r'^'+flag+r':BOOL=ON$',cache,re.M):
                parser.error(f'Debug test distribution requires {flag}=ON in {cache_path}')
        if not (build/'psx_game_version.txt').is_file():
            parser.error('Debug test distribution requires its built lobby version stamp')
        if not list((build/'assets/fonts').glob('*.ttf')) or not (build/'assets/img/boxart.tga').is_file():
            parser.error('Debug test distribution requires the built recomp-ui fonts and artwork')
    else:
        for flag in ('PSX_DEBUG_TOOLS','MMX5_COOP_TEST_CHEATS'):
            if not re.search(r'^'+flag+r':BOOL=OFF$',cache,re.M):
                parser.error(f'Owner build requires {flag}=OFF; use --debug-test-build for a diagnostic build')
    for manifest_path in (build/'mods/bundled').glob('*/*/manifest.toml'):
        manifest=tomllib.loads(manifest_path.read_text(encoding='utf-8'))
        if manifest['id'] in ('psx.enhancement.cd-speed','psx.enhancement.fast-loading'):
            parser.error(f'Owner payload must exclude {manifest["id"]}')
    stage_tool=root/'psxrecomp-v4/tools/release_stage.py'
    subprocess.run([sys.executable,str(stage_tool),'stage-execution','--binary',str(exe)],check=True)
    out.mkdir(parents=True)
    shutil.copy2(exe,out/exe.name)
    shutil.copy2(exe.with_suffix('.execution.json'),out/'mmx5-runtime.execution.json')
    version=build/'psx_game_version.txt'
    if version.is_file():shutil.copy2(version,out/version.name)
    for dll in build.glob('*.dll'):
        if dll.name!='asset-research.dll':
            shutil.copy2(dll,out/dll.name)
    for name in ('input.ini','keybinds.ini'):
        if (build/name).is_file():shutil.copy2(build/name,out/name)
    if (build/'psxrecomp.png').is_file():
        shutil.copy2(build/'psxrecomp.png',out/'psxrecomp.png')
    if (build/'assets').is_dir():
        shutil.copytree(build/'assets',out/'assets')
    (out/'bios').mkdir()
    shutil.copy2(bios,out/'bios/openbios.bin')
    if (build/'bios/OpenBIOS.LICENSE').is_file():
        shutil.copy2(build/'bios/OpenBIOS.LICENSE',out/'bios/OpenBIOS.LICENSE')
    shutil.copytree(build/'mods/bundled',out/'mods/bundled')
    config=(root/'game.toml').read_text(encoding='utf-8')
    for section,key,value in (
        ('game','exe',str((root/native['game']['exe']).resolve())),
        ('game','disc',str(disc)),
        ('recompiler','seeds',str((root/native['recompiler']['seeds']).resolve())),
        ('recompiler','bios_config',str((root/native['recompiler']['bios_config']).resolve())),
        ('runtime','memcard_dir',str(out/'saves')),
        ('video','renderer','opengl'),('video','aspect_ratio','4:3'),
        ('runtime','overlay_autocompile_cmd',
         f'py -3 "{root}/psxrecomp-v4/tools/compile_overlays.py" --game-toml "{out}/game.toml" '
         f'--recompiler "{root}/psxrecomp-v4/recompiler/build/psxrecomp-game.exe" '
         f'--runtime-include "{root}/psxrecomp-v4/runtime/include" --cps'),
    ):
        config=change_config(config,section,key,value)
    if args.no_overlay_cache:
        config=change_config(config,'runtime','overlay_cache',False)
        config=change_config(config,'runtime','overlay_autocompile_cmd','')
    (out/'game.toml').write_text(config,encoding='utf-8')
    if not args.no_overlay_cache:
        cg_tag=subprocess.check_output([sys.executable,str(stage_tool),'cg-tag',
            '--runtime-include',str(root/'psxrecomp-v4/runtime/include'),
            '--recompiler',str(root/'psxrecomp-v4/recompiler/build/psxrecomp-game.exe'),
            '--game-toml',str(out/'game.toml'),'--flavor-from-build',str(build),
            '--runtime-target','psx-runtime'],text=True).strip()
        subprocess.run([sys.executable,str(stage_tool),'stage-cache',
            '--game-id',native['game']['id'],'--cache-src-root',str(cache_source),
            '--stage',str(out),'--cg-tag',cg_tag],check=True)
    (out/'mods/state.toml').write_text('format_version = 2\n\n[[package]]\nid="mmx5.coop"\n'
        '\n[[feature]]\npackage_id="mmx5.coop"\nid="coop"\nenabled=true\n'
        f'[feature.values]\nkeep_zero="false"\nlocal_cameras="{args.local_cameras}"\n'
        f'door_players="{args.door_players}"\n',encoding='utf-8')
    (out/'settings.toml').write_text('[video]\nfullscreen = 0\n\n[launcher]\nskip_launcher = false\n',encoding='utf-8')
    debug_args='\n$coopArgs += @("--debug-port", [string]$coopDebugPort)' if args.debug_test_build else ''
    debug_setup='''$coopListener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Loopback, 0)
$coopListener.Start()
$coopDebugPort = $coopListener.LocalEndpoint.Port
$coopListener.Stop()
''' if args.debug_test_build else ''
    (out/'Play X5 Co-op.ps1').write_text('''param([switch]$VerifyOnly)
$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
$coopScratch = Join-Path $PSScriptRoot 'scratch'
New-Item -ItemType Directory -Force -Path (Join-Path $coopScratch 'tmp'),(Join-Path $coopScratch 'localappdata'),(Join-Path $coopScratch 'appdata'),(Join-Path $coopScratch 'cache'),(Join-Path $coopScratch 'data'),(Join-Path $coopScratch 'config'),(Join-Path $coopScratch 'shaders') | Out-Null
$env:TEMP = Join-Path $coopScratch 'tmp'
$env:TMP = $env:TEMP
$env:TMPDIR = $env:TEMP
$env:LOCALAPPDATA = Join-Path $coopScratch 'localappdata'
$env:APPDATA = Join-Path $coopScratch 'appdata'
$env:XDG_CACHE_HOME = Join-Path $coopScratch 'cache'
$env:XDG_DATA_HOME = Join-Path $coopScratch 'data'
$env:XDG_CONFIG_HOME = Join-Path $coopScratch 'config'
$env:PYTHONPYCACHEPREFIX = Join-Path $coopScratch 'cache/python'
$env:MESA_SHADER_CACHE_DIR = Join-Path $coopScratch 'shaders'
$env:__GL_SHADER_DISK_CACHE_PATH = Join-Path $coopScratch 'shaders'
'''+debug_setup+'''foreach ($coopFlag in @('SDL_AUDIODRIVER','SDL_AUDIO_DRIVER','PSX_HEADLESS','PSX_HEADLESS_OPENGL','PSX_NETPLAY','PSX_DISPLAY_RING')) {
    Remove-Item -LiteralPath ('Env:' + $coopFlag) -ErrorAction SilentlyContinue
}
if (Test-Path -LiteralPath 'C:/msys64/mingw64/bin' -PathType Container) {
    $env:PATH = 'C:/msys64/mingw64/bin;' + $env:PATH
}
$coopExe = Join-Path $PSScriptRoot 'mmx5-runtime.exe'
$coopArgs = @('--launcher','--game',(Join-Path $PSScriptRoot 'game.toml'),'--bios',(Join-Path $PSScriptRoot 'bios/openbios.bin'),'--renderer','opengl','--memcard-dir',(Join-Path $PSScriptRoot 'saves'))'''+debug_args+'''
$coopLaunch = [pscustomobject]@{executable=$coopExe;arguments=$coopArgs;working_directory=$PSScriptRoot;temp=$env:TEMP;localappdata=$env:LOCALAPPDATA;appdata=$env:APPDATA;shader_cache=$env:MESA_SHADER_CACHE_DIR;audio_driver=$env:SDL_AUDIODRIVER}
if ($VerifyOnly) {
    $coopLaunch | ConvertTo-Json -Depth 4
    exit 0
}
$coopLaunch | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'owner-launch.json') -Encoding UTF8
& $coopExe @coopArgs
exit $LASTEXITCODE
''',encoding='utf-8')
    (out/'Play X5 Co-op.cmd').write_text('@echo off\n"%SystemRoot%\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "%~dp0Play X5 Co-op.ps1"\n',encoding='utf-8')
    (out/'README.txt').write_text('''Mega Man X5 local co-op playtest

Run Play X5 Co-op.cmd. Start in a normal window with normal audio.
The launcher exposes P1/P2 controller bindings; choose a separate pad for each.
Co-op is enabled; choose X or Zero through the original game menus.
P2 is the counterpart (base X when P1 is Zero). Each seat opens its own
native menu with Start. P2 holds Select for 90 gameplay updates to leave,
then taps Select near a grounded living P1 to return. A fallen actor stays
out until a team retry/new stage. Shared retry count and stored tanks.
The counterpart uses its native teleport arrival on stage entry/return and
native departure on withdrawal, including Zero's character animations.
Split panes show their local player's native life meter. Ride Armor belongs to P1; P2 stays
on foot. Both players use Ride Chasers in the forced car sequence. Select
departure is locked there; one death leaves the survivor playing, and two
deaths consume one shared retry. Full road traversal still needs playtesting.
Zero's mounted rider pose and facing have been corrected.

This directory owns its settings and memory cards. The original disc/exe
remain in the worktree. Keep other enhancement mods off for this baseline.

Shared-screen netplay is optional: use Host / Join in the launcher when
both peers have this same build, game region and co-op package enabled.
The trusted co-op profile uses one shared 4:3 camera and delay sync. Offline
play needs no network session. For independent displays, select Mods ->
X + Zero Co-op -> Netplay cameras -> Split before hosting. The host chooses
for both peers; direct LAN launches need the same setting on each peer.
Each peer binds its local controller as P1; the session assigns its seat.
Each online player then gets its own 4:3 camera and native HUD. Offline
uses the Local co-op display option. This payload starts with side-by-side
local cameras unless prepared with --local-cameras unified. Native menus and
story scenes retain shared presentation.

Diagnostics and Alia hint suppression are enabled by default. Diagnostics
record accepted inputs, starting cards and mode/build history beneath saves.
Please retain that directory with a bug report and describe the stage,
character/seat, last action, and what you expected to happen.

This is an interim local candidate, not full X4 parity or a qualified campaign.
Doors accept either player by default; the Both-player option requires the pair.
Offline co-op save states/rewind have bounded local qualification. Broad
boss/reward/ending progression and network coverage remain open. Keep native
memory-card saves for ongoing campaigns. Progress comes from the supplied
native card, if present. See docs/COOP_PARITY_BURNDOWN.md for bounded evidence.
''',encoding='utf-8')
    if args.door_players=='both':
        with (out/'README.txt').open('a',encoding='utf-8') as note:
            note.write('\nThis QA profile starts with Both-player doors; the product default remains Either.\n')
    if card is not None:
        # The launcher isolates OpenBIOS cards; CLI tests can use the legacy
        # root. Seed both in this new directory before either is opened.
        for location in (out/'saves',out/'saves/openbios'):
            location.mkdir(parents=True,exist_ok=True)
            (location/'card1.mcd').write_bytes(card)
        with (out/'README.txt').open('a',encoding='utf-8') as note:
            note.write('\nNative save included: choose CONTINUE, Memory Card 1,\n'
                       'then choose the desired native save slot and confirm Yes.\n'
                       f'Original save source: {args.save_source_url or args.save_source_note}\n')
    branch=subprocess.check_output(['git','branch','--show-current'],cwd=root,text=True).strip()
    revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
    framework_revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root/'psxrecomp-v4',text=True).strip()
    manifest=dict(branch=branch,source_commit=revision,executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
                  bios_sha256=hashlib.sha256(bios.read_bytes()).hexdigest(),
                  framework_commit=framework_revision,netplay_cameras=['unified','split'],
                  default_camera=args.local_cameras,human_validated=False,source_docs=str(root/'docs/COOP_PARITY_BURNDOWN.md'),
                  controller_players=2,debug_test_build=args.debug_test_build,
                  overlay_cache=not args.no_overlay_cache,door_players=args.door_players,
                  lobby_version=version.read_text().strip() if version.is_file() else None,
                  qualification_cheats=bool(re.search(r'^MMX5_COOP_TEST_CHEATS:BOOL=ON$',cache,re.M)))
    (out/'docs').mkdir(exist_ok=True)
    shutil.copy2(root/'docs/COOP_PARITY_BURNDOWN.md',out/'docs/COOP_PARITY_BURNDOWN.md')
    for name in ('COOP_PLAYTESTING.md','COOP_HUMAN_SETUP_QUEUE.md'):
        if (root/'docs'/name).is_file():shutil.copy2(root/'docs'/name,out/'docs'/name)
    manifest['execution']=json.loads((out/'mmx5-runtime.execution.json').read_text(encoding='utf-8'))
    manifest['diagnostics_default_on']=True
    manifest['alia_suppression_default_on']=True
    manifest['forbidden_loading_packages_present']=False
    manifest['preparer_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    if args.debug_test_build:
        shutil.copy2(root/'docs/COOP_DEBUG_TESTING.md',out/'docs/COOP_DEBUG_TESTING.md')
        with (out/'README.txt').open('a',encoding='utf-8') as note:
            note.write('\nDebug test build: the recomp-ui Controller page exposes P1 and P2.\n'
                       'Use separate device bindings for local co-op. Host/Join and the\n'
                       'netplay lobby are available from the launcher; online peers each\n'
                       'bind their local device as P1 and choose their seat in the room.\n')
            if manifest['qualification_cheats']:
                note.write('Qualification cheats enabled; see docs/COOP_DEBUG_TESTING.md.\n'
                           'Use the same cheat-enabled build on both peers.\n')
    if card is not None:
        manifest['stage_access_card_sha256']=hashlib.sha256(card).hexdigest()
        manifest['save_source_url']=args.save_source_url
        manifest['save_source_note']=args.save_source_note
    (out/'CHECKPOINT.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    print(out/'Play X5 Co-op.ps1')


if __name__=='__main__':
    main()
