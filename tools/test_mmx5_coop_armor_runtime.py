"""Controlled original-engine Ride Armor checks on an owned offline process.

This explicitly writes a stage/section fixture, then lets original entry
modes 5/6 and loader modes 7/8/9 load disc graphics/code and initialize the stage. It does not
claim native menu progression, traversal, a boss defeat, or human acceptance.
The mount fixture positions a body near an EXISTING authored vehicle; original
mount gates, animation, controls and world dispatch stay native. Armor damage
and destruction remain outside this check.
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

RIDES = 0x800d1dc0


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def wait_for(predicate, seconds=8):
    deadline = time.monotonic()+seconds
    while not predicate():
        if time.monotonic()>deadline:
            raise TimeoutError('Original Armor scenario did not reach its expected state')
        time.sleep(.02)


def snapshot(r):
    bodies = r.players()
    rides = [r.read(RIDES+i*0xb0, 0xb0) for i in range(2)]
    return dict(state=r.state(), life=r.lifecycle(),
                mount=[body[0xd5] for body in bodies], hp=[body[0x5c] for body in bodies],
                xy=[list(struct.unpack_from('<ii', body, 8)) for body in bodies],
                rides=[ride.hex() for ride in rides])


def load_stage(r):
    # Restore the old stage's menu pages before loading new graphics. Native
    # title entry also records mission +26/+27 and resets overlay state.
    r.resume_native_pause()
    r.release()
    enrolled_before=next(c['count'] for c in r.request('mod_counters')['counters']
                         if c['name']=='mmx5.coop.enrolled')
    # Use the provenance-recorded fixture at the native world boundary. It
    # reproduces fresh mission checkpoint/spawn-policy resets before mode5.
    r.load_location(4,1,0)
    deadline = time.monotonic()+80
    while True:
        play = r.read(PLAY, 32)
        body = r.read(PLAYER, 8)  # Native RAM, not a previous stage's diagnostic copy.
        enrollments=next(c['count'] for c in r.request('mod_counters')['counters']
                        if c['name']=='mmx5.coop.enrolled')
        if (play[0] == 10 and play[12:14] == b'\x04\x01' and
                body[4] == 1 and not play[1] and not play[0x1c] and
                r.state()['enrolled'] and enrollments>enrolled_before and
                r.lifecycle()[:2]==(0,0)):
            break
        require(time.monotonic()<deadline, 'Native disc stage loader timed out')
        if play[0]==10 and play[1]==2:
            r.resume_native_pause()
        elif play[0]!=10 or play[0x1c]:
            r.tap(0x4000, .04)
        # Once control opens, stay neutral. Repeated Cross can keep Zero
        # jumping forever, so an idle-pose observation is not a loader gate.
        time.sleep(.15)
    require(r.read(0x800ee970, 4) == b'\x0a\x00\x00\x00', 'Wrong original actor overlay')
    wait_for(lambda: bool(r.read(RIDES, 1)[0]))
    ride = r.read(RIDES, 0xb0)
    require(ride[1] == 1 and ride[4:7] == b'\x01\x00\x01', 'Authored Armor is not idle')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    connection=parser.add_mutually_exclusive_group(required=True)
    connection.add_argument('--port', type=int)
    connection.add_argument('--session',type=Path,help='Verify the owned PID and exact binary before attaching')
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    session=None
    if args.session:
        session=json.loads(args.session.read_text());exe=Path(session['command'][0]).resolve()
        require(Path(psutil.Process(session['pid']).exe()).resolve()==exe,'Not the owned process')
        require(hashlib.sha256(exe.read_bytes()).hexdigest()==session['executable_sha256'],'Owned binary changed')
    r = Runtime(session['port'] if session else args.port)
    report = dict(controlled_stage_fixture=True, original_input_progression=False,
                  human_validation=False, success=False)
    if session:report.update(executable_sha256=session['executable_sha256'],pid=session['pid'],
                             session=str(args.session.resolve()),resident_loading=session.get('resident_loading',True))
    require(not r.request('netplay_status')['active'], 'Fixtures are offline only')
    try:
        load_stage(r)
        report['loaded'] = snapshot(r)
        r.screenshot(args.out/'loaded.png')

        # P1 is away. P2 overlaps the actual Armor but must stay on foot.
        r.fixture(7, seat=1)
        before = r.state()['world_ticks']
        wait_for(lambda: r.state()['world_ticks'] >= before+20)
        require(not r.players()[1][0xd5], 'P2 mounted P1-only Armor')
        require(not r.read(RIDES+0x97, 1)[0]&0x40, 'P2 occupied the world Armor')
        report['p2_overlap'] = snapshot(r)
        r.screenshot(args.out/'p2-overlap.png')

        r.fixture(7, seat=0)
        wait_for(lambda: bool(r.players()[0][0xd5]))
        require(not r.players()[1][0xd5], 'P1 mounting moved P2 into the Armor')
        report['p1_mount'] = snapshot(r)
        # The negative rider slot precedes completion of the native mount
        # animation. Controls belong to Armor mode1 after arrival mode0A ends.
        wait_for(lambda: r.players()[0][0xd5] == 0xff and r.read(RIDES+5, 1) == b'\x01')
        r.screenshot(args.out/'p1-mounted.png')
        x = struct.unpack_from('<i', r.read(RIDES+8, 4))[0]
        r.tap(0x20, .08, 0)
        require(struct.unpack_from('<i', r.read(RIDES+8, 4))[0]>x, 'P1 cannot move native Armor')
        # P2 input must not control that same vehicle. No P1 input is held.
        x = struct.unpack_from('<i', r.read(RIDES+8, 4))[0]
        r.tap(0x80, .08, 1)
        require(struct.unpack_from('<i', r.read(RIDES+8, 4))[0]>=x, 'P2 steered P1 Armor left')
        report['controls'] = snapshot(r)

        r.tap(0x4010, .08, 0)  # Native Up + Jump dismount.
        wait_for(lambda: not r.players()[0][0xd5])
        require(not r.read(RIDES+0x97, 1)[0]&0x40, 'Dismount left Armor occupied')
        report['dismount'] = snapshot(r)
        r.screenshot(args.out/'dismounted.png')
        # Test survivor ownership in a separate native load, keeping P2 on
        # its safe starting platform. Dismount before a rider HP fatal fixture:
        # Armor damage is a different native path and remains unqualified.
        load_stage(r)
        report['survivor_loaded'] = snapshot(r)
        r.screenshot(args.out/'survivor-loaded.png')
        r.fixture(7, seat=0)
        wait_for(lambda: r.players()[0][0xd5] == 0xff and r.read(RIDES+5, 1) == b'\x01')
        r.tap(0x4010, .08, 0)
        wait_for(lambda: not r.players()[0][0xd5])

        # Original fatal/body and native Armor dismount; P2 owns the one world
        # pass after P1 falls, but must not inherit P1's vehicle or controls.
        r.fixture(1, seat=0)
        wait_for(lambda: r.lifecycle()[0] == 5)
        require(r.lifecycle()[1] == 0 and not r.lifecycle()[2], 'P1 Armor death wiped living P2')
        require(not r.players()[1][0xd5], 'P2 inherited fallen P1 Armor')
        require(not r.read(RIDES+0x97, 1)[0]&0x40, 'Native Armor did not release fallen P1')
        counters = r.request('mod_counters')['counters']
        require(any(c['name']=='mmx5.coop.p1-armor-world-projections' and c['count'] for c in counters),
                'P2 survivor never exercised native P1 Armor ownership')
        before = r.state()
        x=struct.unpack_from('<i',r.players()[1],8)[0]
        r.tap(0x20,.08,1)
        after = r.state()
        require(struct.unpack_from('<i',r.players()[1],8)[0]>x,'P2 on-foot survivor lost controls')
        require(after['native_timer']-before['native_timer'] == after['world_ticks']-before['world_ticks'],
                'Vehicle routing advanced the native world timer twice')
        report['survivor'] = snapshot(r)
        report['counters'] = counters
        r.screenshot(args.out/'p2-survivor.png')
        report['success'] = True
        print('PASS original Armor: P1 controls/dismount, P2 on foot, survivor world', flush=True)
    except Exception as error:
        report['error'] = traceback.format_exc()
        raise
    finally:
        try:
            r.release();report['final'] = snapshot(r)
            r.screenshot(args.out/'final.png')
            if r.lifecycle()[1] == 0 and r.read(PLAY+1, 1) == b'\x00':r.tap(8, .05, 1)
        except Exception:report['cleanup_error']=traceback.format_exc()
        finally:(args.out/'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')


if __name__ == '__main__':
    main()
