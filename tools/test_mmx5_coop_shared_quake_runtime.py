"""Check both private cameras through one original intro debris/impact event.

Native CP1 and position/history setup bypass the preceding route. P1's original
jump crosses the source X>=3048 trigger. Native code must issue, advance and
retire both quake axes; no effect, timer, phase or success flags are seeded.
Palette flashes, distant local effects and the later rescue remain separate.
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


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    parent=json.loads(args.session.read_text(encoding='utf-8-sig'))
    exe=Path(parent['command'][0]).resolve()
    assert Path(psutil.Process(parent['pid']).exe()).resolve()==exe, 'Not the owned QA process'
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==parent['executable_sha256'], 'Candidate changed'
    r=Runtime(parent['port'])
    assert not r.request('netplay_status')['active'], 'Offline setup only'
    args.out.mkdir(parents=True,exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    d=dict(success=False,candidate_sha256=parent['executable_sha256'],samples=[],axes={},
           setup='Original raw0CP1 loader; HP-only refill/contact grace off; position/history/camera setup to source-qualified native floor before X>=3048. Route bypassed.',
           limits='One native nearby shared debris/impact event, both quake axes and retirement/control. No distant palette flash, later rescue, whole intro or online effect pass.')

    def counters():
        return {row['name']:int(row['count']) for row in r.request('mod_counters')['counters']}

    def observe(label):
        data=r.read(r.diagnostic(),0x908)
        row=dict(label=label,world=struct.unpack_from('<I',data,24)[0],
                 life=list(data[0x40:0x44]),scene=list(data[0x4C:0x4E]),
                 bodies=[data[0x300:0x458].hex(),data[0x100:0x258].hex()],
                 cameras=[data[0x710+i*0xFC:0x710+(i+1)*0xFC].hex() for i in (0,1)])
        d['samples'].append(row)
        assert not struct.unpack_from('<I',data,12)[0], 'Co-op failure'
        return row,[bytes.fromhex(b) for b in row['bodies']],[bytes.fromhex(c) for c in row['cameras']]

    def effects():
        return {hex(base):r.read(base,n*size).hex() for base,n,size in
                ((0x80092090,48,0x9C),(0x80094398,96,0x70),(0x800D1F40,128,0x60))}

    def wait(predicate,label,seconds=45):
        end=time.monotonic()+seconds
        while True:
            row,bodies,cameras=observe(label)
            if predicate(row,bodies,cameras):return row,bodies,cameras
            assert time.monotonic()<end,label
            time.sleep(.015)

    try:
        r.release();r.resume_native_pause();r.fixture(11,hp=3);r.fixture(12,hp=0)
        r.load_location(0,0,1)
        wait(lambda z,b,c:r.read(PLAY,1)==b'\x0A' and r.read(PLAY+0x1D,1)==b'\1' and
             z['life'][:2]==[0,0] and z['scene']==[0,0] and not r.read(PLAY+0x1C,1)[0],
             'native-checkpoint-release')
        r.teleport_pair(((2960,1120),(2920,1120)),(2800,960))
        settled=observe('native-position-acknowledged')[0]['world']
        wait(lambda z,b,c:z['world']>=settled+24,'native-position-settling')
        r.input(0x20,0)
        approach=observe('native-walk-start')[0]['world']
        wait(lambda z,b,c:z['world']>=approach+120,'original-walk-before-obstacle')
        r.input(0,0)
        released=observe('native-walk-release')[0]['world']
        row,bodies,cameras=wait(lambda z,b,c:z['world']>=released+4 and
                                all(v[4:6]==b'\1\2' and v[0x70]&8 for v in b),
                                'native-floor-before-trigger')
        assert 3008<=struct.unpack_from('<i',bodies[0],8)[0]/65536<3048, 'Missed original pre-trigger obstacle'
        d['before']=row;d['counters_before']=counters();start=row['world']
        d['effects_before']=effects();r.input(0x4020,0)
        last_active=start;released=False;end=time.monotonic()+35
        while True:
            row,bodies,cameras=observe('native-nearby-event')
            assert row['life'][:2]==[0,0] and row['scene']==[0,0], 'Native event lost a hero/control owner'
            active=False
            for axis,mask in ((0,16),(1,1)):
                if not cameras[0][0x34]&mask:continue
                active=True;last_active=row['world']
                fields=(0x36+axis,0x3A+axis,0x3C+axis,0x3E+axis,0x45+axis)
                assert cameras[1][0x34]&mask, 'P2 missed native quake command'
                assert all(cameras[0][offset]==cameras[1][offset] for offset in fields), 'Private quake clocks/phases disagree'
                entry=d['axes'].setdefault(str(axis),dict(samples=[],origins=[[],[]]))
                entry['samples'].append(row)
                for seat,camera in enumerate(cameras):
                    entry['origins'][seat].append(struct.unpack_from('<h',camera,10+4*axis)[0])
                if not released:r.input(0,0);released=True
                if 'effects'not in entry:
                    entry['effects']=effects()
                    r.screenshot(args.out/f'native-axis-{axis}.png')
            if d['axes'].keys()=={'0','1'} and row['world']>=start+200 and row['world']>=last_active+32:
                d['retired']=row;break
            assert row['world']<start+400 and time.monotonic()<end, 'Native shared event did not finish'
            time.sleep(.01)
        r.release()
        for axis,entry in d['axes'].items():
            assert len(entry['samples'])>=2, 'Insufficient active quake observations'
            assert len(set(entry['origins'][1]))>=2, 'P2 timer advanced without actual camera displacement'
        row,bodies,cameras=wait(lambda z,b,c:b[1][4:6]==b'\1\2' and not b[1][0x67] and b[1][0x70]&8,
                                'native-partner-idle-recovery',15)
        x_before=struct.unpack_from('<i',bodies[1],8)[0];tick=row['world']
        r.input(0x80,1)
        row,bodies,cameras=wait(lambda z,b,c:z['world']>=tick+16,'native-partner-controls',10)
        r.input(0,1);d['p2_left']=(struct.unpack_from('<i',bodies[1],8)[0]-x_before)/65536
        assert d['p2_left']<-2, 'Partner did not retain native control'
        d['counters_after']=counters()
        assert d['counters_after'].get('mmx5.coop.shared-quake-commands',0)>d['counters_before'].get('mmx5.coop.shared-quake-commands',0), 'No original command publication'
        d['stats']=r.request('render_pass_stats')
        assert not any(d['stats'].get(key,0) for key in ('verify_mismatch','watchdog','vram_leaks','device_reads','aborted')), 'Private rendering fault'
        d['effects_after']=effects();r.screenshot(args.out/'native-retired-controls.png')
        d['success']=True
    except Exception:
        d['error']=traceback.format_exc();print(d['error'],flush=True)
    finally:
        r.release()
        try:
            if r.read(PLAY+1,1)==b'\0':r.tap(8,.15,0)
        except Exception:pass
        (args.out/'report.json').write_bytes((json.dumps(d,indent=2)+'\n').encode())
    print(json.dumps({k:d.get(k) for k in ('success','p2_left','error')}),flush=True)
    return 0 if d['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())
