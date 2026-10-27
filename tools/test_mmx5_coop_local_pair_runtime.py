"""Qualify private local cameras, reversible draws and canvas transitions.

Owned offline session only. Position/contact protection bypass traversal;
this checks presentation and update boundaries, not campaign mechanics.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time
import traceback

from PIL import Image
import psutil
from coop_debug import Runtime,PLAY
from audit_mmx5_coop_locations import snapshot,counter,wait_ready


def require(value,reason):
    if not value:raise AssertionError(reason)


def wait(predicate,reason,seconds=25):
    deadline=time.monotonic()+seconds
    while not predicate():
        require(time.monotonic()<deadline,reason);time.sleep(.04)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    session=json.loads(args.session.read_text())
    executable=Path(session['command'][0]).resolve()
    require(Path(psutil.Process(session['pid']).exe()).resolve()==executable,'Not the owned process')
    require(hashlib.sha256(executable.read_bytes()).hexdigest()==session['executable_sha256'],'Owned binary changed')
    require(session['local_cameras']=='split' and session['render_verify'],'Use local Split and restoration verification')
    args.out.mkdir(parents=True,exist_ok=False)
    r=Runtime(session['port'])
    require(not r.request('netplay_status')['active'],'Offline local pair required')
    report=dict(success=False,human_validation=False,executable_sha256=session['executable_sha256'],captures={},
                bypassed_prerequisites=['original stage1/area0/checkpoint1 load (midboss bypassed)',
                                       'P2 positioned at checkpoint2 native settled coordinates',
                                       'HP refill and native contact grace held'])
    def capture(name):
        path=args.out/(name+'.png');r.screenshot(path)
        with Image.open(path) as picture:
            size=picture.size
            pane_hashes=[hashlib.sha256(picture.crop((seat*size[0]//2,64,(seat+1)*size[0]//2,size[1])).tobytes()).hexdigest()
                         for seat in (0,1)]
        report['captures'][name]=dict(size=size,composed_pane_hashes=pane_hashes,
                                     pair=r.request('stereo_stats'),state=snapshot(r))
        return size
    def cameras():
        data=r.read(r.diagnostic()+0x710,0x1F8)
        return [list(struct.unpack_from('<h',data,seat*0xFC+10))+list(struct.unpack_from('<h',data,seat*0xFC+14))
                for seat in (0,1)]
    try:
        r.resume_native_pause();r.release();r.fixture(11,hp=3);r.fixture(12,hp=3)
        previous=counter(r,'mmx5.coop.enrolled')
        r.load_location(1,0,2);wait_ready(r,previous);r.release()
        wait(lambda:r.players()[1][0x70]&8,'Native checkpoint2 arrival did not settle')
        landing=struct.unpack_from('<ii',r.players()[1],8)
        report['native_checkpoint2_landing']=snapshot(r)
        previous=counter(r,'mmx5.coop.enrolled')
        r.load_location(1,0,1);wait_ready(r,previous);r.release()
        require(counter(r,'mmx5.renderer.tiles')>0,'Derived authored renderer did not produce terrain')
        r.teleport(1,landing[0]/65536,landing[1]/65536)
        wait(lambda:abs(cameras()[0][0]-cameras()[1][0])>800,'Independent cameras did not separate')
        def visible():
            require(r.lifecycle()[:2]==(0,0),'A player fell while its native camera settled')
            views=cameras()
            for seat,body in enumerate(r.players()):
                x,y=struct.unpack_from('<ii',body,8)
                if not (0<=x/65536-views[seat][0]<=320 and 0<=y/65536-views[seat][1]<=240):return False
            return True
        wait(visible,'A hero remained outside its own camera after native camera settling',45)
        before=r.state()
        wait(lambda:r.state()['world_ticks']>=before['world_ticks']+24,'World stopped during private rendering')
        after=r.state()
        require(after['ticks']-before['ticks']==after['world_ticks']-before['world_ticks'],'Private draws advanced shared simulation')
        require(r.lifecycle()[:2]==(0,0),'A player fell during the distant-view setup')
        report['cameras']=cameras();report['tick_check']=[before,after]
        for seat,body in enumerate(r.players()):
            x,y=struct.unpack_from('<ii',body,8);view=cameras()[seat]
            require(0<=x/65536-view[0]<=320 and 0<=y/65536-view[1]<=240,
                    f'P{seat+1} is outside its own camera after settling')
        report['controls']=[]
        for seat in (0,1):
            bodies=r.players();tick=r.state()['world_ticks'];r.input(0x20,seat)
            wait(lambda:r.state()['world_ticks']>=tick+8,'Independent input stopped the world')
            r.input(0,seat);moved=r.players()
            require(struct.unpack_from('<i',moved[seat],8)[0]>struct.unpack_from('<i',bodies[seat],8)[0],
                    f'P{seat+1} cannot move in local Split')
            require(struct.unpack_from('<i',moved[seat^1],8)[0]==struct.unpack_from('<i',bodies[seat^1],8)[0],
                    f'P{seat+1} moved the other actor')
            report['controls'].append(dict(seat=seat,before=[b.hex() for b in bodies],after=[b.hex() for b in moved]))
        capture('distant')
        stats=r.request('stereo_stats')
        require(stats['pairs']>0 and stats['eye_cycles'][0]==stats['eye_cycles'][1],'Panes came from different world boundaries')
        # eye_hashes are restored machine-state hashes, not image hashes.
        require(len(set(report['captures']['distant']['composed_pane_hashes']))==2,'Distant composed panes are identical')
        for width,height in ((1600,600),(1024,900),(1280,720)):
            r.request('window_size',w=width,h=height)
            wait(lambda:capture('resize-'+str(width))==(width,height),'Host resize did not reach composed output')
        r.tap(8,.15,1)
        wait(lambda:r.read(PLAY+1,1)==b'\x02','P2 pause did not open')
        wait(lambda:r.read(0x800D4F44,1)==b'\x01','Native pause fade did not finish')
        pause_tick=r.state()['world_ticks'];capture('p2-pause')
        time.sleep(.2)
        require(r.state()['world_ticks']==pause_tick,'Pause advanced the shared world')
        paused_pairs=counter(r,'mmx5.coop.local-pair-committed')
        r.resume_native_pause()
        wait(lambda:counter(r,'mmx5.coop.local-pair-committed')>paused_pairs,'Private pairs did not resume')
        capture('resumed')
        report['render_stats']=r.request('render_pass_stats')
        require(report['render_stats']['verify_checks']>0,'Restoration verifier did not run')
        for key in ('verify_mismatch','watchdog','device_reads','vram_leaks'):
            require(not report['render_stats'][key],f'Private rendering fault: {key}')
        require(not counter(r,'mmx5.coop.local-view-state-leak'),'Host simulation context leaked from a pane')
        require(not r.state()['failed'],'Co-op renderer failed')
        report['success']=True
        print('PASS local pair: distant cameras, own HUD, resize/pause, once-per-world and restoration checks',flush=True)
    except Exception:
        report['error']=traceback.format_exc();capture('failure');raise
    finally:
        try:
            r.release();r.resume_native_pause();r.fixture(12,hp=0)
            if r.read(PLAY+1,1)==b'\x00':r.tap(8,.10)
        finally:
            (args.out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')


if __name__=='__main__':main()
