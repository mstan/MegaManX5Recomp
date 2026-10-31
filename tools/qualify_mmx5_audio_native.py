"""Run original X5 VAB/key-on routines in an isolated MIPS CPU.

Requires local Unicorn and owner-supplied original EXE/extracted DAT. Produces
metadata/write-footprint evidence only, never distributes audio or certifies
in-game playback. No original instruction is replaced or stubbed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from unicorn import Uc,UC_ARCH_MIPS,UC_MODE_MIPS32,UC_MODE_LITTLE_ENDIAN,UC_HOOK_MEM_WRITE
from unicorn import mips_const as mips


def asset(data,record,member):
    sector,size=struct.unpack_from('<II',data,record*8)
    chunk=data[sector*2048:sector*2048+size]
    count,total=struct.unpack_from('<II',chunk)
    if total!=len(chunk) or member>=count:raise ValueError('Invalid original DAT record')
    cursor=2048
    for index in range(count):
        kind,size=struct.unpack_from('<II',chunk,8+index*8)
        if cursor+size>len(chunk):raise ValueError('Truncated DAT member')
        if index==member:return kind,chunk[cursor:cursor+size]
        cursor+=(size+2047)&~2047
    raise ValueError('Missing original DAT member')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',type=Path,required=True)
    parser.add_argument('--dat',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    executable=args.exe.read_bytes();dat=args.dat.read_bytes()
    if executable[:8]!=b'PS-X EXE':raise ValueError('Original PS-X EXE required')
    base,size=struct.unpack_from('<II',executable,0x18)
    report=dict(executable_sha256=hashlib.sha256(executable).hexdigest(),
                dat_sha256=hashlib.sha256(dat).hexdigest(),success=False,cases=[],
                limits='Original SDK only. No SPU DSP, stage, native audio mix or audible qualification.')
    for character,record in enumerate((88,90)):
        for group,member in enumerate((10,12)):
            kind,header=asset(dat,record,member);body_kind,body=asset(dat,record,member+1)
            offset=struct.unpack_from('<I',header)[0]&0xffffff
            if kind!=6 or body_kind!=(0x20303 if group else 0x20101) or header[offset:offset+4]!=b'pBAV':
                raise ValueError('Unexpected character audio layout')
            uc=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN)
            uc.ctl_set_cpu_model(mips.UC_CPU_MIPS32_24KC)
            uc.mem_map(0,0x200000);uc.mem_map(0x1F800000,0x2000)
            uc.mem_write(base&0x1fffffff,executable[2048:2048+size])
            def write(address,payload):uc.mem_write(address&0x1fffffff,payload)
            def read(address,size):return bytes(uc.mem_read(address&0x1fffffff,size))
            def word(address,value):write(address,struct.pack('<I',value))
            writes=[]
            # This Unicorn version misexecutes a RAM store in a jump delay
            # slot when a matching memory-write hook is installed. Run original
            # RAM instructions uninstrumented; compare snapshots instead. Only
            # MMIO has a write hook, so zero-valued device writes remain visible.
            uc.hook_add(UC_HOOK_MEM_WRITE,
                        lambda engine,access,address,size,value,user:writes.append((address,size)),
                        begin=0x1F801000,end=0x1F801FFF)
            def call(address,arguments):
                for register,value in zip(('A0','A1','A2','A3'),arguments):
                    uc.reg_write(getattr(mips,'UC_MIPS_REG_'+register),value)
                uc.reg_write(mips.UC_MIPS_REG_SP,0x801F0000)
                uc.reg_write(mips.UC_MIPS_REG_RA,0x801FFF00)
                uc.emu_start(address,0x801FFF00,count=2000000)
                if uc.reg_read(mips.UC_MIPS_REG_PC)!=0x801FFF00:raise RuntimeError('Native routine exceeded instruction bound')
                return uc.reg_read(mips.UC_MIPS_REG_V0)
            bank=12+character*2+group
            write(0x801A0000,header)
            word(0x8007DCF4,1)  # Original transfer gate's idle representation.
            opened=call(0x80062688,(0x801A0000+offset,bank,0,0))
            status=read(0x800EA7A0+bank,1)[0]
            if opened!=bank or status!=2:raise AssertionError(f'Original VAB open failed: {opened}, {status}')
            write(0x800EA7A0+bank,b'\x01');call(0x8005FCC8,(0,0,0,0))
            # Select actual on-disc program/tone entries. Parser installs packed
            # program ordinals; exercise every declared tone, not just a model.
            vab=header[offset:];version=struct.unpack_from('<I',vab,4)[0]
            capacity=128 if version>=5 else 64
            tones=[];ordinal=0
            for program in range(capacity):
                count=vab[32+program*16]
                if not count:continue
                for tone in range(count):
                    at=32+capacity*16+(ordinal*16+tone)*32
                    # 5D218 returns the original tone attributes. The actual
                    # game caller16734/16738 uses attribute6 as note and5 as
                    # fine pitch, not the tone's center-note attribute4.
                    note=vab[at+6];fine=vab[at+5]
                    vag=struct.unpack_from('<H',vab,at+22)[0]
                    # Original caller supplies four stack arguments: note,
                    # fine, right gain, left gain. The first two are native
                    # tone data; full gains make the calculation observable.
                    write(0x801F0010,struct.pack('<4I',note,fine,127,127))
                    before=read(0,0x200000)
                    writes.clear()
                    result=call(0x8005D458,(20,bank,program,tone))
                    after=read(0,0x200000);touched=[]
                    for page in range(0,0x200000,4096):
                        if page==0x1EF000 or before[page:page+4096]==after[page:page+4096]:continue
                        touched.extend(page+i for i,(a,b) in enumerate(zip(before[page:page+4096],after[page:page+4096])) if a!=b)
                    allowed=((0xE96AC,0xE96B0),(0xE9F40,0xEA4A8),
                             (0xEA4B0,0xEA630),(0xEA638,0xEA67C),(0xEA760,0xEA7A0))
                    outside=[f'{address:08X}' for address in touched
                             if not any(begin<=address<end for begin,end in allowed)]
                    regs=list(struct.unpack('<8H',read(0x800EA4B0+20*16,16)))
                    tones.append(dict(program=program,tone=tone,vag=vag,result=result,
                                      regs=regs,changed_addresses=[f'{address:08X}' for address in touched],mmio_writes=writes[:],
                                      outside_expected=outside))
                    if result!=20 or outside or writes:raise AssertionError(f'Unexpected native key-on: bank{bank}, {tones[-1]}')
                ordinal+=1
            report['cases'].append(dict(character=character,group=(1,3)[group],bank=bank,
                                        header_bytes=len(header),sample_bytes=len(body),vab_offset=offset,
                                        opened=opened,parsed_status=status,tones=tones))
            print('Native bank',bank,'original tones',len(tones),'bounded writes, no MMIO',flush=True)
    report['success']=True
    args.out.parent.mkdir(parents=True,exist_ok=True)
    with args.out.open('x',encoding='utf-8') as stream:json.dump(report,stream,indent=2);stream.write('\n')


if __name__=='__main__':main()
