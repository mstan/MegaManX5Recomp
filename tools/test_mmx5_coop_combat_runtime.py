"""Bounded native X charge/release/counter checks in an owned offline session.

The Grizzly checkpoint and contact/HP protection bypass earlier traversal.
Charge, release, effect allocation, projectile expiry and menus use original
input/code. This does not qualify enemy damage, specials or the whole campaign.
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
from audit_mmx5_coop_locations import counter,snapshot,wait_ready


def require(value,message):
    if not value:raise AssertionError(message)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    session=json.loads(args.session.read_text());exe=Path(session['command'][0]).resolve()
    require(Path(psutil.Process(session['pid']).exe()).resolve()==exe,'Not the owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest()==session['executable_sha256'],'Owned binary changed')
    args.out.mkdir(parents=True,exist_ok=False)
    r=Runtime(session['port']);require(not r.request('netplay_status')['active'],'Offline fixture required')
    report=dict(success=False,human_validation=False,executable_sha256=session['executable_sha256'],cases=[],
                session=str(args.session.resolve()),resident_loading=session.get('resident_loading',True),
                bypassed_prerequisites=['native Grizzly checkpoint1 load','HP refill/contact grace held'],
                limits='Native charge/release, owned emitter context and counter recovery; damage/specials/trails unverified.')
    def ticks(n):
        before=r.state()['world_ticks'];deadline=time.monotonic()+20
        while r.state()['world_ticks']<before+n:
            require(time.monotonic()<deadline,'World stopped during native input');time.sleep(.01)
    def wait(predicate,message,seconds=15):
        deadline=time.monotonic()+seconds
        while not predicate():
            require(time.monotonic()<deadline,message);time.sleep(.01)
    def effects():
        data=r.read(0x80094398,0x2a00)
        return [dict(address=hex(0x80094398+i),bytes=data[i:i+0x70].hex())
                for i in range(0,len(data),0x70) if data[i] and data[i+1]==2]
    try:
        r.resume_native_pause();r.release();r.fixture(11,hp=3);r.fixture(12,hp=3)
        previous=counter(r,'mmx5.coop.enrolled');r.load_location(1,0,1);wait_ready(r,previous)
        seat=next(i for i,body in enumerate(r.players()) if body[2]==0);other=seat^1
        report['x_seat']=seat
        for phase in ('before-menu','after-p2-menu','after-p1-menu'):
            if phase!='before-menu':
                owner=1 if phase=='after-p2-menu' else 0
                r.tap(8,.10,owner);wait(lambda:r.read(PLAY+1,1)==b'\x02','Menu did not open')
                r.resume_native_pause()
            for repetition in range(3):
                wait(lambda:all(b[4]==1 and b[5]==2 for b in r.players()),'Heroes not at native idle')
                wait(lambda:not r.players()[seat][0x98] and not effects(),'Previous charge did not expire')
                case=dict(name=f'{phase}-{repetition}',before=snapshot(r),samples=[])
                report['cases'].append(case)
                r.request('spu_events_reset')
                r.input(0x8000,seat);ticks(120)
                charged=r.players();case['charged']=[b.hex() for b in charged]
                require(charged[seat][0x95]>=0x12,'Original input did not reach full buster charge')
                # Original4F21C..4F278 distinguishes base-X18 from Fourth
                # Armor20. The original input/parts select the release kind;
                # do not impose the counterpart's base-X kind on armored P1.
                kind=charged[seat][0x95];case['native_full_charge_kind']=kind
                require(kind in (0x12,0x14),'Unqualified full-charge variant')
                require(charged[other][0x95]==0 and charged[other][0x98]==0,'Charge changed the other hero')
                if repetition==0:r.screenshot(args.out/f'{phase}-charged.png')
                allocation_counter=f'mmx5.coop.p{seat+1}-buster-child-allocations'
                allocations_before=counter(r,allocation_counter)
                r.request('fn_clear');r.request('fn_filter',lo='0x8002D078',hi='0x8004F3F8')
                r.input(0,seat)
                deadline=time.monotonic()+3
                while time.monotonic()<deadline:
                    bodies=r.players();active=effects()
                    case['samples'].append(dict(bodies=[b.hex() for b in bodies],effects=active))
                    if any((b:=bytes.fromhex(e['bytes']))[2]==kind and b[4] for e in active):break
                    time.sleep(.01)
                require(any((b:=bytes.fromhex(e['bytes']))[2]==kind and b[4]
                            for row in case['samples'] for e in row['effects']),
                        f'Initialized full-charge native type2/subtype{kind} emitter was not observed')
                require(all(bytes.fromhex(row['bodies'][other])[0x98]==0 for row in case['samples']),
                        'Charged release incremented the other hero shot counter')
                ticks(8);case['during']=snapshot(r)
                require(bytes.fromhex(case['during']['bodies'][other])[0x98]==0,
                        'Deferred projectile allocation incremented the other hero shot counter')
                # Trace the original allocator: a projectile may hit
                # or leave the view between TCP RAM samples. Keep any sampled
                # counter transition as additional evidence, not an allocation
                # prerequisite based on observer timing.
                ticks(20)
                case['trace']=r.request('fn_entry_dump',count=512,addr_lo='0x8004F3F4',addr_hi='0x8004F3F8')
                case['allocations']=r.request('fn_entry_dump',count=128,addr_lo='0x8002D078',addr_hi='0x8002D07C')
                case['allocation_returns']=r.request('fn_exit_dump',count=128,addr_lo='0x8002D078',addr_hi='0x8002D07C')
                original_calls=[e for e in case['allocations']['entries'] if int(e['ra'],16)==0x8004F768]
                case['native_child_allocations']=counter(r,allocation_counter)-allocations_before
                require(original_calls and case['native_child_allocations']==1,
                        'Original charged release did not allocate exactly one owned child projectile')
                case['shot_counter_observed']=any(bytes.fromhex(row['bodies'][seat])[0x98]>0 for row in case['samples']) or \
                                              bytes.fromhex(case['during']['bodies'][seat])[0x98]>0
                case['events']=r.request('spu_events',count=512)
                require(case['trace']['entries'],'Original deferred buster root was not executed')
                if seat==1:
                    # Fresh native allocations have mode0 and a zero assembly
                    # pointer until original4F1FC initializes them. Qualify the
                    # pointer only after that original initialization ran.
                    require(all(0x9f000000<=struct.unpack_from('<I',bytes.fromhex(e['bytes']),60)[0]<0x9f100000
                                for row in case['samples'] for e in row['effects']
                                if (b:=bytes.fromhex(e['bytes']))[2]==kind and b[4]),
                            'P2 charge emitter retained campaign character assembly')
                if repetition==0:r.screenshot(args.out/f'{phase}-released.png')
                wait(lambda:not r.players()[seat][0x98] and not effects(),'Native shot counter or emitter leaked')
                case['after']=snapshot(r);r.request('fn_disable')
                require(bytes.fromhex(case['after']['bodies'][other])[0x98]==0,
                        'Projectile expiry changed the other hero shot counter')
                require(r.lifecycle()[:2]==(0,0) and not r.state()['failed'],'Combat damaged co-op state')
                print('PASS',case['name'],'native charge, owner and expiry',flush=True)
        report['counters']=r.request('mod_counters')['counters'];report['success']=True
    except Exception:
        report['error']=traceback.format_exc()
        try:
            report['failure']=snapshot(r);r.screenshot(args.out/'failure.png')
        except Exception:report['capture_error']=traceback.format_exc()
        raise
    finally:
        try:
            r.request('fn_disable');r.release();r.resume_native_pause();r.fixture(12,hp=0)
            if r.read(PLAY+1,1)==b'\x00':r.tap(8,.10)
        except Exception:report['cleanup_error']=traceback.format_exc()
        finally:(args.out/'report.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':main()
