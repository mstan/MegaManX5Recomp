"""Complete one actual P2-led Grizzly refight and its original hub return.

Requires the existing pad2 entry receipt on the same owned offline session.
Native motion, camera targets, an actual ammo spend, final hit and return are
checked together. Position/positive boss HP are setup only; no success flags.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time
import traceback

import psutil
from coop_debug import Runtime,PLAY
from test_mmx5_coop_scene_fatal_runtime import serial


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--entry',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();parent=json.loads(args.session.read_text(encoding='utf-8'))
    exe=Path(parent['command'][0]).resolve()
    assert Path(psutil.Process(parent['pid']).exe()).resolve()==exe
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==parent['executable_sha256']
    entry=json.loads(args.entry.read_text(encoding='utf-8'))
    assert entry['success'] and entry['leader']==1 and len(entry['cases'])==1
    assert entry['cases'][0]['pad']['byte2']==2 and entry['executable_sha256']==parent['executable_sha256']
    assert Path(entry['session']).resolve()==args.session.resolve()
    r=Runtime(parent['port']);assert not r.request('netplay_status')['active']
    args.out.mkdir(parents=True,exist_ok=False);(args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    d=dict(success=False,session=str(args.session.resolve()),entry=str(args.entry.resolve()),
           candidate_sha256=parent['executable_sha256'],samples=[],checks={},writes=[],
           bypasses=['Existing native stage12/0 hub checkpoint1 load and P2 pad approach; preceding campaign/route bypassed.',
                     'HP-only refill through setup and actual final hit; disabled after bossHP0 before native return. Native hurt/contact timers remain untouched; no healing during return.',
                     'X/P2 position/history for the native buster approach. No camera, terrain, ammo, phase, success or progression writes.'],
           limits='One actual Grizzly pad2/CP2->native CP11 return. Native release restores camera rangeX464..688/Y16 after its temporary688 lock. No owner-fatal timing/F103, other refights, production or netplay claim.')
    boss=0x80092090

    def save():
        (args.out/'report.json').write_bytes((json.dumps(d,indent=2)+'\n').encode())

    def obs(label):
        personal=r.personal_effects();p=r.read(PLAY,0x1c0);c=r.read(r.diagnostic()+0x710,0x1f8)
        row=dict(label=label,personal=serial(personal),play=p.hex(),boss=r.read(boss,0x9c).hex(),
                 cameras=[c[:0xfc].hex(),c[0xfc:].hex()])
        d['samples'].append(row);assert not personal['state']['failed']
        return row,p,personal

    def wait(test,label,seconds=25):
        end=time.monotonic()+seconds
        while not test():
            assert not r.state()['failed'] and time.monotonic()<end,label
            time.sleep(.015)

    def ticks(n):
        before=r.state()['world_ticks'];wait(lambda:r.state()['world_ticks']>=before+n,'Native world stopped')

    def move(seat,button,n,label):
        wait(lambda:r.players()[seat][4:6]==b'\x01\x02','Hero must finish native hurt/motion before input probe')
        before=obs(label+'-before');r.input(button,seat)
        try:ticks(n)
        finally:r.input(0,seat)
        after=obs(label+'-after');a,b=before[2]['bodies'][seat],after[2]['bodies'][seat]
        delta=struct.unpack_from('<i',b,8)[0]-struct.unpack_from('<i',a,8)[0]
        assert delta>2*65536 if button==0x20 else delta<-2*65536
        assert after[2]['life'][:2]==[0,0] and not after[2]['scene'][0]
        return dict(before=before[0],after=after[0],delta_x=delta/65536)

    def grounded():
        s=r.personal_effects();p=r.read(PLAY,0x20)
        return p[:2]==b'\x0a\0' and not p[0x1c] and s['life'][:2]==[0,0] and s['scene']==[0,0] and all(
            b[4]==1 and on_ground(b) and not any(b[o] for o in (0x7a,0xd0,0xd4)) for b in s['bodies'])

    def pads():
        pool=r.read(0x800c6a10,64*0xa4)
        return [dict(address=hex(0x800c6a10+i),body=pool[i:i+0xa4].hex()) for i in range(0,len(pool),0xa4)
                if pool[i] and pool[i+1:i+3]==b'\x17\x02']

    def on_ground(body):
        if body[0x70]&8:return True
        if not body[0x71]&8 or not struct.unpack_from('<I',body,0x68)[0]:return False
        box=r.read(struct.unpack_from('<I',body,0x68)[0],4)
        foot=struct.unpack_from('<i',body,12)[0]/65536+struct.unpack_from('<b',box,1)[0]+box[3]
        x=struct.unpack_from('<i',body,8)[0]/65536
        for actor in pads():
            pad=bytes.fromhex(actor['body'])
            if pad[4]!=1:continue
            collider=r.read(struct.unpack_from('<I',pad,0x68)[0],4)
            top=struct.unpack_from('<i',pad,12)[0]/65536+struct.unpack_from('<b',collider,1)[0]-collider[3]
            center=struct.unpack_from('<i',pad,8)[0]/65536
            if foot==top and abs(x-center)<box[2]+collider[2]:return True
        return False

    try:
        r.input(0,0);r.input(0,1);r.resume_native_pause();r.fixture(11,hp=3);r.fixture(12,hp=0)
        p=r.read(PLAY,0x1c0);assert p[12:14]==b'\x0c\0' and p[0x1d]==2 and not p[0x2e]
        wait(grounded,'Both original refight arrivals must reach safe native feet/control')
        d['arrival']=obs('actual-grounded-refight-arrival')[0];r.screenshot(args.out/'grounded-arrival.png')
        d['arrival_motion']=[]
        d['arrival_motion'].append(move(1,0x20,12,'p2-native-right'))
        # Probe toward the arena interior after native entry.
        d['arrival_motion'].append(move(0,0x20,8,'p1-native-right'))
        d['checks']['safe_arrival_and_both_native_controls']='passed';save();ticks(40)
        camera=obs('released-authored-arena-camera')[0]
        camera_ok=True
        for raw in camera['cameras']:
            c=bytes.fromhex(raw)
            x,y=struct.unpack_from('<ii',c,8)
            camera_ok &= 464*65536<=x<=688*65536 and y==16*65536
            camera_ok &= struct.unpack_from('<8h',c,0x1c)==(688,464,16,16,688,464,16,16)
        for before,after in zip(d['arrival']['cameras'],camera['cameras']):
            camera_ok &= abs(struct.unpack_from('<i',bytes.fromhex(after),8)[0]-struct.unpack_from('<i',bytes.fromhex(before),8)[0])>2*65536
        d['camera_targets']=camera;d['checks']['both_native_arena_targets_and_follow']='passed' if camera_ok else 'failed'
        r.screenshot(args.out/'native-arena-cameras.png');save()
        wait(lambda:r.players()[1][4:6]==b'\x01\x02','X must finish native motion before ammo check')
        for _ in range(9):
            if r.players()[1][0x93]==1:break
            r.input(0x800,1);ticks(6);r.input(0,1);ticks(8)
        else:raise AssertionError('Earned native cycle did not select CShot')
        wait(lambda:not r.players()[1][0x98],'Previous native shot must retire')
        energy=struct.unpack_from('<H',r.players()[1],0xaa)[0]
        for _ in range(12):
            wait(lambda:r.players()[1][4:6]==b'\x01\x02','X must finish native hurt before a fresh attack edge')
            r.input(0x1000,1);ticks(4);r.input(0,1);ticks(8)
            spent=struct.unpack_from('<H',r.players()[1],0xaa)[0]
            if spent<energy:break
        else:raise AssertionError('No actual native earned CShot ammo spend')
        d['actual_ammo_spend']=[energy,spent]
        wait(lambda:not r.players()[1][0x98],'Native CShot must retire before weapon switching')
        wait(lambda:r.players()[1][4:6]==b'\x01\x02','X must finish native attack/hurt before weapon switching')
        r.input(0x400,1);ticks(6);r.input(0,1);ticks(8)
        assert r.players()[1][0x93]==0
        d['before_finish']=obs('before-native-final-hit')[0]
        b=r.read(boss,0x9c);assert b[:2]==b'\x41\x36' and b[4]==1 and b[0x5c]>0
        r.fixture(12,hp=0)
        d['writes'].append(dict(address=hex(boss+0x5c),before=b[0x5c],value=1,purpose='PositiveHP setup only'))
        r.request('write_ram',addr=hex(boss+0x5c),val='0x01');d['positive_hp_setup']=obs('positive-boss-hp-only')[0];save()
        # Reuse the actual grounded native entry ledge. A generic boss-80px
        # approach can place X in the lower room hazard between platforms.
        anchor=bytes.fromhex(d['arrival']['personal']['bodies'][1])
        target,floor=(v/65536 for v in struct.unpack_from('<ii',anchor,8))
        d['safe_finish_anchor']=[target,floor]
        for _ in range(24):
            row,p,s=obs('native-buster-finish');b=bytes.fromhex(row['boss'])
            if not b[0x5c]&127 and b[4]==2:d['actual_final_hit']=row;break
            assert p[0x1d]==2 and s['life'][:2]==[0,0] and b[4]==1
            if not struct.unpack_from('<I',b,0x54)[0] or s['bodies'][1][5]==0x11:ticks(8);continue
            x=struct.unpack_from('<i',b,8)[0]/65536;on_left=x>target
            r.teleport(1,target,floor);r.input(0x20 if on_left else 0x80,1);ticks(2)
            r.input(0x8000,1);ticks(4);r.input(0,1);ticks(24)
        else:raise AssertionError('No actual native Grizzly refight final hit')
        r.fixture(11,hp=0)
        r.input(0,0);r.input(0,1);r.screenshot(args.out/'actual-refight-defeat.png');save()
        end=time.monotonic()+100;saw_return=False
        while True:
            row,p,s=obs('native-defeat-to-hub-return')
            assert all(v in (0,3) for v in s['life'][:2]),'A living hero was lost during original refight return'
            if p[0x1d]==11:saw_return=True
            if p[0x1d]==11 and p[0] in (8,9):d.setdefault('native_return_departure',row)
            if saw_return and 'native_return_departure' in d and p[:2]==b'\x0a\0' and p[0x1d]==11 and not p[0x1c] and \
               s['life'][:2]==[0,0] and s['scene']==[0,0] and all(
                   body[4]==1 and on_ground(body) and not any(body[o] for o in (0x7a,0xd0,0xd4)) for body in s['bodies']):
                d['returned']=row;break
            assert time.monotonic()<end,'Native refight defeat did not safely return both heroes'
            time.sleep(.03)
        assert p[12:14]==b'\x0c\0' and p[0x2e] in (1,2)
        d['return_pad_actors']=pads();assert len(d['return_pad_actors'])==1
        pad=bytes.fromhex(d['return_pad_actors'][0]['body']);assert pad[4]==1 and pad[5]==1
        d['pad_collision_box']=r.read(struct.unpack_from('<I',pad,0x68)[0],4).hex()
        d['return_body_boxes']=[r.read(struct.unpack_from('<I',b,0x68)[0],4).hex() for b in s['bodies']]
        d['return_motion']=[move(1,0x20,6,'returned-p2-native-right'),move(0,0x80,6,'returned-p1-native-left')]
        d['checks']['actual_defeat_original_cp11_return_safe_feet_and_controls']='passed'
        d['after_return']=obs('native-hub-after-motion')[0];r.screenshot(args.out/'actual-safe-hub-return.png')
        assert [b[0xc9] for b in s['bodies']]==[0xf9,0xf9]
        d['ammo_after_return']=struct.unpack_from('<H',s['bodies'][1],0xaa)[0]
        assert d['ammo_after_return']==spent,'Native continuation lost the actually spent private ammo'
        for current,previous in zip(s['bodies'],d['actual_final_hit']['personal']['bodies']):
            previous=bytes.fromhex(previous)
            assert current[0x5c]==previous[0x5c] and current[0xa8:0xc8]==previous[0xa8:0xc8]
        d['checks']['earned_inventory_retained']='passed';d['success']=True
    except Exception:
        d['error']=traceback.format_exc()
        try:d['failure']=obs('failure')[0];r.screenshot(args.out/'failure.png')
        except Exception:d['capture_error']=traceback.format_exc()
    finally:
        try:r.input(0,0);r.input(0,1);r.fixture(11,hp=0);r.fixture(12,hp=0)
        finally:save()
    print(json.dumps({k:d.get(k) for k in ('success','checks','actual_ammo_spend','ammo_after_return','error')}),flush=True)
    return 0 if d['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())
