"""Controlled native Ride Chaser sequence on an owned offline development run.

Original stage-entry modes5/6 and loader modes7/8/9 load stage5/section0 from the disc. This is a
sequence fixture, not menu progression, complete traversal or human acceptance.
The fatal fixture changes only the victim's rider/car HP; native death/retry
logic runs normally.
"""
import argparse
import json
from pathlib import Path
import struct
import time

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
    return dict(state=r.state(), life=r.lifecycle(), play=r.read(PLAY,32).hex(),
                players=[b.hex() for b in r.players()], cars=[c.hex() for c in r.cars()])


def wait_driving(r):
    # Arrival mode0 moves automatically. Establish original grounded driving
    # mode1 before using movement/jump as survivor-control evidence.
    deadline=time.monotonic()+12
    while True:
        require(r.lifecycle()[:2]==(0,0),'A rider died before the driving-control window')
        cars=r.cars()
        if all(c[4]==1 and c[5]==1 and c[0x70]&8 for c in cars):
            return
        require(time.monotonic()<deadline,'Native cars never reached grounded driving mode')
        time.sleep(.015)


def load_stage(r):
    # Close the requesting seat's native menu before new disc graphics load.
    # In particular, restore P2's private menu texture pages in the old stage.
    r.resume_native_pause()
    r.release()
    enrolled_before=next(c['count'] for c in r.request('mod_counters')['counters']
                         if c['name']=='mmx5.coop.enrolled')
    # Use original 20B80/title entry too: it records the selected mission at
    # PLAY+26/+27 before loading the title overlay and returning to mode7.
    for offset, value in ((12,5),(13,0),(1,0),(2,0),(3,0),(0,5)):
        r.request('write_ram',addr=hex(PLAY+offset),val=hex(value))
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
    parser.add_argument('--port',type=int,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    args.out.mkdir(parents=True,exist_ok=True)
    r=Runtime(args.port)
    require(not r.request('netplay_status')['active'],'Car fixtures are offline only')
    report=dict(controlled_stage_fixture=True,human_validation=False,success=False,cases=[])
    try:
        for victim in (1,0):
            load_stage(r)
            report['cases'].append(dict(victim=victim,loaded=snapshot(r)))
            case=report['cases'][-1]
            r.screenshot(args.out/f'cars-{victim}.png')
            if victim==1:
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
            wait_driving(r)
            case['driving']=snapshot(r)
            r.screenshot(args.out/f'driving-{victim}.png')
            lives=r.read(PLAY+0x45,1)[0]
            require(0<lives<128,'Use a fresh owned run with native retries remaining')
            survivor=victim^1
            r.fixture(1,seat=victim)
            wait_for(lambda:r.lifecycle()[victim] in (4,5))
            require(r.lifecycle()[survivor]==0 and not r.lifecycle()[2], 'One car death ended the team sequence')
            require(r.read(PLAY+0x45,1)[0]==lives,'One car death consumed a retry')
            require(r.cars()[survivor][0] and r.cars()[survivor][4]==1,'Survivor lost its native car')
            before=r.state()
            car=r.cars()[survivor]
            x,y=struct.unpack_from('<ii',car,8)
            r.tap(0x4000,.08,survivor)
            after=r.state()
            require(after['world_ticks']>before['world_ticks'],'Survivor world stopped')
            require(after['native_timer']-before['native_timer']==after['world_ticks']-before['world_ticks'],
                    'Car context advanced the world timer twice')
            require(struct.unpack_from('<i',r.cars()[survivor],8)[0]>x,'Surviving car stopped moving')
            require(struct.unpack_from('<i',r.cars()[survivor],12)[0]<y,'Survivor lost native jump control')
            case['survivor']=snapshot(r)
            r.screenshot(args.out/f'survivor-{survivor}.png')
            r.fixture(1,seat=survivor)
            wait_for(lambda:r.read(PLAY+0x45,1)[0]!=lives)
            require(r.read(PLAY+0x45,1)[0]==lives-1,'Two car deaths did not consume one shared retry')
            case['retries']=[lives,r.read(PLAY+0x45,1)[0]]
            print('PASS car death order',victim,'retry',case['retries'],flush=True)
        report['success']=True
        print('PASS native cars: sequence Select lock, both death orders, survivor control and shared retry',flush=True)
    except Exception as error:
        report['error']=str(error)
        raise
    finally:
        r.release()
        report['final']=snapshot(r)
        # A successful last-death case ends inside the original retry loader.
        # Its disabled display is expected and must not discard the report.
        try:
            r.screenshot(args.out/'final.png')
        except RuntimeError as error:
            if 'display disabled' not in str(error):
                raise
            report['final_capture']='Native reload display disabled'
        (args.out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
        # Native pause, owned by whichever seat is still alive.
        if r.read(PLAY,2)==b'\x0a\x00':r.tap(8,.05,0 if r.lifecycle()[0]==0 else 1)


if __name__=='__main__':
    main()
