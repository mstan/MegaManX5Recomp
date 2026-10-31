"""Exercise an actual original area exit with either hero and retained seats.

Original BIN member2, 800EE9A8..800EE9D8 requests the exit after PLAYER.x>5408.
Original 80020650..80020688 increments section and selects the native loader.
Position/contact protection are setup bypasses; no exit/progression flag is set.
"""
import argparse
import hashlib
import json
from pathlib import Path
import time
import traceback

import psutil
from coop_debug import Runtime, PLAY
from audit_mmx5_coop_locations import counter, snapshot, wait_ready


def wait(predicate,label,seconds=20):
    deadline=time.monotonic()+seconds
    while not predicate():
        if time.monotonic()>deadline:raise TimeoutError(label)
        time.sleep(.025)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',type=int)
    parser.add_argument('--session',type=Path,help='Verify the owned PID, executable hash and debug port')
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--case',choices=('p1_leads','p2_leads','p1_fallen','p2_fallen','p2_absent'),required=True)
    parser.add_argument('--trace-exit',action='store_true',help='Capture original exit-controller call context')
    args=parser.parse_args()
    session=None
    if args.session:
        session=json.loads(args.session.read_text())
        executable=Path(session['command'][0]).resolve()
        if Path(psutil.Process(session['pid']).exe()).resolve()!=executable:
            raise RuntimeError('Not the owned process')
        if hashlib.sha256(executable.read_bytes()).hexdigest()!=session['executable_sha256']:
            raise RuntimeError('Owned binary changed')
        if args.port is not None and args.port!=session['port']:
            parser.error('--port differs from the owned session')
        args.port=session['port']
    if args.port is None:
        parser.error('--session or --port is required')
    args.out.mkdir(parents=True,exist_ok=False)
    r=Runtime(args.port)
    if r.request('netplay_status')['active']:raise RuntimeError('Direct setup fixtures are offline only')
    report=dict(success=False,human_validation=False,case=args.case,
                source_evidence='BIN2 800EE9A8..800EE9D8; EXE80020650..80020688',
                bypassed_prerequisites=['native stage2/area0/checkpoint1 load',
                                       'position beside authored exit','native contact grace held'])
    if session:
        report.update(session=str(args.session.resolve()),executable_sha256=session['executable_sha256'],
                      resident_loading=session.get('resident_loading',True))
    try:
        r.resume_native_pause();r.release()
        r.fixture(11,hp=0);r.fixture(12,hp=3)
        previous=counter(r,'mmx5.coop.enrolled')
        r.load_location(2,0,1);wait_ready(r,previous)
        r.release()
        if args.trace_exit:
            r.request('fn_clear');r.request('fn_filter',lo='0x800EE988',hi='0x800EE98C')
        if args.case.endswith('fallen'):
            victim=0 if args.case=='p1_fallen' else 1
            r.fixture(1,seat=victim)
            wait(lambda:r.lifecycle()[victim]==5,'Requested native death did not finish')
        if args.case=='p2_absent':
            r.fixture(2,seat=1,hp=17,ammo=73)
            r.input(1,1);wait(lambda:r.lifecycle()[1]==2,'Native withdrawal did not finish');r.input(0,1)
        leader=1 if args.case in ('p2_leads','p1_fallen') else 0
        # Keep the waiting actor on its native checkpoint platform. The open
        # space near x5220 is a pit, so moving it there would confound a
        # two-living-actor leadership check with an unintended death.
        r.teleport(leader,5376,384)
        split=any(row['name']=='mmx5.coop.split-state' for row in r.request('mod_counters')['counters'])
        if not split and r.lifecycle()[:2]==(0,0):
            # Shared-camera motion intentionally limits separation. Keep the
            # waiting hero below the exit threshold on the same approach.
            r.teleport(leader^1,5376,384)
            report['bypassed_prerequisites'].append('waiting actor positioned on the Unified approach')
        report['before']=snapshot(r)
        if r.lifecycle()[leader]!=0:raise AssertionError('Area-exit setup killed its leader before the control check')
        flags=r.read(PLAY+0x4C,8)
        retries=r.read(PLAY+0x45,1)
        retained=r.players()[1]
        r.screenshot(args.out/'before.png')
        previous=counter(r,'mmx5.coop.enrolled')
        r.input(0x20,leader)
        wait(lambda:r.read(PLAY+12,2)==b'\x02\x01','Original exit did not advance stage2 to area1',15)
        r.release()
        # Observe a new enrollment, original control release and counterpart
        # arrival. An absent partner may legitimately remain absent.
        deadline=time.monotonic()+75
        while True:
            play=r.read(PLAY,0x20)
            if play[0]==10 and not play[1] and not play[0x1C] and counter(r,'mmx5.coop.enrolled')>previous:
                if r.lifecycle()[1] not in (1,3,4):break
            if time.monotonic()>deadline:raise TimeoutError('Native destination arrival did not release control')
            if play[0]==10 and play[0x1C]:r.tap(0x4000,.025)
            time.sleep(.05)
        report['after']=snapshot(r)
        if [row['character'] for row in report['after']['players']]!=[row['character'] for row in report['before']['players']]:
            raise AssertionError('Area transfer changed the campaign/counterpart roster')
        if r.read(PLAY+0x4C,8)!=flags:raise AssertionError('Area transfer changed boss completion flags')
        if r.read(PLAY+0x45,1)!=retries:raise AssertionError('Survivor area transfer consumed a team retry')
        if r.state()['failed']:raise AssertionError('Co-op failed during actual native area transfer')
        if args.case.endswith('fallen') and r.lifecycle()[:2]!=(0,0):
            raise AssertionError('Forward area did not recover the fallen teammate')
        if args.case=='p2_absent' and r.lifecycle()[1]!=2:
            raise AssertionError('Forward area rejoined a deliberately absent teammate')
        if args.case=='p2_absent' and (r.players()[1][0x5C]!=retained[0x5C] or
                                     r.players()[1][0xA8:0xC8]!=retained[0xA8:0xC8]):
            raise AssertionError('Forward area changed the withdrawn teammate resources')
        r.screenshot(args.out/'after.png')
        report['success']=True
        print('PASS actual native area exit:',args.case,flush=True)
    except Exception:
        report['error']=traceback.format_exc();report['failure']=snapshot(r)
        r.screenshot(args.out/'failure.png');raise
    finally:
        try:
            if args.trace_exit:
                report['exit_trace']=r.request('fn_entry_dump',count=64)
                report['exit_trace_stats']=r.request('fn_stats');r.request('fn_disable')
            r.release();r.resume_native_pause();r.fixture(12,hp=0)
            if r.read(PLAY+1,1)==b'\x00':r.tap(8,.05)
        finally:
            (args.out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')


if __name__=='__main__':main()
