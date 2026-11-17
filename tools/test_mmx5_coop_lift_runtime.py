"""Qualify the first authored stage8 support with overlapping native checks.

Mission/checkpoint setup and HP refills are explicit bypasses. Native input
must admit the lift, release its script, carry both heroes and finish motion.
No support phase, contact, camera limit, control or success flag is authored.
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
from test_mmx5_coop_snapshot_runtime import state_operation


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--continue-approach',action='store_true',help='Continue the same native CP1 approach after a harness assertion; no reload')
    parser.add_argument('--survivor-retry',action='store_true',help='Distinct ownership phases: P1 fallen before trigger, actual wipe/retry, then P1 fallen during motion')
    parser.add_argument('--boarding-seat',type=int,choices=(0,1),default=1,
                        help='Seat that boards first; the other must own its native walk when it joins')
    args=parser.parse_args()
    parent=json.loads(args.session.read_text(encoding='utf-8'))
    exe=Path(parent['command'][0]).resolve()
    assert Path(psutil.Process(parent['pid']).exe()).resolve()==exe
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==parent['executable_sha256']
    r=Runtime(parent['port']);assert not r.request('netplay_status')['active']
    r.release=lambda:[r.input(0,seat) for seat in (0,1)]
    args.out.mkdir(parents=True,exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    report=dict(success=False,session=str(args.session.resolve()),candidate_sha256=parent['executable_sha256'],
                samples=[],checks={},bypasses=['Original fresh mission8/0/CP1 loader; preceding route bypassed',
                'HP refill both; original contact/hurt/pit/crush remain enabled'],
                limits='First class12/sub0 support only. Later subtypes, timed supports, negative carry commands, actual stage exit and netplay remain unverified.')

    def save():
        (args.out/'report.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())

    def xy(body):
        return struct.unpack_from('<ii',body,8)[0]/65536,struct.unpack_from('<ii',body,8)[1]/65536

    def sample(label):
        s=r.personal_effects();pool=r.read(0x800C6A10,64*0xA4)
        objects=[(0x800C6A10+i,pool[i:i+0xA4]) for i in range(0,len(pool),0xA4)
                 if pool[i] and pool[i+1:i+3]==b'\x12\0']
        row=dict(label=label,personal=serial(s),parents=[dict(address=hex(a),body=b.hex()) for a,b in objects],
                 play=r.read(PLAY,0x1C0).hex(),camera=r.read(0x8009A1F8,0xFC).hex(),
                 split_cameras=r.read(r.diagnostic()+0x710,0x1F8).hex(),frame=r.request('frame')['frame'])
        report['samples'].append(row)
        assert not s['state']['failed'],'Plugin failed'
        return row,s,objects[0][1] if objects else None

    def until(label,predicate,seconds=20):
        end=time.monotonic()+seconds
        while True:
            row,s,obj=sample(label)
            if predicate(s,obj):return row,s,obj
            assert time.monotonic()<end,label
            time.sleep(.035)

    def ticks(n,label):
        start=r.state()['world_ticks']
        return until(label,lambda s,o:s['state']['world_ticks']>=start+n)

    def healthy(s,obj):
        assert s['life'][:2]==[0,0] and obj is not None,'Lost live pair/support'

    def board_pair():
        leading=args.boarding_seat;trailing=leading^1
        row,s,obj=sample('pair-boarding-before');first=xy(s['bodies'][trailing])
        r.input(0x20,leading)
        try:row,s,obj=until('first-rider-native-top',lambda s,o:o is not None and o[5]==2 and
            xy(s['bodies'][leading])[0]>=2112 and s['bodies'][leading][0x71]&8)
        finally:r.release()
        support_xy=xy(obj);start=s['state']['world_ticks']
        # Diagnostics are also published before the shared solid pass, when
        # native terrain work has cleared71 for this frame. Require a fresh
        # real contact after the waiting interval, rather than sampling that
        # intermediate body boundary as if it were the completed world pass.
        row,s,obj=until('lone-rider-native-wait',lambda s,o:
            s['state']['world_ticks']>=start+24 and s['bodies'][leading][0x71]&8)
        healthy(s,obj)
        assert obj[5]==2 and xy(obj)==support_xy and s['scene']==[0,0],'Down lift departed before both riders boarded'
        assert xy(s['bodies'][trailing])==first and s['bodies'][trailing][0x70]&8,'Non-rider original floor was lost'
        assert s['bodies'][leading][0x71]&8,'Lone rider lost real support'
        report['checks']['lone_rider_wait_and_nonrider_floor']=dict(status='passed',sample=row)
        save();r.input(0x20,trailing)
        try:row,s,obj=until('both-native-top-admission',lambda s,o:o is not None and o[5]==5 and
            all(b[0x71]&8 for b in s['bodies']))
        finally:r.release()
        healthy(s,obj);return row,s,obj

    try:
        r.release();r.resume_native_pause()
        assert r.read(PLAY,1)==b'\x0a','Requires a native entered mission'
        r.fixture(11,hp=3);r.fixture(12,hp=0)
        if not args.continue_approach:
            previous=counter(r,'mmx5.coop.enrolled');r.load_location(8,0,1);wait_ready(r,previous)
        else:
            assert r.read(PLAY+12,2)==b'\x08\0' and r.read(PLAY+0x1D,1)==b'\x01'
            report['continued_same_native_approach']=True
        row,s,obj=until('checkpoint-settle',lambda s,o:s['life'][:2]==[0,0] and
                        all(b[4:6]==b'\x01\x02' and b[0x70]&8 for b in s['bodies']))
        assert sorted(b[2] for b in s['bodies'])==[0,1],'Expected X/Zero pair'
        zero_seat=next(i for i,b in enumerate(s['bodies'])if b[2]==1)
        x_seat=zero_seat^1
        assert xy(s['bodies'][0])[0]<1944 and s['scene']==[0,0]
        report['entry']=row;r.screenshot(args.out/'checkpoint.png');save()
        if args.survivor_retry:
            retries=r.read(PLAY+0x45,1)[0];flags=r.read(PLAY+0x4C,8)
            r.fixture(11,hp=2);r.fixture(1,seat=0)
            row,s,obj=until('p1-native-death-before-trigger',lambda s,o:s['life'][0]==5 and s['life'][1]==0)
            report['before_trigger_fatal']=row;save()
            r.input(0x20,1)
            try:row,s,obj=until('surviving-p2-native-trigger',lambda s,o:o is not None and o[4]==1 and o[5]==5)
            finally:r.release()
            assert s['scene'][0]==2 and s['life'][0]==5 and s['life'][1]==0
            report['survivor_admission']=row
            row,s,obj=until('survivor-native-release-and-carry',lambda s,o:o is not None and o[6]==3 and s['bodies'][1][0x71]&8)
            start_y=xy(obj)[1]
            row,s,obj=until('survivor-actual-motion',lambda s,o:o is not None and xy(o)[1]>=start_y+24)
            assert s['life'][:2]==[5,0] and r.read(PLAY+0x45,1)[0]==retries and r.read(PLAY+0x4C,8)==flags
            report['checks']['p2_starts_and_rides_with_p1_fallen']=dict(status='passed',sample=row)
            r.screenshot(args.out/'survivor-start-motion.png');save()
            enrolled=counter(r,'mmx5.coop.enrolled');r.fixture(1,seat=1)
            row,s,obj=until('actual-native-team-wipe-retry',lambda s,o:counter(r,'mmx5.coop.enrolled')>enrolled and
                s['life'][:2]==[0,0] and all(b[4:6]==b'\x01\x02' and b[0x70]&8 for b in s['bodies']),seconds=80)
            assert r.read(PLAY+12,2)==b'\x08\0' and r.read(PLAY+0x1D,1)==b'\x01'
            assert r.read(PLAY+0x45,1)[0]==((retries-1)&255) and r.read(PLAY+0x4C,8)==flags
            report['checks']['actual_native_retry']=dict(status='passed',sample=row,before_retries=retries,after_retries=(retries-1)&255)
            r.fixture(11,hp=3);save()
            row,s,obj=board_pair()
            row,s,obj=until('fresh-pair-top-contacts-after-retry',lambda s,o:o is not None and o[6]==3 and
                s['scene']==[0,0] and all(b[0x71]&8 for b in s['bodies']))
            healthy(s,obj)
            report['checks']['fresh_pair_support_after_retry']=dict(status='passed',sample=row)
            r.screenshot(args.out/'retry-both-carry.png');save()
            r.fixture(11,hp=2);r.fixture(1,seat=0)
            row,s,obj=until('p1-native-death-during-motion',lambda s,o:s['life'][0]==5 and s['life'][1]==0)
            assert obj is not None and obj[5]==5 and obj[6]==3
            report['during_motion_fatal']=row;save()
            row,s,obj=until('survivor-native-destination',lambda s,o:o is not None and o[5]==8,seconds=100)
            assert s['life'][:2]==[5,0] and xy(obj)[1]>=2128 and s['bodies'][1][0x71]&8
            assert r.read(PLAY+0x45,1)[0]==((retries-1)&255) and r.read(PLAY+0x4C,8)==flags
            report['checks']['survivor_descent_destination']=dict(status='passed',sample=row)
            y0=xy(s['bodies'][1])[1];r.input(0x4000,1)
            try:row,s,obj=ticks(8,'survivor-native-destination-jump')
            finally:r.release()
            assert xy(s['bodies'][1])[1]<y0-2,'Survivor control did not recover'
            report['checks']['survivor_native_control']=dict(status='passed',sample=row)
            r.screenshot(args.out/'survivor-destination-jump.png');report['success']=True
        else:
            before=counter(r,'mmx5.coop.p2-descending-support-triggers')
            row,s,obj=board_pair()
            # Both refreshed rider contacts may first be observable after
            # the original stand lock releases. Retain the actual admission
            # observed during this same approach instead of requiring it now.
            assert obj[5]==5 and any(v['personal']['scene'][0] in(1,2) and v['parents'] and
                bytes.fromhex(v['parents'][0]['body'])[5]==5 for v in report['samples']), 'Native shared admission absent'
            report['checks']['both_riders_native_trigger']=dict(status='passed',sample=row,
                p2_adapter_admissions=counter(r,'mmx5.coop.p2-descending-support-triggers')-before)
            r.screenshot(args.out/'p2-trigger.png');save()
            if args.boarding_seat==0:
                assert s['scene'][0]==2,'P2 entrant did not own its original walk'
                report['p2_walk_start']=row
                walk_x=xy(s['bodies'][1])[0];walk_world=s['state']['world_ticks']
            row,s,obj=until('native-shake-release',lambda s,o:o is not None and o[5]==5 and o[6]==3)
            if args.boarding_seat==0:
                delta=xy(s['bodies'][1])[0]-walk_x
                assert delta>=40 and s['state']['world_ticks']>=walk_world+8,'Native P2 walk was skipped'
                report['checks']['p2_native_walk']=dict(status='passed',delta_x=delta,sample=row)
            healthy(s,obj);report['native_release']=row;save()
            row,s,obj=until('passenger-native-return',lambda s,o:s['scene']==[0,0] and
                            s['life'][:2]==[0,0] and all(b[4]==1 and b[3] for b in s['bodies']))
            healthy(s,obj);assert obj[5]==5,'Passenger did not return during actual moving phase'
            report['checks']['support_passenger_return']=dict(status='passed',sample=row)
            row,s,obj=until('both-native-top-contact',lambda s,o:o is not None and o[5]==5 and
                            all(b[0x71]&8 for b in s['bodies']))
            healthy(s,obj);report['ride_start']=row;r.screenshot(args.out/'both-riding.png');save()
            report['corner_control_state']=state_operation(r,'save',5)
            save()
            initial=xy(s['bodies'][zero_seat])[1]-xy(obj)[1]
            r.input(0x4000,zero_seat)
            try:row,s,obj=ticks(8,'zero-native-jump')
            finally:r.input(0,zero_seat)
            rise=initial-min(xy(bytes.fromhex(v['personal']['bodies'][zero_seat]))[1]-
                             xy(bytes.fromhex(v['parents'][0]['body']))[1]
                             for v in report['samples'] if v['label']=='zero-native-jump' and v['parents'])
            assert rise>=3,'No native jump from actual support'
            row,s,obj=until('zero-native-reland',lambda s,o:o is not None and all(b[0x71]&8 for b in s['bodies']))
            healthy(s,obj);report['checks']['jump_and_rider']=dict(status='passed',relative_rise=rise,sample=row)
            report['charge_before']=row;r.input(0x8000,x_seat)
            try:
                row,s,obj=ticks(120,'x-native-charge')
                x_body=s['bodies'][x_seat]
                full=x_body[0x95] in (0x12,0x14) or (x_body[0xFA]==2 and x_body[0x95]==0x13)
                assert x_body[0x93]==0 and full,'Native full buster charge absent'
                report['charged']=row
            finally:r.input(0,x_seat)
            report['checks']['native_full_charge']=dict(status='passed',sample=row)
            # Full charge/release is recorded separately from actual projectile
            # travel/contact, which this receipt alone does not qualify.
            row,s,obj=ticks(20,'x-native-release')
            healthy(s,obj);report['release_after']=row;r.screenshot(args.out/'charge-after-release.png');save()
            row,s,obj=until('native-descent-stop',lambda s,o:o is not None and o[5]==8,seconds=240)
            healthy(s,obj);assert xy(obj)[1]>=2128 and s['scene']==[0,0]
            report['checks']['native_descent_destination']=dict(status='passed',sample=row)
            r.screenshot(args.out/'native-destination.png');save()
            # Native horizontal control and jump after destination, without
            # teleporting away or editing the platform/contact state.
            x0=xy(s['bodies'][0])[0];r.input(0x80,0)
            try:row,s,obj=ticks(8,'zero-destination-left')
            finally:r.input(0,0)
            assert xy(s['bodies'][0])[0]<x0-2,'Zero destination control remains locked'
            healthy(s,obj);report['checks']['destination_control']=dict(status='passed',sample=row)
            report['success']=True
    except Exception:
        report['error']=traceback.format_exc()
        try:report['failure']=sample('failure')[0];r.screenshot(args.out/'failure.png')
        except Exception:report['capture_error']=traceback.format_exc()
    finally:
        r.release();save()
    print(json.dumps({k:report.get(k) for k in ('success','error')}),flush=True)
    return 0 if report['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())
