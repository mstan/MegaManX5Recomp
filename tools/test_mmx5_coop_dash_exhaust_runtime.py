"""Check ordinary native dash exhaust ownership, expiry and slot reuse.

Use an owned offline runner. Grizzly CP0, HP/contact protection and positions
are setup bypasses. Native input creates the dash and the original shared
effect; retained dash trails are a separate mechanic. No effect flags are set.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time
import traceback

import psutil
from coop_debug import Runtime, PLAY
from audit_mmx5_coop_locations import counter, snapshot, wait_ready


def require(value, message):
    if not value:
        raise AssertionError(message)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--dat',type=Path,required=True)
    args=parser.parse_args()
    session=json.loads(args.session.read_text(encoding='utf-8'))
    exe=Path(session['command'][0]).resolve()
    require(Path(psutil.Process(session['pid']).exe()).resolve()==exe,'Not owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest()==session['executable_sha256'],'Binary changed')
    r=Runtime(session['port'])
    require(not r.request('netplay_status')['active'],'Offline setup only')
    args.out.mkdir(parents=True,exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    report=dict(success=False,human_validation=False,session=str(args.session.resolve()),
                executable_sha256=session['executable_sha256'],
                harness_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                cases=[],bypassed_prerequisites=['Grizzly CP0 native loader','HP/contact protection',
                    'P2 position/history only, native floor checked before input'],
                limits='One on-foot dash per seat on this candidate; shared ordinary type1 exhaust only. '
                    'Sequential pool reads bracketed by coherent bodies; composed captures retained for visual review. '
                    'No vehicle exhaust, wall-jump dust, post-door dash or broad stage pass.')
    dat=args.dat.read_bytes()
    report['dat_sha256']=hashlib.sha256(dat).hexdigest()
    assemblies=[]
    for record in (88,90):
        sector,size=struct.unpack_from('<II',dat,record*8)
        entry=dat[sector*2048:sector*2048+size]
        require(len(entry)==size and struct.unpack_from('<I',entry,4)[0]==size,'Invalid original character record')
        cursor=2048
        for asset in range(10):
            kind,length=struct.unpack_from('<II',entry,8+asset*8)
            if asset==9:
                require(kind==3 and cursor+length<=len(entry),'Missing original assembly')
                assemblies.append(entry[cursor:cursor+length])
            cursor+=(length+2047)&~2047

    def save():
        (args.out/'report.json').write_bytes((json.dumps(report,indent=2)+'\n').encode('utf-8'))

    def wait(predicate,message,seconds=12):
        end=time.monotonic()+seconds
        while not predicate():
            require(time.monotonic()<end,message)
            time.sleep(.01)

    def ticks(n):
        start=r.state()['world_ticks']
        wait(lambda:r.state()['world_ticks']>=start+n,'Native world stopped')

    def effects():
        pool=r.read(0x80094398,0x2A00)
        return [(i,pool[i*0x70:(i+1)*0x70]) for i in range(96)
                if pool[i*0x70] and pool[i*0x70+1]==1]

    def idle():
        return r.lifecycle()[:2]==(0,0) and all(b[4:6]==b'\x01\x02' and b[0x70]&8 for b in r.players())

    try:
        r.release();r.resume_native_pause()
        previous=counter(r,'mmx5.coop.enrolled')
        r.load_location(1,0,0);wait_ready(r,previous)
        r.fixture(11,hp=3);r.fixture(12,hp=3)
        wait(idle,'Original arrival did not settle')
        start=snapshot(r)
        x,y=start['players'][0]['x'],start['players'][0]['y']
        r.teleport(1,x-80,y);ticks(8)
        require(idle(),'Position setup did not retain both grounded heroes')
        previous_slots=set()
        for seat in (1,0):
            case=dict(seat=seat,before=snapshot(r),samples=[])
            report['cases'].append(case)
            require(not effects(),'Previous ordinary exhaust remains active')
            # P2 approaches stationary P1; the next native P1 allocation must
            # clear a reused P2-owned slot rather than follow P2 again.
            r.input(0x2020,seat)
            begin=r.state()['world_ticks']
            slots=set();initialized=0;captured=False
            while r.state()['world_ticks']<begin+12:
                before=r.personal_effects()
                active=effects()
                after=r.personal_effects()
                row=dict(world_before=before['state']['world_ticks'],world_after=after['state']['world_ticks'],
                         bodies_before=[b.hex() for b in before['bodies']],
                         bodies_after=[b.hex() for b in after['bodies']],effects=[])
                case['samples'].append(row)
                for index,actor in active:
                    row['effects'].append(dict(index=index,raw=actor.hex()))
                    slots.add(index)
                    if not actor[4]:continue
                    body=after['bodies'][seat]
                    table=assemblies[body[2]]
                    base=struct.unpack_from('<I',body,0x3c)[0]-struct.unpack_from('<I',table)[0]
                    expected=base+struct.unpack_from('<I',table,4)[0]
                    require(struct.unpack_from('<I',actor,0x3c)[0]==expected,'Exhaust uses the other hero assembly')
                    require(actor[0x15]==body[0x15],'Exhaust uses the other hero facing')
                    require(struct.unpack_from('<H',actor,0x42)[0]==0x7802,'Exhaust lost native CLUT7802')
                    # Original4F654 uses signed integer position halves and
                    # character offsets. Publication can lag the body pass by
                    # one tick; allow one native dash step plus rounding.
                    dx,dy=((32,12),(36,14))[body[2]]
                    if body[0x15]:dx=-dx
                    for offset,delta in ((8,dx),(12,dy)):
                        bounds=[(struct.unpack_from('<i',s['bodies'][seat],offset)[0]>>16)+delta
                                for s in (before,after)]
                        value=struct.unpack_from('<i',actor,offset)[0]>>16
                        require(min(bounds)-8<=value<=max(bounds)+8,'Exhaust follows the other hero position')
                    initialized+=1
                if initialized and not captured:
                    r.screenshot(args.out/f'seat{seat+1}-dash.png');captured=True
                time.sleep(.003)
            r.release()
            require(initialized,'No initialized native ordinary exhaust observed')
            require(any(0<b[0x8c]<128 for row in case['samples'] for b in
                        [bytes.fromhex(row['bodies_before'][seat])]),'Original dash was not admitted')
            wait(lambda:not effects(),'Ordinary exhaust did not expire')
            wait(idle,'Dash did not return to native floor/control')
            case['after']=snapshot(r)
            require(not case['after']['state']['failed'],'Co-op failed')
            worlds=case['after']['state']['world_ticks']-case['before']['state']['world_ticks']
            native=case['after']['state']['native_timer']-case['before']['state']['native_timer']
            require(worlds==native and worlds>0,'Dash changed once-per-world timing')
            require(case['after']['players'][seat^1]['x']==case['before']['players'][seat^1]['x'],
                    'Dash moved the other hero')
            case.update(slots=sorted(slots),initialized_observations=initialized,
                        reused_slots=sorted(slots&previous_slots),passed=True)
            if seat==0:require(case['reused_slots'],'No native slot reuse was exercised')
            previous_slots=slots
            save()
        report['success']=True
    except Exception:
        report['error']=traceback.format_exc()
    finally:
        r.release()
        if r.read(PLAY,2)==b'\x0a\0':r.tap(8,.2)
        save()
    print(json.dumps({k:report.get(k) for k in ('success','error')},ensure_ascii=True),flush=True)
    return 0 if report['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())
