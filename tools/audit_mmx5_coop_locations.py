"""Sample original X5 checkpoint locations in an owned private QA session.

This is the coverage driver, not a campaign pass. Per-location native loads,
motion, contacts and captures are reported independently; bosses, exits,
retry activation and audio stay unverified until their dedicated checks run.
The --session report must come from test_mmx5_coop_runtime --keep-running.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time
import traceback

import psutil
from coop_debug import Runtime, PLAY, PLAYER


def snapshot(runtime):
    bodies=runtime.players()
    return dict(state=runtime.state(), play=runtime.read(PLAY,0x1C0).hex(),
                lifecycle=list(runtime.lifecycle()), bodies=[body.hex() for body in bodies],
                players=[dict(character=body[2],state=body[4],action=body[5],hp=body[0x5C],
                              contact=body[0x70],x=struct.unpack_from('<i',body,8)[0]/65536,
                              y=struct.unpack_from('<i',body,12)[0]/65536) for body in bodies],
                camera=runtime.read(0x8009A1F8,0xFC).hex())


def counter(runtime,name):
    return next((row['count'] for row in runtime.request('mod_counters')['counters']
                 if row['name']==name),0)


def ground_jump_probe(runtime,seat):
    """A bounded native input/rise/landing observation, not section clearance."""
    before=snapshot(runtime)
    result=dict(status='unverified',seat=seat,before=before,arrival_samples=[],samples=[],
                limits='Only this launch point; scrolling, hazards, moving solids and route traversal remain unverified.')
    # The loader can finish before the native arrival reaches its floor. Let
    # that animation settle without moving either hero or changing its state.
    # Mounts and active scripts require their own checks, rather than an
    # automatic dismount/lock-clear here.
    arrival_tick=before['state']['world_ticks'];deadline=time.monotonic()+10
    while True:
        bodies=[bytes.fromhex(body) for body in before['bodies']]
        if any(body[0xD5] or body[0xD0] or body[0xD4] or body[0x7A] for body in bodies):break
        if all(body[0x70]&8 and body[4:6]==b'\x01\x02' for body in bodies):break
        if (before['lifecycle'][:2]!=[0,0] or
                before['state']['world_ticks']>=arrival_tick+60 or time.monotonic()>deadline):break
        result['arrival_samples'].append(before)
        time.sleep(.04);before=snapshot(runtime)
    result['before']=before
    bodies=[bytes.fromhex(body) for body in before['bodies']]
    if any(value in (4,5) for value in before['lifecycle'][:2]) or any(
            body[4]==2 and not (body[0x5C]&127) for body in bodies):
        result['status']='failed'
        result['reason']='A hero died before the jump input; entry survival failed, cause unqualified.'
        return result
    if any(not (body[0x70]&8) or body[4:6]!=b'\x01\x02' or body[0xD5] or
           body[0xD0] or body[0xD4] or body[0x7A] for body in bodies):
        result['reason']='Both heroes are not settled, unlocked on-foot actors; dedicated native mechanics required.'
        return result
    runtime.input(0x4000,seat)
    try:
        # Measure after submission; earlier baselines can credit pre-input ticks.
        submitted=runtime.state()['world_ticks'];deadline=time.monotonic()+20
        while True:
            sample=snapshot(runtime);result['samples'].append(sample)
            if sample['state']['world_ticks']>=submitted+6:break
            if time.monotonic()>deadline:raise TimeoutError('Native world did not advance for the jump probe')
            time.sleep(.03)
    finally:
        runtime.input(0,seat)
    initial_y=before['players'][seat]['y']
    result['rise_pixels']=initial_y-min(s['players'][seat]['y'] for s in result['samples'])
    result['other_body_stationary_during_rise']=all(
        (s['players'][seat^1]['x'],s['players'][seat^1]['y'])==
        (before['players'][seat^1]['x'],before['players'][seat^1]['y']) for s in result['samples'])
    if result['rise_pixels']<2:
        result['reason']='No clear jump rise observed; native ceiling, contact or input admission needs location-specific review.'
        return result
    deadline=time.monotonic()+15
    while True:
        sample=snapshot(runtime);result['samples'].append(sample)
        if sample['lifecycle'][:2]!=[0,0]:
            result['status']='failed';result['reason']='A hero died during the bounded native jump; cause unqualified.'
            return result
        body=bytes.fromhex(sample['bodies'][seat])
        if body[0x70]&8 and body[4:6]==b'\x01\x02':
            result['status']='passed';result['landed']=sample
            return result
        if time.monotonic()>deadline:
            result['reason']='No settled landing observed within the probe; follow the actual terrain/controller separately.'
            return result
        time.sleep(.04)


def wait_ready(runtime,previous,timeout=90):
    deadline=time.monotonic()+timeout
    while time.monotonic()<deadline:
        play=runtime.read(PLAY,0x20)
        if play[0]==10 and counter(runtime,'mmx5.coop.enrolled')>previous:
            if runtime.lifecycle()[:2]==(0,0) and not play[1] and not play[0x1C]:
                return
        # Only dismiss real native dialogue/briefing during setup. Never alter
        # dialogue, boss or room-success flags.
        if play[0] in (4,5,6,10):
            runtime.tap(0x4000,.025)
        time.sleep(.08)
    raise TimeoutError('Original location loader/control boundary timed out')


def recover_native_menu(runtime):
    """Return from a failed setup through native input and mission entry.

    No death, victory, retry or campaign flag is manufactured. Recovery is a
    separate bypass and never changes the failed location's disposition.
    """
    runtime.release()
    deadline=time.monotonic()+20
    while runtime.read(PLAY,1)[0] not in (4,13):
        if time.monotonic()>deadline:raise TimeoutError('Failed setup did not reach a recoverable native menu')
        time.sleep(.05)
    before=snapshot(runtime)
    previous=counter(runtime,'mmx5.coop.enrolled')
    receipt=int.from_bytes(runtime.read(runtime.diagnostic()+0x80,4),'little')
    runtime.input(6|(12<<8))
    deadline=time.monotonic()+8
    try:
        while int.from_bytes(runtime.read(runtime.diagnostic()+0x80,4),'little')==receipt:
            if time.monotonic()>deadline:raise TimeoutError('Native mission-entry recovery not acknowledged')
            time.sleep(.03)
    finally:runtime.input(0)
    if int.from_bytes(runtime.read(runtime.diagnostic()+0x88,4),'little')!=1:
        raise RuntimeError('Native mission-entry recovery rejected')
    wait_ready(runtime,previous)
    return dict(before=before,after=snapshot(runtime),
                bypass='qualification Grizzly mission-entry input from the original selector/Game Over')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--inventory',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--locations',help='Comma-separated original location IDs; default all')
    parser.add_argument('--continue-on-failure',action='store_true',
                        help='Retain failures and recover only through original selector/Game Over mission entry')
    parser.add_argument('--stop-owned',action='store_true',help='Stop only the verified QA PID after the audit')
    parser.add_argument('--check-ground-jump',action='store_true',
                        help='Record bounded native jump/landing checks where both heroes have a settled on-foot launch point')
    args=parser.parse_args()
    session=json.loads(args.session.read_text(encoding='utf-8'))
    executable=Path(session['command'][0]).resolve()
    process=psutil.Process(session['pid'])
    if Path(process.exe()).resolve()!=executable:
        raise RuntimeError('QA process does not match the owned session executable')
    if hashlib.sha256(executable.read_bytes()).hexdigest()!=session['executable_sha256']:
        raise RuntimeError('Owned executable changed after session launch')
    runtime=Runtime(session['port'])
    if runtime.request('netplay_status')['active']:
        raise RuntimeError('Direct offline setup fixtures are forbidden online')
    inventory_source=args.inventory.read_bytes()
    inventory=json.loads(inventory_source)
    selected=set(args.locations.split(',')) if args.locations else None
    locations=[row for row in inventory['locations'] if selected is None or row['id'] in selected]
    if selected is not None and selected!={row['id'] for row in locations}:
        raise ValueError('Requested location missing from original content inventory')
    args.out.mkdir(parents=True,exist_ok=False)
    harness_source=Path(__file__).read_bytes()
    (args.out/'harness.py').write_bytes(harness_source)
    report=dict(format_version=1,executable_sha256=session['executable_sha256'],
                harness_sha256=hashlib.sha256(harness_source).hexdigest(),
                inventory_sha256=hashlib.sha256(inventory_source).hexdigest(),
                session=str(args.session.resolve()),resident_loading=session.get('resident_loading',True),
                source_sha256=inventory['source_sha256'],character=session['character'],
                human_validation=False,success=False,locations=[],
                bypassed_prerequisites=['mission selection and intervening traversal via original loader',
                                       'explicit HP refill; native terrain/pits/crush remain active'])
    try:
        runtime.resume_native_pause();runtime.release()
        runtime.fixture(11,hp=3)
        if not int.from_bytes(runtime.read(runtime.diagnostic()+0x64,4),'little'):
            raise RuntimeError('Qualification protection fixture unavailable')
        for location in locations:
            folder=args.out/location['id'];folder.mkdir()
            result=dict(location=location,status='unverified',checks=dict(location['checks']))
            report['locations'].append(result)
            try:
                previous=counter(runtime,'mmx5.coop.enrolled')
                applied=counter(runtime,'mmx5.coop.fixture-checkpoints-applied')
                current=runtime.read(PLAY+12,2)
                continuation=(current==bytes((location['stage'],location['section'])) and runtime.lifecycle()[:2]==(0,0))
                result['load_path']='native_same_area_continuation' if continuation else 'native_mission_title'
                runtime.load_location(location['stage'],location['section'],location['checkpoint'],continuation)
                wait_ready(runtime,previous)
                play=runtime.read(PLAY,0x20)
                if (play[12],play[13])!=(location['stage'],location['section']):
                    raise AssertionError('Original loader did not retain the requested stage/section')
                receipt=int.from_bytes(runtime.read(runtime.diagnostic()+0x9C,4),'little')
                expected=location['stage']|(location['section']<<8)|(location['checkpoint']<<16)
                if receipt!=expected:
                    raise AssertionError('Original initializer did not consume the requested checkpoint')
                result['observed_checkpoint_after_native_arrival']=play[0x1D]
                result['checkpoint_changed_during_native_arrival']=play[0x1D]!=location['checkpoint']
                if counter(runtime,'mmx5.coop.fixture-checkpoints-applied')!=applied+1:
                    raise AssertionError('Requested checkpoint was not applied exactly once')
                runtime.release()
                result['load']='passed'
                result['entry']=snapshot(runtime)
                runtime.screenshot(folder/'entry.png')
                result['input_samples']=[]
                for seat in (0,1):
                    if args.check_ground_jump:
                        probe=ground_jump_probe(runtime,seat)
                        result.setdefault('ground_jump_probes',[]).append(probe)
                        if probe['status']=='failed':
                            result.setdefault('observed_failures',[]).append(probe['reason'])
                            if not args.continue_on_failure:raise AssertionError(probe['reason'])
                            # A living survivor can still use the next native
                            # location setup. Do not require a team wipe or
                            # manufacture a frontend recovery for this death.
                            break
                        continue
                    before=snapshot(runtime)
                    runtime.tap(0x4000,.10,seat)
                    after=snapshot(runtime)
                    result['input_samples'].append(dict(seat=seat,before=before,after=after))
                    runtime.screenshot(folder/f'p{seat+1}-jump.png')
                    time.sleep(.25)
                result['counters']=runtime.request('mod_counters')
                result['status']='failed' if result.get('observed_failures') else 'sampled'
                # Sampling records observations only. Stage-specific expected
                # trajectories/terrain/actors still need explicit assertions.
                print(location['id'],'loaded; '+('party loss observed, cause unqualified' if result.get('observed_failures')
                      else 'sampled; mechanics unverified'),flush=True)
            except Exception:
                result['status']='failed';result['error']=traceback.format_exc()
                try:
                    result['failure']=snapshot(runtime);runtime.screenshot(folder/'failure.png')
                except Exception:result['capture_error']=traceback.format_exc()
                if not args.continue_on_failure:raise
                try:result['recovery']=recover_native_menu(runtime)
                except Exception:
                    result['recovery_error']=traceback.format_exc();raise
            finally:
                (folder/'report.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
                (args.out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
        report['success']=all(row['status']=='sampled' for row in report['locations'])
    except Exception:
        report['error']=traceback.format_exc();print(report['error'],flush=True)
    finally:
        try:
            runtime.release()
            if runtime.read(PLAY+1,1)==b'\x00':runtime.tap(8,.05)
        except Exception:pass
        (args.out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
        if args.stop_owned and process.is_running():
            process.terminate();process.wait(timeout=15)
    return 0 if report['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())
