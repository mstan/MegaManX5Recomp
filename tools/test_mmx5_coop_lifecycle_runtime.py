"""Native death/survivor/retry checks on an owned, already booted QA process.

Fatal fixtures are acknowledged at the original world boundary; all death
animation, effects, native retry consumption and checkpoint reload stay native.
They do not constitute a stage playthrough or human acceptance.
"""
import argparse
import json
from pathlib import Path
import struct
import time
from coop_debug import Runtime, PLAY


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def wait_for(runtime, predicate, timeout=12, dialogue=False):
    deadline = time.monotonic()+timeout
    while not predicate():
        require(time.monotonic()<deadline, 'Native lifecycle boundary timed out')
        if dialogue and runtime.read(PLAY+0x1C, 1)!=b'\x00':
            runtime.tap(0x4000, .05)
        time.sleep(.025)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    runtime = Runtime(args.port)
    report = dict(human_validation=False, cases=[])
    try:
        runtime.release()
        runtime.resume_native_pause()
        for first_dead in (0, 1):
            wait_for(runtime, lambda: runtime.state()['enrolled'] and runtime.lifecycle()[:2]==(0,0),
                     timeout=20, dialogue=True)
            lives = runtime.read(PLAY+0x45, 1)[0]
            require(lives>0, 'Fixture needs a native retry')
            survivor = first_dead^1
            runtime.fixture(1, first_dead)
            wait_for(runtime, lambda: runtime.lifecycle()[first_dead]==5)
            require(runtime.lifecycle()[survivor]==0, 'Surviving partner was killed')
            require(runtime.read(PLAY+0x45, 1)[0]==lives, 'One death consumed a shared retry')
            runtime.screenshot(args.out/f'p{first_dead+1}-fallen.png')
            before = runtime.players()
            clock_before = runtime.state()
            runtime.input(0x20, survivor)
            time.sleep(.15)
            runtime.input(0, survivor)
            after = runtime.players()
            clock_after = runtime.state()
            require(struct.unpack_from('<i', after[survivor], 8)[0]>
                    struct.unpack_from('<i', before[survivor], 8)[0], 'Survivor lost independent controls')
            require(clock_after['world_ticks']-clock_before['world_ticks']>5, 'Survivor world stopped')
            require(clock_after['world_ticks']-clock_before['world_ticks']==
                    clock_after['native_timer']-clock_before['native_timer'], 'Survivor world advanced twice')
            runtime.input(1, first_dead)
            time.sleep(.08)
            runtime.input(0, first_dead)
            require(runtime.lifecycle()[first_dead]==5, 'Fallen actor rejoined on Select')
            runtime.screenshot(args.out/f'p{survivor+1}-survivor.png')
            runtime.fixture(1, survivor)
            wait_for(runtime, lambda: runtime.read(PLAY+0x45, 1)[0]!=lives)
            require(runtime.read(PLAY+0x45, 1)[0]==lives-1, 'Team wipe did not consume exactly one retry')
            wait_for(runtime, lambda: runtime.state()['enrolled'] and runtime.lifecycle()[:2]==(0,0),
                     timeout=20, dialogue=True)
            require(all(actor[0x5C]&0x7f for actor in runtime.players()), 'Team failed to respawn')
            runtime.screenshot(args.out/f'team-respawn-{first_dead}.png')
            report['cases'].append(dict(first_dead=first_dead, lives_before=lives,
                                        lives_after=runtime.read(PLAY+0x45, 1)[0],
                                        before=clock_before, after=clock_after))
            print('PASS native death order', first_dead, 'retry', lives, '->', lives-1, flush=True)
        report['success'] = True
        runtime.tap(8,.05)
    finally:
        runtime.release()
        (args.out/'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')


if __name__ == '__main__':
    main()
