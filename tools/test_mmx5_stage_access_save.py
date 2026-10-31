"""Load a prepared X5 card through native menus in an owned offline process.

No gameplay RAM writes or savestates. This validates save recognition and
stage selection, not boss traversal or owner acceptance.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import socket
import subprocess
import time
from coop_debug import Runtime,PLAY


def check_stages(r, out, report):
    # BIN20 EF858 owns the eight-way cursor at PLAY+28. Its original
    # F5050 table maps the selected portrait to the native stage number.
    deadline=time.monotonic()+90
    while r.read(PLAY,3)!=b'\x04\x04\x00':
        if time.monotonic()>deadline:raise TimeoutError('Interactive Maverick selector')
        r.tap(0x4000,.04);time.sleep(.12)
    r.input(0)
    table=r.read(0x800f5050,8)
    if set(table)!=set(range(1,9)):raise AssertionError('Unexpected native Maverick stage table')
    play=r.read(PLAY,0x1c0)
    if not (play[0x79]<5 or play[0xcb]&0x80):raise AssertionError('Zero unavailable in loaded roster')
    if play[0x4c]:raise AssertionError('Test save has defeated bosses')
    if play[0x28]!=0:raise AssertionError('Expected original initial portrait')
    choices=[]
    # Down the left column, across, up the right column, then return left.
    for button,expected in ((0,0),(0x40,1),(0x40,2),(0x40,3),(0x20,7),
                            (0x10,6),(0x10,5),(0x10,4),(0x80,0)):
        if button:r.tap(button,.055);time.sleep(.12)
        cursor=r.read(PLAY+0x28,1)[0]
        if cursor!=expected:raise AssertionError(f'Native cursor {cursor}, expected {expected}')
        choices.append(dict(cursor=cursor,stage=table[cursor]))
        r.screenshot(out/f'portrait-{cursor}.png')
    if {c['stage'] for c in choices}!=set(range(1,9)):
        raise AssertionError('Not all eight native stage choices visited')
    report['stage_choices']=choices;report['zero_available']=True
    r.tap(0x4000,.04)
    deadline=time.monotonic()+90
    while True:
        play=r.read(PLAY,14)
        if play[0]==10 and play[1]==0:
            if r.state()['ready']:
                if r.lifecycle()[:2]==(0,0):break
            else:
                # A Maverick stage can open with native Alia dialogue before
                # the player-control/enrollment boundary.
                r.tap(0x4000,.04)
        elif play[0] in (4,5,6):
            r.tap(0x4000,.04)
        if time.monotonic()>deadline:raise TimeoutError('Native selected-stage entry and counterpart arrival')
        time.sleep(.08)
    r.input(0)
    if r.read(PLAY+12,1)!=table[:1]:raise AssertionError('Entered wrong selected stage')
    report['entered_stage']=table[0]
    report['actors']=[body[2] for body in r.players()]
    report['lifecycle']=r.lifecycle()
    r.screenshot(out/'native-boss-stage.png')
    # Park only this owned process to keep its entry capture reproducible.
    r.tap(8,.04)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','game','disc','bios','card','out'):p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--keep-running',action='store_true')
    p.add_argument('--title-only',action='store_true',help='Park an owned run before menu input for native menu research')
    p.add_argument('--separate-etanks',action='store_true',help='Enable optional private E contents for this native-card session')
    args=p.parse_args()
    args.out.mkdir(parents=True,exist_ok=False)
    saves=args.out/'saves';saves.mkdir()
    shutil.copy2(args.card,saves/'card1.mcd')
    with socket.socket() as s:s.bind(('127.0.0.1',0));port=s.getsockname()[1]
    env=os.environ.copy();env['PATH']='C:/msys64/mingw64/bin;'+env['PATH']
    env['PSX_DEV_INPUT']='0';env['SDL_AUDIODRIVER']='dummy'
    state=args.exe.resolve().parent/'mods/state.toml'
    previous=state.read_bytes() if state.exists() else None
    selection='format_version=2\n[[package]]\nid="mmx5.coop"\n[[feature]]\npackage_id="mmx5.coop"\nid="coop"\nenabled=true\n'
    if args.separate_etanks:selection+='[feature.values]\nseparate_etanks="true"\n'
    state.write_text(selection)
    log=(args.out/'runtime.log').open('w')
    command=[str(args.exe.resolve()),'--hidden-window','--renderer','opengl','--no-launcher',
             '--game',str(args.game.resolve()),'--disc',str(args.disc.resolve()),
             '--bios',str(args.bios.resolve()),'--memcard-dir',str(saves.resolve()),'--debug-port',str(port)]
    process=subprocess.Popen(command,cwd=args.exe.resolve().parent,env=env,stdout=log,stderr=subprocess.STDOUT,
                             creationflags=subprocess.CREATE_NO_WINDOW if os.name=='nt' else 0)
    r=Runtime(port);report=dict(pid=process.pid,port=port,command=command,human_validation=False,success=False,
                              harness_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                              executable_sha256=hashlib.sha256(args.exe.read_bytes()).hexdigest(),
                              card_sha256=hashlib.sha256(args.card.read_bytes()).hexdigest())
    report['separate_etanks']=args.separate_etanks
    print('Owned save QA process',process.pid,'port',port,flush=True)
    try:
        deadline=time.monotonic()+30
        while True:
            try:r.request('frame');break
            except OSError:
                if time.monotonic()>deadline:raise TimeoutError('Debug boot')
                time.sleep(.15)
        r.input(0,0);r.input(0,1)
        if args.title_only:
            report['success']=True
            return
        deadline=time.monotonic()+60
        # Original pre-game controller is separate from PLAY. Its state6
        # owns the Game Start / Continue menu; PLAY mode1 is player select.
        while r.read(0x800d1f28,1)!=b'\x06':
            if time.monotonic()>deadline:raise TimeoutError('Native title menu')
            r.tap(8,.015);time.sleep(.04)
        r.input(0)
        while r.read(0x800d1f29,1)!=b'\x01' or r.read(0x8009a424,1)!=b'\x00':
            if time.monotonic()>deadline:raise TimeoutError('Native title fade')
            time.sleep(.03)
        r.screenshot(args.out/'title.png')
        r.tap(0x40,.08);time.sleep(.1)
        if r.read(0x800d1f2a,1)!=b'\x01':raise AssertionError('Continue cursor did not move')
        r.screenshot(args.out/'load-selected.png')
        r.tap(0x4000,.08)
        deadline=time.monotonic()+60
        trace=[]
        while True:
            play=r.read(PLAY,0x1c0)
            pair=play[:4].hex()
            if not trace or trace[-1]['mode']!=pair:
                trace.append(dict(mode=pair,stage=list(play[12:14])))
                print('Native save menu',trace[-1],flush=True)
                r.screenshot(args.out/f'load-{len(trace):02}.png')
            if play[0]==4 and play[12:14]==b'\x0d\x00':
                break
            if time.monotonic()>deadline:raise TimeoutError('Native Load Game did not reach stage selection')
            # Original confirmation defaults to Load Cancel (PLAY+A == 1).
            if play[:4]==b'\x00\x03\x00\x04' and play[0xa]:
                r.tap(0x80,.08)
            r.tap(0x4000,.08);time.sleep(.2)
        r.input(0);time.sleep(.3)
        r.screenshot(args.out/'stage-select.png')
        report['trace']=trace
        check_stages(r,args.out,report)
        report['play']=r.read(PLAY,0x1c0).hex()
        report['success']=True
        print('PASS prepared card: native Load Game, all eight choices, both characters and native boss-stage entry',flush=True)
    except Exception as e:
        report['error']=str(e)
        try:report['play']=r.read(PLAY,0x1c0).hex();r.screenshot(args.out/'failure.png')
        except Exception:pass
        raise
    finally:
        (args.out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
        if not args.keep_running:
            try:r.request('quit')
            except Exception:pass
            try:process.wait(timeout=8)
            except subprocess.TimeoutExpired:process.terminate();process.wait(timeout=8)
        if previous is None:state.unlink(missing_ok=True)
        else:state.write_bytes(previous)
        log.close()


if __name__=='__main__':main()
