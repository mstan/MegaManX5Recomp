"""Cold-load a captured private menu before enrollment; use mapped save/rewind.

All output and temporary title settings must be on F:/G:. One owned process;
the caller stops the previous session first. Background audio is muted.
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
import traceback
import psutil
from coop_debug import Runtime, PLAY
from test_mmx5_coop_snapshot_runtime import require, state_operation
from test_mmx5_coop_scene_fatal_runtime import serial


def mapped_load(r, slot=0):
    """Load through the normal host slot panel and Cross, never a RAM flag."""
    def host(buttons):
        r.request('set_input',layer='host',buttons=hex(0xFFFF^buttons))
    def wait(test, label):
        deadline=time.monotonic()+15
        while not test():
            require(time.monotonic()<deadline,label);time.sleep(.03)
    r.request('clear_input');host(0);time.sleep(.15);host(0x801)
    try:
        wait(lambda:r.request('overlay_status')['save_menu_open'],'Mapped load panel did not open')
        host(0);time.sleep(.15)
        require(r.request('overlay_status')['save_slot']==slot,'Mapped slot focus differs from requested slot')
        generation=r.request('savestate_status')['generation']
        host(0x4000);time.sleep(.15);host(0)
        wait(lambda:r.request('savestate_status')['generation']>generation,'Mapped Cross load did not finish')
        result=r.request('savestate_status')
        require(result['last_ok']==1 and result['last_op']=='load' and result['last_slot']==slot,'Mapped Cross load rejected')
        return result
    finally:
        host(0);r.request('clear_input')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--parent',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True)
    p.add_argument('--keep-running',action='store_true')
    a=p.parse_args();parent=json.loads(a.parent.read_text());exe=Path(parent['command'][0]).resolve()
    require(hashlib.sha256(exe.read_bytes()).hexdigest()==parent['executable_sha256'],'Binary changed')
    require(not any(Path(proc.exe()).resolve()==exe for proc in psutil.process_iter() if proc.name().lower()=='mmx5-runtime.exe'), 'Stop the previous owned candidate before cold launch')
    require(a.out.resolve().drive.upper() in ('F:','G:'),'QA must use F:/G:')
    a.out.mkdir(parents=True,exist_ok=False);(a.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    build=exe.parent;state=build/'mods/state.toml';settings=build/'settings.toml'
    old_state=state.read_bytes() if state.exists() else None
    old_settings=settings.read_bytes() if settings.exists() else None
    report=dict(success=False,executable_sha256=parent['executable_sha256'],separate_etanks=True,samples=[],checks=[],operations=[],
                parent=str(a.parent),limits='Offline cold-load/mapped save/rewind only; no network or authored scene completion pass.')
    process=None;log=None

    def save():
        (a.out/'report.json').write_text(json.dumps(report,indent=2)+'\n')

    def wait(test,label,seconds=25):
        deadline=time.monotonic()+seconds
        while not test():
            require(process.poll() is None,'Owned game exited')
            require(time.monotonic()<deadline,label);time.sleep(.03)

    def observe(label):
        s=r.personal_effects();report['samples'].append(dict(label=label,personal=serial(s),play=r.read(PLAY,0x80).hex(),
                    menu_owner=r.read(r.diagnostic()+0x48,1)[0],rewind=r.request('rewind_status')));save();return s

    def host(buttons):
        r.request('set_input',layer='host',buttons=hex(0xFFFF^buttons))

    try:
        state.write_text('format_version=2\n[[feature]]\npackage_id="mmx5.coop"\nid="coop"\nenabled=true\n[feature.values]\ncameras="unified"\nlocal_cameras="unified"\ndoor_players="either"\nseparate_etanks="true"\n')
        settings.write_text('[video]\nrewind=true\nrewind_depth=50\nrewind_interval=8\n')
        saves=a.out/'saves';(saves/'openbios').mkdir(parents=True)
        source=Path(parent['command'][parent['command'].index('--memcard-dir')+1])/'openbios/state_8005894C_slot07.pst'
        shutil.copy2(source,saves/'openbios'/source.name)
        report['state_sha256']=hashlib.sha256(source.read_bytes()).hexdigest()
        command=parent['command'].copy()
        command[command.index('--memcard-dir')+1]=str(saves.resolve())
        with socket.socket() as reserve:
            reserve.bind(('127.0.0.1',0));port=reserve.getsockname()[1]
        command[command.index('--debug-port')+1]=str(port)
        env=os.environ.copy();env['SDL_AUDIODRIVER']='dummy';env['PATH']='C:/msys64/mingw64/bin;'+env['PATH']
        log=(a.out/'runtime.log').open('w')
        process=subprocess.Popen(command,cwd=build,env=env,stdout=log,stderr=subprocess.STDOUT,
                    creationflags=subprocess.CREATE_NO_WINDOW if os.name=='nt' else 0)
        report.update(pid=process.pid,port=port,command=command);save()
        print('Owned cold X5 process',process.pid,'port',port,flush=True)
        r=Runtime(port)
        deadline=time.monotonic()+90
        while True:
            try:r.request('frame');break
            except OSError:
                require(process.poll() is None and time.monotonic()<deadline,'Cold startup failed');time.sleep(.1)
        # No Game Start/character selection or native co-op enrollment first.
        initial=r.state();report['pre_load']=initial
        require(initial['enrolled']==0 and initial['banks']==0,'Cold load already warmed the character cache')
        report['operations'].append(state_operation(r,'load',7))
        wait(lambda:r.read(PLAY+1,1)==b'\x02','Cold saved menu not restored')
        restored=observe('cold-private-menu-restored')
        require(r.read(r.diagnostic()+0x48,1)[0]==2 and [b[0x5C] for b in restored['bodies']]==[13,11], 'Cold menu owner/private actors incorrect')
        require(r.request('rewind_status')['enabled'],'Rewind setting not applied')
        r.screenshot(a.out/'cold-menu.png')
        # The normal host shortcut opens the slot UI; Square must perform save.
        r.request('clear_input');host(0);time.sleep(.15);host(0x801)
        wait(lambda:r.request('overlay_status')['save_menu_open'],'Mapped save panel did not open')
        host(0);time.sleep(.15);generation=r.request('savestate_status')['generation']
        host(0x8000);time.sleep(.15);host(0)
        wait(lambda:r.request('savestate_status')['generation']>generation,'Mapped Square save did not finish')
        mapped=r.request('savestate_status');require(mapped['last_ok']==1 and mapped['last_op']=='save','Mapped Square save failed')
        report['operations'].append(mapped)
        # A save may close the panel; otherwise normal Circle closes it.
        if r.request('overlay_status')['save_menu_open']:
            host(0x2000);time.sleep(.15);host(0)
        r.request('clear_input');r.resume_native_pause();r.release()
        # Keep enough menu samples in the ring, then leave/move and rewind back.
        r.tap(8,.15,1)
        wait(lambda:r.read(PLAY+1,1)==b'\x02','Guest native menu did not reopen')
        time.sleep(1.5);menu=observe('rewind-menu-baseline')
        r.resume_native_pause();r.release();r.input(0x20,0);r.input(0x20,1);time.sleep(.30);r.release()
        report['before_rewind']=serial(r.personal_effects())
        require(r.request('rewind_status')['snaps']>=5,'No rewind history captured')
        r.request('clear_input');host(0);time.sleep(.15);host(5)
        wait(lambda:r.request('rewind_status')['open'],'Mapped rewind did not open')
        host(0);time.sleep(.15);r.screenshot(a.out/'rewind-filmstrip.png')
        # Move several samples into the earlier paused menu, then accept Cross.
        # Hold through the filmstrip repeat gate; short pulses can be missed
        # while composed thumbnails are presented. This seeks about13 samples.
        host(0x80);time.sleep(1.5);host(0);time.sleep(.15)
        r.screenshot(a.out/'rewind-selected.png')
        host(0x4000);time.sleep(.15);host(0)
        wait(lambda:not r.request('rewind_status')['open'],'Rewind accept did not close')
        wait(lambda:r.read(PLAY+1,1)==b'\x02','Rewind did not reach earlier owned menu')
        rewound=observe('mapped-rewind-restored-menu')
        require(r.read(r.diagnostic()+0x48,1)[0]==2,'Rewind lost guest menu owner')
        require([b[0x5C] for b in rewound['bodies']]==[b[0x5C] for b in menu['bodies']],'Rewind lost private HP')
        r.screenshot(a.out/'rewound-menu.png');r.request('clear_input');r.resume_native_pause();r.release()
        # B deliberately froze Zero's native slash. Let the restored attack
        # finish and the frontend load-input guard release before testing move.
        wait(lambda:all(b[4:6]==b'\x01\x02' for b in r.players()),'Restored native attack did not finish')
        before=r.players()[1][8:12];r.input(0x20,1)
        try:wait(lambda:r.players()[1][8:12]!=before,'Control did not recover after rewind',3)
        finally:r.release()
        observe('rewind-resumed-world');r.screenshot(a.out/'rewind-resumed.png')
        report['checks']=['Cold state Load before co-op enrollment restores private actors/retained P2 menu',
                          'Normal host save-panel shortcut and Square save',
                          'Normal host rewind shortcut/filmstrip/selection/Cross accepts earlier P2-owned menu and resumes control']
        report['success']=True;print('PASS cold state and mapped rewind/save',flush=True)
    except Exception:
        report['error']=traceback.format_exc();print(report['error'],flush=True)
    finally:
        for path,old in ((state,old_state),(settings,old_settings)):
            if old is None:path.unlink(missing_ok=True)
            else:path.write_bytes(old)
        if process and not a.keep_running:
            try:r.request('quit');process.wait(timeout=10)
            except Exception:process.terminate();process.wait(timeout=10)
        if log:log.close()
        save()
    return 0 if report['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())
