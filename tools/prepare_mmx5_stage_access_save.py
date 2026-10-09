"""Create a separate native X5 card from a fresh offline gameplay snapshot.

The original BIN24 serializer (800F2970..2BF0), resident checksum validator
(8001DB78), native icon/title and PS1 directory format define the output.
This is a stage-access test fixture, not a completed campaign or a savestate.
Original disc data and generated cards remain local; existing cards are refused.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct


def encode_slot(play, ammo):
    if len(play)!=0x1c0 or len(ammo)!=32:
        raise ValueError('Need full native PLAY and 16 original ammo words')
    if play[0x79]>=5 or play[0x4c] or play[0xcb]:
        raise ValueError('Use fresh progression with both characters and unbeaten bosses')
    slot=bytearray(0x158)
    # Native phase1 is the post-intro Maverick selection chapter. Keep its
    # eight bosses unbeaten and both characters available for this fixture.
    slot[1]=1;slot[2]=play[0xa0]
    # +3 is the native save-screen selection flag (800D1599), zero here.
    slot[4:8]=play[0x84:0x88];slot[8:0x20]=play[0x88:0xa0]
    slot[0x20:0x24]=play[0x80:0x84];slot[0x24:0x26]=play[0x7e:0x80]
    slot[0x26:0x46]=ammo
    slot[0x4b:0x51]=play[0x4d:0x53];slot[0x52:0x58]=play[0xa3:0xa9]
    for target,source in ((0x46,0x47),(0x47,0x48),(0x48,0x53),(0x49,0x54),
                          (0x4a,0x4c),(0x51,0x4b),(0x58,0x4a),(0x59,0xa1),
                          (0x5a,0xa9),(0x5b,0xaa),(0x5c,0xab),(0x5d,0x10f),
                          (0x5e,0x110),(0x5f,0x1ba),(0xf0,0xca),(0xf1,0xcb),
                          (0x152,0x1b8),(0x153,0x1b9)):
        slot[target]=play[source]
    slot[0x59]=1  # One completed mission: the opening stage (native PLAY+A1).
    slot[0x60:0xe0]=play[0x114:0x194]
    slot[0xe0:0xe4]=play[0xac:0xb0];slot[0xe4:0xe8]=play[0x1b4:0x1b8]
    slot[0xe8:0xec]=play[0xc2:0xc6];slot[0xec:0xf0]=play[0xc6:0xca]
    slot[0xf2:0x112]=play[0xcd:0xed];slot[0x112:0x132]=play[0xed:0x10d]
    slot[0x132:0x152]=play[0x194:0x1b4]
    # Resident 1DCC8 returns a loaded game to mode3/stage select only when
    # PLAY+194 is FF. Fresh pre-intro RAM has queued event0; remove that
    # 16-byte pending-story queue using its original empty sentinel.
    slot[0x132:0x142]=b'\xff'*16
    struct.pack_into('<H',slot,0x154,(sum(slot[:0x154])+0x55)&0xffff)
    return slot


def xor_frame(frame):
    value=0
    for byte in frame[:127]:value^=byte
    return value


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',type=Path,required=True)
    p.add_argument('--bin',type=Path,required=True)
    p.add_argument('--play',type=Path,required=True)
    p.add_argument('--blank-card',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True)
    args=p.parse_args()
    if args.out.exists():p.error('Use a fresh card output; existing saves are never overwritten')
    exe=args.exe.read_bytes();archive=args.bin.read_bytes();play=args.play.read_bytes()
    if exe[:8]!=b'PS-X EXE':p.error('Original X5 EXE required')
    base,size=struct.unpack_from('<II',exe,0x18);exe=exe[2048:2048+size]
    sector,size=struct.unpack_from('<II',archive,24*8)
    overlay=archive[sector*2048:sector*2048+size]
    def original(address,size):
        return overlay[address-0x800ee970:address-0x800ee970+size]
    if struct.unpack_from('<I',overlay)[0]!=28 or original(0x800f2bf0,4)!=bytes.fromhex('5401e2a4'):
        p.error('Unexpected original save module/serializer')
    if exe[0x8001db94-base:0x8001db98-base]!=bytes.fromhex('54010224'):
        p.error('Unexpected original slot checksum range')
    filename=original(0x800ee978,16).split(b'\0')[0]
    if filename!=b'BASLUS-01334':p.error('Wrong game region/save filename')
    ammo=exe[0x8006fae4-base:0x8006fb04-base]
    slot=encode_slot(play,ammo)
    card=bytearray(args.blank_card.read_bytes())
    if len(card)!=131072 or card[:2]!=b'MC':p.error('Need a formatted 128KB PS1 card')
    if any(card[n*128]!=0xa0 for n in range(1,16)):
        p.error('Template card must be empty; occupied cards are never modified')
    payload=bytearray(8192)
    payload[:4]=b'SC\x13\x01'
    # Original 800F2D0C title, 3988 palette and 3A14 three icon frames.
    payload[4:27]=original(0x800ee9a0,23)
    payload[0x60:0x80]=original(0x800f51f8,32)
    payload[0x80:0x200]=original(0x800f5218,384)
    for n in range(8):payload[0x200+n*0x158]=0xff
    payload[0x200:0x358]=slot
    directory=bytearray(128)
    directory[0]=0x51
    struct.pack_into('<I',directory,4,8192)
    directory[8:10]=b'\xff\xff';directory[10:10+len(filename)]=filename
    directory[127]=xor_frame(directory)
    card[128:256]=directory;card[8192:16384]=payload
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_bytes(card)
    report=dict(stage_access_fixture=True,human_validation=False,filename=filename.decode(),
                card_sha256=hashlib.sha256(card).hexdigest(),slot_checksum=struct.unpack_from('<H',slot,0x154)[0],
                native_phase=slot[1],defeated_bosses=slot[0x4a],zero_flags=slot[0xf1],
                source_play_sha256=hashlib.sha256(play).hexdigest(),
                load_validation='Pending original-game Load Game and stage selection')
    args.out.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(args.out)


if __name__=='__main__':main()
