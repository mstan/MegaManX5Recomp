"""Stage a local unified co-op development build with private settings/cards.

Original game assets remain in the owner's checkout. Nothing is published.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tomllib
from test_mmx5_coop_netplay import change_config


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build',type=Path,default=Path('build-coop'))
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    root=Path(__file__).resolve().parent.parent
    build=args.build.resolve()
    out=args.out.resolve()
    if out.exists():
        parser.error('Use a fresh output directory to preserve previous playtest settings/cards')
    exe=build/'mmx5-runtime.exe'
    bios=build/'bios/openbios.bin'
    package=build/'mods/bundled/mmx5.coop/0.0.1/manifest.toml'
    for path in (exe,bios,package):
        if not path.is_file():parser.error(f'Missing built input: {path}')
    out.mkdir(parents=True)
    shutil.copy2(exe,out/exe.name)
    for dll in build.glob('*.dll'):
        if dll.name!='asset-research.dll':
            shutil.copy2(dll,out/dll.name)
    if (build/'psxrecomp.png').is_file():
        shutil.copy2(build/'psxrecomp.png',out/'psxrecomp.png')
    (out/'bios').mkdir()
    shutil.copy2(bios,out/'bios/openbios.bin')
    if (build/'bios/OpenBIOS.LICENSE').is_file():
        shutil.copy2(build/'bios/OpenBIOS.LICENSE',out/'bios/OpenBIOS.LICENSE')
    shutil.copytree(build/'mods/bundled',out/'mods/bundled')
    if (build/'cache').is_dir():
        shutil.copytree(build/'cache',out/'cache')
    config=(root/'game.toml').read_text(encoding='utf-8')
    native=tomllib.loads(config)
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
Life meters are stacked on the left. Ride Armor belongs to P1; P2 stays
on foot. Both players use Ride Chasers in the forced car sequence. Select
departure is locked there; one death leaves the survivor playing, and two
deaths consume one shared retry. Full road traversal still needs playtesting.

This directory owns its settings and memory cards. The original disc/exe
remain in the worktree. Keep other enhancement mods off for this baseline.

This is an early checkpoint, not a qualified full campaign. See the source
docs/COOP_SPIKE.md for tested paths and outstanding stage/vehicle/armor work.
Independent cameras and optional X-versus-Zero PvP remain deferred.
''',encoding='utf-8')
    branch=subprocess.check_output(['git','branch','--show-current'],cwd=root,text=True).strip()
    revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
    manifest=dict(branch=branch,source_commit=revision,executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
                  bios_sha256=hashlib.sha256(bios.read_bytes()).hexdigest(),
                  human_validated=False,source_docs=str(root/'docs/COOP_SPIKE.md'))
    (out/'CHECKPOINT.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    print(out/'Play X5 Co-op.ps1')


if __name__=='__main__':
    main()
