"""One authored intro slab: P2 arm, survivor lifetime, native wipe/retry.

Checkpoint/position/HP setup bypasses the intervening route and block. Native
code must arm, move and retire the original object; no success/contact bits
are written. This is a falling object, not an elevator/support-floor pass.
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
from audit_mmx5_coop_locations import counter, wait_ready
from test_mmx5_coop_scene_fatal_runtime import serial


def require(value, message):
    if not value:
        raise AssertionError(message)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--session',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True)
    a=p.parse_args();parent=json.loads(a.session.read_text(encoding='utf-8'))
    if a.out.resolve().drive.upper()=='C:':p.error('Generated QA data must stay off C:')
    exe=Path(parent['command'][0]).resolve()
    require(Path(psutil.Process(parent['pid']).exe()).resolve()==exe,'Not owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest()==parent['executable_sha256'],'Binary changed')
    r=Runtime(parent['port']);require(not r.request('netplay_status')['active'],'Offline only')
    a.out.mkdir(parents=True,exist_ok=False);(a.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    report=dict(success=False,executable_sha256=parent['executable_sha256'],session=str(a.session),
                samples=[],checks=[],bypasses=['native0/0/CP1 loader bypasses route',
                    'P2 position2896,1010 bypasses the preceding block; no trigger/physics/contact/retirement flag set',
                    'HP refill protects setup/P2; P1/P2 fatal fixtures request native deaths for survivor/retry checks'],
                limits='Authored placement42 class1/sub1 arm/fall/retirement only; water, native lifts, rideable support, signed-negative release, other subtypes and network remain unverified.')

    def save():
        (a.out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')

    def observe(label):
        s=r.personal_effects();pool=r.read(0x800C6A10,64*0xA4);slabs=[]
        for i in range(64):
            b=pool[i*0xA4:(i+1)*0xA4]
            if b[0] and b[1]==1:
                slabs.append(dict(address=0x800C6A10+i*0xA4,subtype=b[2],state=list(b[4:8]),
                    pos=list(struct.unpack_from('<ii',b,8)),velocity=struct.unpack_from('<i',b,0x24)[0],
                    acceleration=struct.unpack_from('<i',b,0x2C)[0],body=b.hex()))
        row=dict(label=label,personal=serial(s),slabs=slabs,play=r.read(PLAY,0x80).hex(),
                 trigger_counter=counter(r,'mmx5.coop.p2-falling-slab-triggers'))
        report['samples'].append(row);save();return row

    def wait(test,label,seconds=25,record=False):
        end=time.monotonic()+seconds
        while True:
            row=observe(label) if record else dict(personal=serial(r.personal_effects()))
            if test(row):return row
            require(time.monotonic()<end,label);time.sleep(.04)

    def grounded(row):
        s=row['personal'];return s['life'][:2]==[0,0] and all(
            bytes.fromhex(b)[4:6]==b'\x01\x02' and bytes.fromhex(b)[0x70]&8 for b in s['bodies'])

    def arm(label):
        initial=observe(label+'-before');expected=initial['trigger_counter']+1
        r.teleport(1,2896,1010)
        armed=wait(lambda row:any(x['subtype']==1 and x['state'][2]==1 and x['acceleration']==0x1000
                                 for x in row['slabs']),label+'-arm',record=True)
        require(armed['trigger_counter']==expected,'New native trigger callback did not run exactly once')
        bodies=[bytes.fromhex(b)for b in armed['personal']['bodies']]
        require(struct.unpack_from('<i',bodies[0],8)[0]<2880*65536 and
                struct.unpack_from('<i',bodies[1],8)[0]>=2880*65536,'Wrong actual native trigger setup')
        require(armed['personal']['life'][:2]==[0,0],'Approach lost a hero before arm')
        target=next(x for x in armed['slabs'] if x['subtype']==1)
        require(target['pos'][1]>=960*65536,'Original slab did not retain authored origin')
        r.screenshot(a.out/(label+'-armed.png'));return target,armed

    try:
        r.release();r.resume_native_pause();r.fixture(11,hp=3);r.fixture(12,hp=0)
        enrolled=counter(r,'mmx5.coop.enrolled');r.load_location(0,0,1);wait_ready(r,enrolled)
        wait(grounded,'Native checkpoint floor did not settle');observe('native-grounded-entry')
        retries=r.read(PLAY+0x45,1)[0];flags=r.read(PLAY+0x4C,8)
        target,armed=arm('first-P2')
        r.fixture(11,hp=2);r.fixture(1,seat=0)
        wait(lambda row:row['personal']['life'][0] in (4,5),'P1 native fatal entry did not run')
        retired=wait(lambda row:all(x['address']!=target['address'] or x['subtype']!=1
                                   for x in row['slabs']),'Native slab did not retire with P1 dead',record=True)
        require(retired['personal']['life'][0] in (4,5) and retired['personal']['life'][1]==0,
                'P1 fatal blocked/lost its living survivor')
        require(any(x['pos'][1]>target['pos'][1] for row in report['samples']
                    for x in row.get('slabs',[]) if x['address']==target['address'] and x['subtype']==1),
                'No actual native falling motion observed')
        require(r.read(PLAY+0x45,1)[0]==retries and r.read(PLAY+0x4C,8)==flags,
                'One fallen hero caused a retry or changed completion outcomes')
        wait(lambda row:row['personal']['life'][0]==5,'P1 death did not finish')
        r.input(0x4000,1);start=r.state()['world_ticks'];before_y=struct.unpack_from('<i',r.players()[1],12)[0]
        wait(lambda row:row['personal']['state']['world_ticks']>=start+6,'World stopped after slab retirement')
        r.release();require(struct.unpack_from('<i',r.players()[1],12)[0]<before_y-65536,'P2 native jump did not recover')
        observe('living-P2-native-jump');r.screenshot(a.out/'survivor.png')
        report['checks']+=['P2 original threshold arm/callback once/native falling motion',
                           'Armed original object retires while P1 is dead; native P2 jump and no team retry']
        # The final survivor dies through native code; the original retry owns
        # checkpoint/life restoration. No loader/continuation fixture substitutes.
        enrolled=counter(r,'mmx5.coop.enrolled');r.fixture(1,seat=1)
        wait(lambda row:counter(r,'mmx5.coop.enrolled')>enrolled and grounded(row),
             'Original team wipe/retry did not restore grounded pair',seconds=80)
        after_retry=observe('actual-native-retry')
        require(r.read(PLAY+12,2)==b'\0\0' and r.read(PLAY+0x1D,1)==b'\x01','Native retry changed checkpoint')
        require(r.read(PLAY+0x45,1)[0]==((retries-1)&255),'Native wipe did not spend exactly one retry')
        require(r.read(PLAY+0x4C,8)==flags,'Native retry changed boss outcomes')
        target,armed=arm('after-retry-P2')
        wait(lambda row:all(x['address']!=target['address'] or x['subtype']!=1 for x in row['slabs']),
             'Retried native slab did not retire',record=True)
        require(r.lifecycle()[:2]==(0,0),'Retried falling object lost a healthy hero')
        observe('native-retry-second-retirement');r.screenshot(a.out/'retry.png')
        report['checks'].append('One actual native team wipe/retry restores checkpoint/pair; original P2-triggered object arms and retires again')
        report['success']=True;print('PASS authored falling object / survivor lifetime / actual native retry',flush=True)
    except Exception:
        report['error']=traceback.format_exc();print(report['error'],flush=True)
    finally:
        try:r.release();observe('final')
        except Exception:pass
        save()
    return 0 if report['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())
