"""Stage a local unified co-op development build with private settings/cards.

Original game assets remain in the owner's checkout. Nothing is published.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tomllib
from test_mmx5_coop_netplay import change_config


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build',type=Path,default=Path('build-coop'))
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--save-card','--stage-access-card',dest='stage_access_card',type=Path,
                        help='Seed a fresh playtest with an unmodified downloaded 128KB PS1 card')
    parser.add_argument('--save-source-url',help='Online source of the downloaded save card')
    parser.add_argument('--debug-test-build',action='store_true',
                        help='Require both launcher controller seats, debug tools and the netplay lobby')
    args=parser.parse_args()
    root=Path(__file__).resolve().parent.parent
    build=args.build.resolve()
    out=args.out.resolve()
    if out.exists():
        parser.error('Use a fresh output directory to preserve previous playtest settings/cards')
    card=args.stage_access_card.read_bytes() if args.stage_access_card else None
    if card is not None and (len(card)!=131072 or card[:2]!=b'MC'):
        parser.error('Downloaded save card must be a native 128KB PS1 card')
    if card is not None:
        if not args.save_source_url or not args.save_source_url.startswith(('https://','http://')):
            parser.error('A downloaded card requires --save-source-url')
        if hashlib.sha256(card).hexdigest()=='4a173ef3b348f61073a89b8e416687f556b44b7475ef60c0e3564a0ac093f3e9':
            parser.error('The synthetic stage-access card is retired; use a downloaded save')
    exe=build/'mmx5-runtime.exe'
    bios=build/'bios/openbios.bin'
    package=build/'mods/bundled/mmx5.coop/0.0.1/manifest.toml'
    for path in (exe,bios,package):
        if not path.is_file():parser.error(f'Missing built input: {path}')
    native=tomllib.loads((root/'game.toml').read_text(encoding='utf-8'))
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
    out.mkdir(parents=True)
    shutil.copy2(exe,out/exe.name)
    version=build/'psx_game_version.txt'
    if version.is_file():shutil.copy2(version,out/version.name)
    for dll in build.glob('*.dll'):
        if dll.name!='asset-research.dll':
            shutil.copy2(dll,out/dll.name)
    if (build/'psxrecomp.png').is_file():
        shutil.copy2(build/'psxrecomp.png',out/'psxrecomp.png')
    if (build/'assets').is_dir():
        shutil.copytree(build/'assets',out/'assets')
    (out/'bios').mkdir()
    shutil.copy2(bios,out/'bios/openbios.bin')
    if (build/'bios/OpenBIOS.LICENSE').is_file():
        shutil.copy2(build/'bios/OpenBIOS.LICENSE',out/'bios/OpenBIOS.LICENSE')
    shutil.copytree(build/'mods/bundled',out/'mods/bundled')
    if (build/'cache').is_dir():
        shutil.copytree(build/'cache',out/'cache')
    config=(root/'game.toml').read_text(encoding='utf-8')
    for section,key,value in (
        ('game','exe',str((root/native['game']['exe']).resolve())),
        ('game','disc',str((root/native['game']['disc']).resolve())),
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
    (out/'game.toml').write_text(config,encoding='utf-8')
    (out/'mods/state.toml').write_text('format_version = 2\n\n[[package]]\nid="mmx5.coop"\n'
        '\n[[feature]]\npackage_id="mmx5.coop"\nid="coop"\nenabled=true\n'
        '[feature.values]\nkeep_zero="false"\n',encoding='utf-8')
    (out/'Play X5 Co-op.ps1').write_text('''$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
$coopListener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Loopback, 0)
$coopListener.Start()
$coopDebugPort = $coopListener.LocalEndpoint.Port
$coopListener.Stop()
if (Test-Path -LiteralPath 'C:/msys64/mingw64/bin' -PathType Container) {
    $env:PATH = 'C:/msys64/mingw64/bin;' + $env:PATH
}
& (Join-Path $PSScriptRoot 'mmx5-runtime.exe') --game (Join-Path $PSScriptRoot 'game.toml') --bios (Join-Path $PSScriptRoot 'bios/openbios.bin') --renderer opengl --memcard-dir (Join-Path $PSScriptRoot 'saves') --debug-port $coopDebugPort
exit $LASTEXITCODE
''',encoding='utf-8')
    (out/'README.txt').write_text('''Mega Man X5 unified co-op development checkpoint

Run Play X5 Co-op.ps1. The launcher exposes P1/P2 controller bindings.
Co-op is enabled; choose X or Zero through the original game menus.
P2 is the counterpart (base X when P1 is Zero). Each seat opens its own
native menu with Start. P2 holds Select for 90 gameplay updates to leave,
then taps Select near a grounded living P1 to return. A fallen actor stays
out until a team retry/new stage. Shared retry count and stored tanks.
The counterpart uses its native teleport arrival on stage entry/return and
native departure on withdrawal, including Zero's character animations.
Life meters are stacked on the left. Ride Armor belongs to P1; P2 stays
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
always uses Unified. Native menus and story scenes retain shared presentation.

This is an early checkpoint, not a qualified full campaign. See the source
docs/COOP_SPIKE.md for tested paths and outstanding stage/vehicle/armor work.
Independent online displays are a new playtest candidate. Optional story PvP
remains deferred. Full room/boss/script-handoff qualification is still pending.
''',encoding='utf-8')
    if card is not None:
        # The launcher isolates OpenBIOS cards; CLI tests can use the legacy
        # root. Seed both in this new directory before either is opened.
        for location in (out/'saves',out/'saves/openbios'):
            location.mkdir(parents=True,exist_ok=True)
            (location/'card1.mcd').write_bytes(card)
        with (out/'README.txt').open('a',encoding='utf-8') as note:
            note.write('\nDownloaded save included: choose CONTINUE, Memory Card 1,\n'
                       'then choose the desired native save slot and confirm Yes.\n'
                       f'Original save source: {args.save_source_url}\n')
    branch=subprocess.check_output(['git','branch','--show-current'],cwd=root,text=True).strip()
    revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
    framework_revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root/'psxrecomp-v4',text=True).strip()
    manifest=dict(branch=branch,source_commit=revision,executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
                  bios_sha256=hashlib.sha256(bios.read_bytes()).hexdigest(),
                  framework_commit=framework_revision,netplay_cameras=['unified','split'],
                  default_camera='unified',human_validated=False,source_docs=str(root/'docs/COOP_SPIKE.md'),
                  controller_players=2,debug_test_build=args.debug_test_build,
                  lobby_version=version.read_text().strip() if version.is_file() else None,
                  qualification_cheats=bool(re.search(r'^MMX5_COOP_TEST_CHEATS:BOOL=ON$',cache,re.M)))
    if args.debug_test_build:
        (out/'docs').mkdir()
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
    (out/'CHECKPOINT.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    print(out/'Play X5 Co-op.ps1')


if __name__=='__main__':
    main()
