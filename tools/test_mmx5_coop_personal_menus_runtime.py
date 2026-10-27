"""One owned offline X/P1-Zero/P2 paired-menu regression, no game launcher.

Use one earned-weapon profile or disclose --weapon-eligibility-mask. Optional --prepare bypasses only the
route to original raw1/0/CP0. HP/ammo/E/W supplies are disclosed setup; all
cursor changes, selections, healing, pause/resume and snapshot ops are native.
The owned healthy game remains running on either success or observer failure.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import time
import traceback


def require(value,message):
    if not value: raise AssertionError(message)


def write_setup_ram(runtime,address,data,attempts=5):
    """Byte-only setup; retry only the same allowed bytes after view rollback."""
    allowed=((0x800D1C76,0x800D1C80),(0x8009A169,0x8009A16A))
    require(data and any(lo<=address and address+len(data)<=hi for lo,hi in allowed),
            'RAM setup outside E/W/acquisition or canonical X eligibility')
    require(1<=attempts<=5,'Setup retry bound must be1..5')
    history=getattr(runtime,'_menu_setup_writes',None)
    if history is None:
        history=[];runtime._menu_setup_writes=history
    receipt=dict(address=hex(address),data=data.hex(),attempts=[],success=False,
                 scope='same bytes only; explicit setup, no menu/result flags')
    history.append(receipt)
    for attempt in range(1,attempts+1):
        require(not runtime.request('netplay_status')['active'],'Offline setup writes only')
        play=runtime.read(0x800D1C00,0x20)
        require(play[:2]==b'\x0a\x00' and not play[0x1c] and
                runtime.read(runtime.diagnostic()+0x4c,2)==b'\0\0',
                'Resource setup requires native unpaused gameplay without scene lock')
        before=runtime.request('frame')['frame']
        for offset,value in enumerate(data):
            runtime.request('write_ram',addr=hex(address+offset),val=hex(value))
        first=runtime.read(address,len(data))
        end=time.monotonic()+3
        frame=runtime.request('frame')['frame']
        while frame<before+2 and time.monotonic()<end:
            time.sleep(.02);frame=runtime.request('frame')['frame']
        require(frame>=before+2,'Frontend stopped while verifying allowed setup bytes')
        settled=runtime.read(address,len(data))
        receipt['attempts'].append(dict(attempt=attempt,frame_before=before,frame_after=frame,
                                        immediate=first.hex(),settled=settled.hex()))
        if first==data and settled==data:
            receipt['success']=True;return receipt
        time.sleep(.02)
    raise AssertionError('Explicit byte setup did not persist after bounded retries: '+json.dumps(receipt))


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--session',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True)
    p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[1])
    p.add_argument('--slot',type=int,choices=range(10),default=8)
    p.add_argument('--prepare',action='store_true')
    p.add_argument('--weapon-eligibility-mask',type=lambda text:int(text,0),choices=range(1,256),metavar='MASK',
                   help='Disclosed temporary P1 BODY+C9 mask; grants no earned progress/reward')
    a=p.parse_args()
    sys.path.insert(0,str(a.repo/'tools'))
    import psutil
    from coop_debug import Runtime,PLAY
    from coop_private_observer import PrivateObserver
    from test_mmx5_coop_snapshot_runtime import state_operation
    from audit_mmx5_coop_locations import counter,wait_ready
    parent=json.loads(a.session.read_bytes());exe=Path(parent['command'][0]).resolve()
    require(Path(psutil.Process(parent['pid']).exe()).resolve()==exe,'Not owned executable')
    sha=hashlib.sha256(exe.read_bytes()).hexdigest()
    require(sha==parent['executable_sha256'],'Candidate executable changed')
    r=Runtime(parent['port'])
    require(not r.request('netplay_status')['active'],'Offline resource setup only')
    a.out.mkdir(parents=True,exist_ok=False)
    (a.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    report=dict(success=False,human_validation=False,session=str(a.session.resolve()),candidate=sha,
        samples=[],operations=[],input=[],setup=['HP/ammo/E/W supplies','Contact protection during menu/control check','E2 energy zeroed; one shared E16 and W16 supplied'],
        limits='One X/P1-Zero/P2 shared-E episode. Private-E option and LAN are separate existing contracts. '
               'Own-menu art/fades require review of composed captures; no visual pass from state alone.')
    obs=None
    r._menu_setup_writes=[];report['setup_write_receipts']=r._menu_setup_writes
    def save(): (a.out/'report.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    def wait(pred,label,seconds=30):
        end=time.monotonic()+seconds
        while True:
            value=pred()
            if value:return value
            require(time.monotonic()<end,label)
            time.sleep(.025)
    def frames(n):
        start=r.request('frame')['frame'];wait(lambda:r.request('frame')['frame']>=start+n,'Frontend frames stopped',20)
    def pulse(button,seat,n=6):
        report['input'].append(dict(seat=seat,button=hex(button),frames=n))
        r.input(button,seat)
        try:frames(n)
        finally:r.input(0,seat)
        frames(8)
    def host_symbols():
        text=subprocess.check_output(['C:/msys64/mingw64/bin/nm.exe','--defined-only',str(exe)],encoding='utf-8',errors='replace')
        names=['second','personal_menus','personal_active','personal_initialized','personal_menu_call',
               'personal_rendering','personal_shared_call','stage_call']
        values={}
        for name in names:
            found=re.findall(r'^([0-9a-fA-F]+) [bBdD] '+name+'$',text,re.MULTILINE)
            require(len(found)==1,'Missing/ambiguous exact symbol '+name);values[name]=int(found[0],16)
        return {k:obs.second+v-values['second'] for k,v in values.items()}
    def menus():
        for _ in range(100):
            guard=lambda:tuple(int.from_bytes(obs.read(symbols[k],4),'little') for k in
                ['personal_menu_call','personal_rendering','personal_shared_call','stage_call'])
            g=guard()
            if not any(g):
                raw=obs.read(symbols['personal_menus'],0x3d*2)
                if raw==obs.read(symbols['personal_menus'],len(raw)) and guard()==g:
                    return [raw[:0x3d],raw[0x3d:]]
            time.sleep(.001)
        raise RuntimeError('No stable equal menu copies outside native transactions')
    def interactive():
        m=menus()
        return r.read(PLAY,2)==b'\x0a\x02' and not r.read(0x8009A424,1)[0] and all(x[4]==1 for x in m)
    def sample(label,image=True):
        m=menus();s=r.personal_effects()
        row=dict(label=label,menus=[x.hex() for x in m],bodies=[b.hex() for b in s['bodies']],
            state=s['state'],life=s['life'],scene=s['scene'],play=r.read(PLAY,0x1c0).hex(),
            host_copy_contract='two equal host-menu copies; native menu/render/shared/stage guards zero')
        report['samples'].append(row);save()
        if image:r.screenshot(a.out/(label+'.png'))
        return row
    def tank(seat,cursor):
        if menus()[seat][0x14]<10:pulse(0x10,seat)
        for _ in range(5):
            if menus()[seat][0x14]==cursor:break
            pulse(0x20,seat)
        require(menus()[seat][0x14]==cursor,'Native tank cursor did not reach '+str(cursor))
    def move(seat):
        before=r.players();x=struct.unpack_from('<i',before[seat],8)[0]
        other=struct.unpack_from('<i',before[seat^1],8)[0]
        r.input(0x20,seat)
        try:wait(lambda:struct.unpack_from('<i',r.players()[seat],8)[0]>x+65536,'Native movement did not recover',15)
        finally:r.input(0,seat)
        require(struct.unpack_from('<i',r.players()[seat^1],8)[0]==other,'Other seat moved from owner input')
    try:
        r.release();r.resume_native_pause()
        if a.prepare:
            report['setup'].append('Native raw1/0/CP0 loader bypasses traversal; no menu/result flag seeded')
            r.fixture(11,hp=3);r.fixture(12,hp=3)
            old=counter(r,'mmx5.coop.enrolled');r.load_location(1,0,0);wait_ready(r,old)
        require(r.read(PLAY,2)==b'\x0a\x00','Need genuine unpaused native gameplay')
        require([b[2] for b in r.players()]==[0,1],'This bounded episode requires X/P1-Zero/P2')
        wait(lambda:all(b[4]==1 and b[0x70]&8 for b in r.players()),'Need known native support for both heroes',30)
        if a.weapon_eligibility_mask is not None:
            original=r.read(0x8009A169,1)[0]
            write_setup_ram(r,0x8009A169,bytes([original|a.weapon_eligibility_mask]))
            report['setup'].append(dict(temporary_X_BODY_C9_eligibility=hex(a.weapon_eligibility_mask),
                                        original=hex(original),claim='Input/equipment only; no acquisition/progress pass'))
        require(r.read(0x8009A169,1)[0]!=0,'Need earned eligibility or explicit --weapon-eligibility-mask')
        require(not parent.get('separate_etanks',False),'This episode verifies unchanged default shared E')
        r.fixture(12,hp=3);r.fixture(11,hp=0)
        r.fixture(2,seat=0,hp=8,ammo=0x90);r.fixture(2,seat=1,hp=4,ammo=0x90)
        r.fixture(4,seat=0,hp=16);write_setup_ram(r,PLAY+0x77,b'\x00')
        acquired=int.from_bytes(r.read(PLAY+0x7e,2),'little')|0x4000
        write_setup_ram(r,PLAY+0x7e,acquired.to_bytes(2,'little'));write_setup_ram(r,PLAY+0x78,b'\x90')
        obs=PrivateObserver(parent['pid'],exe);symbols=host_symbols()
        require(counter(r,'mmx5.coop.private-etanks-effective')==0,'Actual boot policy is private E')
        leak=counter(r,'mmx5.coop.local-view-state-leak')
        pulse(8,1);wait(interactive,'Both native menus did not become interactive')
        baseline=sample('paired-menus-open');world=r.state()['world_ticks'];initial=menus()
        # Distinct concurrent physical-seat inputs; no cursor/menu RAM writes.
        r.input(0x20,0);r.input(0x10,1)
        try:frames(6)
        finally:r.release()
        frames(8)
        m=menus()
        require(m[0][0x14]!=initial[0][0x14] and m[0][0x14]<10,'No distinct earned X weapon cursor')
        require(m[1][0x14]==10 and m[1][5]==1,'P2 did not independently select E1')
        desired=m[0][0x14];sample('distinct-native-cursors')
        # Current-build snapshot restores both host-menu records/input histories.
        before_menus=menus();before_bodies=r.players()
        report['operations'].append(state_operation(r,'save',a.slot));save()
        pulse(0x80,0);pulse(0x20,1)
        report['operations'].append(state_operation(r,'load',a.slot));r.release();wait(interactive,'Menu state load not interactive')
        require(menus()==before_menus,'Current-build state did not restore both native menus')
        require([(b[0x5c],b[0xa8:0xc8],b[0x93]) for b in r.players()]==
                [(b[0x5c],b[0xa8:0xc8],b[0x93]) for b in before_bodies],'State lost personal resources/selection')
        sample('restored-distinct-menus')
        pulse(0x4000,1,2)
        wait(lambda:menus()[1][5]==3,'P2 native E healing did not begin')
        old_weapon=r.players()[0][0x93]
        pulse(0x4000,0,2)
        expected=r.read(0x80073370+desired,1)[0]
        require(r.players()[0][0x93]==expected and expected!=old_weapon,'P1 native equipment did not change')
        require(r.read(PLAY+1,1)==b'\x02','Weapon confirmation prematurely resumed the team')
        sample('equipment-during-partner-heal')
        r.input(8,0)
        try:wait(lambda:menus()[1][5]==1,'Native E healing did not complete',40);frames(12)
        finally:r.input(0,0)
        frames(8)
        require(r.read(PLAY+1,1)==b'\x02','Held Start queued a resume through healing')
        require(r.players()[1][0x5c]==20 and r.players()[0][0x5c]==8,'Shared E spent wrong amount/owner')
        require((r.read(PLAY+0x76,1)[0]&0x7f)==0,'Shared16 energy was not consumed exactly once')
        tank(0,10);pulse(0x4000,0);frames(20)
        require(r.players()[0][0x5c]==8,'Default shared E silently became a second private reserve')
        wait(lambda:menus()[0][5]==1,'Empty shared E did not finish natively')
        require(r.read(PLAY+0x78,1)==b'\x90','E operation spent shared W')
        sample('shared-E-not-duplicated')
        tank(0,12);tank(1,12)
        other_ammo=r.players()[1][0xa8:0xc8]
        r.input(0x4000,0);r.input(0x4000,1)
        try:frames(2)
        finally:r.release()
        wait(lambda:menus()[0][5]==4,'P1-priority native W did not start')
        require(menus()[1][5]!=4,'Both seats started the same shared W')
        wait(lambda:menus()[0][5]==1,'Native W operation did not finish',40);frames(12)
        require(r.players()[1][0xa8:0xc8]==other_ammo,'Blocked P2 W request was queued/consumed too')
        require((r.read(PLAY+0x78,1)[0]&0x7f)==0,'Shared W did not consume its native16 units')
        sample('shared-W-serialized')
        pulse(1,1)
        require(menus()[1][5]<5,'P2 Select opened campaign modal')
        require(r.state()['world_ticks']==world,'World simulation advanced while both menus were open')
        # A fresh Start now resumes both using one native exit/world update.
        pulse(8,1)
        wait(lambda:r.read(PLAY+1,1)==b'\x00' and not r.read(0x8009A424,1)[0],
             'Shared native fade/equipment return did not finish',45)
        sample('native-resumed-world')
        move(0);move(1);sample('both-native-controls-recovered')
        require(counter(r,'mmx5.coop.local-view-state-leak')==leak,'Private menu draw leaked host context')
        report['success']=True
        report['requires_composed_review']=['own X/Zero portraits/layout/selected cursor',
            'shared exit fades and black texture/palette handoff','both native bodies/HUD/world after resume']
    except Exception:
        report['error']=traceback.format_exc()
    finally:
        r.release()
        if obs:obs.close()
        save()
    print(json.dumps(dict(success=report['success'],report=str(a.out/'report.json'),error=report.get('error'))))
    return 0 if report['success'] else 1


if __name__=='__main__': raise SystemExit(main())
