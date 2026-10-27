"""Controlled native Ride Chaser sequence on an owned offline development run.

Original stage-entry modes5/6 and loader modes7/8/9 load stage5/section0 from the disc. This is a
sequence fixture, not menu progression, complete traversal or human acceptance.
The fatal fixture changes only the victim's rider/car HP; native death/retry
logic runs normally.
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


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def wait_for(predicate, seconds=15):
    deadline = time.monotonic()+seconds
    while not predicate():
        if time.monotonic()>deadline:
            raise TimeoutError('Native car sequence did not reach its expected state')
        time.sleep(.015)


def snapshot(r):
    # Read both cars, riders and clocks at one diagnostic boundary. Multiple
    # TCP round trips can carry a fast car out of the short opening platform
    # while an observer is still assembling its pre-input snapshot.
    data=r.read(r.diagnostic(),0x650)
    keys=('ticks','ready','enrolled','failed','counterpart','banks','world_ticks','native_timer')
    return dict(state=dict(zip(keys,struct.unpack_from('<8I',data))),life=list(data[0x40:0x44]),
                play=r.read(PLAY,32).hex(),players=[data[0x300:0x458].hex(),data[0x100:0x258].hex()],
                cars=[data[0x480:0x530].hex(),data[0x540:0x5f0].hex()])


def wait_driving(r):
    # Arrival mode0 moves automatically. Establish original grounded driving
    # mode1 before using movement/jump as survivor-control evidence.
    deadline=time.monotonic()+12
    while True:
        require(r.lifecycle()[:2]==(0,0),'A rider died before the driving-control window')
        cars=r.cars()
        if all(c[4]==1 and c[5]==1 and c[0x70]&8 for c in cars):
            for body,car in zip(r.players(),cars):
                require(body[0x15]==car[0x15],'Native rider faces opposite its car')
                require(body[0x47]!=0xff,'Native rider kept an expired teleport frame')
            return
        require(time.monotonic()<deadline,'Native cars never reached grounded driving mode')
        time.sleep(.015)


def load_stage(r):
    # Close the requesting seat's native menu before new disc graphics load.
    # In particular, restore P2's private menu texture pages in the old stage.
    r.resume_native_pause()
    r.release()
    # A consumed retry is observed before the original loader has finished.
    # Do not inject a fresh mission into that in-flight loader. Keep native
    # driving input held through arrival, then request the next bounded case.
    if r.read(PLAY+12,1)==b'\x05' and (r.read(PLAY,1)!=b'\x0a' or r.lifecycle()[2]):
        r.input(0x20,0);r.input(0x20,1)
        # PLAY+1C is the original pause gate and remains set during this
        # playable car section. It is not a retry-arrival completion flag.
        wait_for(lambda:r.read(PLAY,2)==b'\x0a\x00' and
                 r.lifecycle()[:2]==(0,0) and all(b[4]==1 for b in r.players()),seconds=30)
    enrolled_before=next(c['count'] for c in r.request('mod_counters')['counters']
                         if c['name']=='mmx5.coop.enrolled')
    # Use original 20B80/title entry too: it records the selected mission at
    # PLAY+26/+27 before loading the title overlay and returning to mode7.
    # Retain fixture provenance and the original fresh-entry policy reset.
    r.load_location(5,0,0)
    # Original controls accelerate right through the opening drop; an idle
    # stock rider also falls here (feature-disabled comparison retained).
    r.input(0x20,0);r.input(0x20,1)
    deadline = time.monotonic()+80
    while True:
        play=r.read(PLAY,32)
        bodies=r.players()
        cars=r.cars()
        enrollments=next(c['count'] for c in r.request('mod_counters')['counters']
                        if c['name']=='mmx5.coop.enrolled')
        if (play[0]==10 and play[12:14]==b'\x05\x00' and
                r.read(PLAYER+0xd5,1)==b'\xff' and bodies[1][0xd5]==0xff and
                enrollments>enrolled_before and r.lifecycle()[:2]==(0,0) and
                all(c[0] and not c[1] and c[4]==1 and c[0x5c]&127 for c in cars)):
            break
        require(time.monotonic()<deadline,'Native Ride Chaser loader timed out')
        # This sequence's original arrival effect clears its own lock. Cross
        # here also jumps the car during arrival and can launch it into a pit.
        time.sleep(.15)
    r.input(0x20,0);r.input(0x20,1)
    require(r.read(0x800ee970,4)==b'\x0b\x00\x00\x00','Wrong native car overlay')
    require(not r.state()['failed'],'Native rider/car render failed')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    connection=parser.add_mutually_exclusive_group(required=True)
    connection.add_argument('--port',type=int)
    connection.add_argument('--session',type=Path,help='Verify the owned PID and exact binary before attaching')
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--retry-budget',action='store_true',
                        help='Seed three retries using replicated qualification input; Game Over stays unverified')
    parser.add_argument('--position-fixture',action='store_true',
                        help='Reposition the survivor onto its observed native road landing before the fatal check')
    parser.add_argument('--victim',type=int,choices=(0,1),
                        help='Isolate one initial fatal seat; the opposite order remains unverified')
    parser.add_argument('--skip-select-lock',action='store_true',
                        help='Isolate fatal checks from the independent long-Select road scenario')
    parser.add_argument('--stop-owned',action='store_true',
                        help='After recording results, stop the verified session instead of trying to pause a car')
    args=parser.parse_args()
    if args.stop_owned and not args.session:parser.error('--stop-owned requires --session')
    args.out.mkdir(parents=True,exist_ok=True)
    session=None
    if args.session:
        session=json.loads(args.session.read_text());exe=Path(session['command'][0]).resolve()
        require(Path(psutil.Process(session['pid']).exe()).resolve()==exe,'Not the owned process')
        require(hashlib.sha256(exe.read_bytes()).hexdigest()==session['executable_sha256'],'Owned binary changed')
    r=Runtime(session['port'] if session else args.port)
    require(not r.request('netplay_status')['active'],'Car fixtures are offline only')
    victims=[args.victim] if args.victim is not None else [1,0]
    report=dict(controlled_stage_fixture=True,human_validation=False,success=False,cases=[],
                requested_fatal_seats=victims,select_lock_requested=1 in victims and not args.skip_select_lock)
    if session:report.update(executable_sha256=session['executable_sha256'],pid=session['pid'],session=str(args.session.resolve()))
    def seed_retries():
        if not args.retry_budget:return
        before=int.from_bytes(r.read(r.diagnostic()+0x80,4),'little')
        # Keep acceleration held during the replicated qualification chord.
        r.input(0x20|6|(13<<8))
        try:wait_for(lambda:int.from_bytes(r.read(r.diagnostic()+0x80,4),'little')==before+1)
        finally:r.input(0x20)
        require(r.read(PLAY+0x45,1)==b'\x03','Qualification retry budget rejected')
        report.setdefault('retry_fixtures',[]).append(r.request('frame')['frame'])
        bypass='three retries seeded through qualification input; Game Over unverified'
        if bypass not in report.setdefault('bypassed_prerequisites',[]):report['bypassed_prerequisites'].append(bypass)
    try:
        for victim in victims:
            load_stage(r)
            seed_retries()
            report['cases'].append(dict(victim=victim,loaded=snapshot(r)))
            case=report['cases'][-1]
            r.screenshot(args.out/f'cars-{victim}.png')
            if victim==1 and not args.skip_select_lock:
                # Hold Select through native world updates, including any
                # native death/survivor transition; never force a safe road.
                before=r.state()['world_ticks']
                deadline=time.monotonic()+15
                r.input(0x20,0);r.input(0x21,1)
                while r.state()['world_ticks']<before+180:
                    require(time.monotonic()<deadline,'Car control window stopped')
                    require(r.lifecycle()[1] not in (1,2,3),'Rider voluntarily left during Select hold')
                    time.sleep(.02)
                r.release()
                require(r.lifecycle()[1] not in (1,2,3),'Select voluntarily despawned a car rider')
                counters=r.request('mod_counters')['counters']
                require(any(c['name']=='mmx5.coop.car-departure-blocked' and c['count'] for c in counters),
                        'Long Select hold did not exercise the explicit sequence lock')
                case['select_lock']=snapshot(r)
                r.screenshot(args.out/'select-locked.png')
                # Fresh authored road start for the independent death scenario.
                load_stage(r)
                seed_retries()
            wait_driving(r)
            case['driving']=snapshot(r)
            if not args.position_fixture:r.screenshot(args.out/f'driving-{victim}.png')
            lives=r.read(PLAY+0x45,1)[0]
            require(0<lives<128,'Use a fresh owned run with native retries remaining')
            survivor=victim^1
            if args.position_fixture:
                landing=bytes.fromhex(case['driving']['cars'][survivor])
                lx,ly=struct.unpack_from('<ii',landing,8)
                # The original car section keeps its pause-input gate set.
                # Position directly rather than waiting for a native menu.
                r.input(0x20,0);r.input(0x20,1)
                r.teleport_car(survivor,lx/65536,ly/65536)
                case['position_fixture']=dict(seat=survivor,x=lx/65536,y=ly/65536)
                report.setdefault('bypassed_prerequisites',[]).append('survivor repositioned at its observed native road landing')
            r.fixture(1,seat=victim)
            wait_for(lambda:r.lifecycle()[victim] in (4,5))
            require(r.lifecycle()[survivor]==0 and not r.lifecycle()[2], 'One car death ended the team sequence')
            require(r.read(PLAY+0x45,1)[0]==lives,'One car death consumed a retry')
            require(r.cars()[survivor][0] and r.cars()[survivor][4]==1,'Survivor lost its native car')
            before=r.state()
            wait_for(lambda:r.cars()[survivor][0x70]&8 and r.cars()[survivor][5]==1)
            car=r.cars()[survivor]
            x,y=struct.unpack_from('<ii',car,8)
            case['jump_before']=snapshot(r)
            r.input(0x4020,survivor)
            try:
                # Poll the native jump while input remains held. A brief
                # wall-clock tap and one later Y sample can miss the entire
                # jump, or compare different heights on the descending road.
                # Original BIN7 F0670..F06C0 accepts edge bit80, changes
                # mode5 to2 and sets +6.5 upward speed; movement subtracts it
                # from Y. Require native jump mode/held input as well as speed.
                def native_jump():
                    car=r.cars()[survivor]
                    require(r.lifecycle()[survivor]==0,'Survivor died before native jump observation')
                    return car[5]==2 and car[0x8a]&0x80 and struct.unpack_from('<i',car,0x24)[0]>0
                wait_for(native_jump)
                case['jump_observed']=snapshot(r)
            finally:r.input(0x20,survivor)
            after=r.state()
            require(after['world_ticks']>before['world_ticks'],'Survivor world stopped')
            require(after['native_timer']-before['native_timer']==after['world_ticks']-before['world_ticks'],
                    'Car context advanced the world timer twice')
            require(struct.unpack_from('<i',r.cars()[survivor],8)[0]>x,'Surviving car stopped moving')
            case['survivor']=snapshot(r)
            r.screenshot(args.out/f'survivor-{survivor}.png')
            require(r.lifecycle()[survivor]==0,'Survivor died before the controlled team-wipe fixture')
            r.fixture(1,seat=survivor)
            wait_for(lambda:r.read(PLAY+0x45,1)[0]!=lives)
            require(r.read(PLAY+0x45,1)[0]==lives-1,'Two car deaths did not consume one shared retry')
            case['retries']=[lives,r.read(PLAY+0x45,1)[0]]
            (args.out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
            print('PASS car death order',victim,'retry',case['retries'],flush=True)
        report['success']=True
        print('PASS requested native car fatal orders, survivor control and shared retry',flush=True)
        if report['select_lock_requested']:print('PASS native car Select lock',flush=True)
    except Exception as error:
        report['error']=traceback.format_exc()
        raise
    finally:
        try:
            r.release();report['final']=snapshot(r)
            # A successful last-death case ends inside the original retry loader.
            try:r.screenshot(args.out/'final.png')
            except RuntimeError as error:
                if 'display disabled' not in str(error):raise
                report['final_capture']='Native reload display disabled'
            # Park after the native retry, rather than leaving neutral cars
            # to fall while the next tool reads evidence. Parking is separate
            # from the recorded death/retry assertions above.
            if args.stop_owned:
                report['stop_requested']=True
            elif r.read(PLAY,1)[0] in (7,8,9) or r.lifecycle()[2]:
                r.input(0x20,0);r.input(0x20,1)
                wait_for(lambda:r.read(PLAY,2)==b'\x0a\x00' and r.lifecycle()[:2]==(0,0),seconds=30)
            if not args.stop_owned and r.read(PLAY,2)==b'\x0a\x00' and not r.read(PLAY+0x1c,1)[0]:
                r.input(0x28,0 if r.lifecycle()[0]==0 else 1)
                wait_for(lambda:r.read(PLAY+1,1)==b'\x02')
            r.release();report['parked']=snapshot(r)
        except Exception:report['cleanup_error']=traceback.format_exc()
        finally:
            if args.stop_owned:
                try:
                    process=psutil.Process(session['pid'])
                    require(Path(process.exe()).resolve()==exe,'Owned path changed before stop')
                    try:r.request('quit')
                    except (OSError,RuntimeError):pass
                    try:process.wait(timeout=10)
                    except psutil.TimeoutExpired:
                        require(Path(process.exe()).resolve()==exe,'Owned path changed before termination')
                        process.terminate();process.wait(timeout=10)
                    report['owned_stopped']=True
                except psutil.NoSuchProcess:report['owned_stopped']=True
                except Exception:report['stop_error']=traceback.format_exc()
            (args.out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')


if __name__=='__main__':
    main()
