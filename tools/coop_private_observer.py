"""Read private X5 co-op projectile state from an owned Windows QA process.

The native guest SHOTS address normally exposes P1 outside P2 projection.
This observer uses existing executable symbols and read-only process access;
it adds no game instrumentation and never writes process memory. Callers must
validate their owned session's process path, executable hash and offline mode.
"""
import ctypes
from ctypes import wintypes
import os
from pathlib import Path
import re
import struct
import subprocess
import time


class PrivateObserver:
    def __init__(self,pid,exe,nm='C:/msys64/mingw64/bin/nm.exe'):
        if os.name!='nt':
            raise RuntimeError('This QA observer requires Windows read-only process access')
        exe=Path(exe).resolve()
        symbols=subprocess.check_output([str(nm),'--defined-only',str(exe)],encoding='utf-8',errors='replace')
        addresses={}
        for name in ('second','world_ticks','projected','busy'):
            matches=re.findall(r'^([0-9a-fA-F]+) [bBdD] '+name+'$',symbols,re.MULTILINE)
            if len(matches)!=1:raise RuntimeError('Expected one exact co-op symbol: '+name)
            addresses[name]=int(matches[0],16)
        with exe.open('rb') as f:
            f.seek(0x3c);pe=struct.unpack('<I',f.read(4))[0];f.seek(pe)
            if f.read(4)!=b'PE\0\0':raise RuntimeError('Not a PE executable')
            f.seek(pe+24)
            if struct.unpack('<H',f.read(2))[0]!=0x20b:raise RuntimeError('Expected native PE32+ QA executable')
            f.seek(pe+48);image_base=struct.unpack('<Q',f.read(8))[0]
        self.kernel=ctypes.WinDLL('kernel32',use_last_error=True)
        psapi=ctypes.WinDLL('psapi',use_last_error=True)
        self.kernel.OpenProcess.argtypes=[wintypes.DWORD,wintypes.BOOL,wintypes.DWORD]
        self.kernel.OpenProcess.restype=wintypes.HANDLE
        self.kernel.CloseHandle.argtypes=[wintypes.HANDLE]
        self.kernel.ReadProcessMemory.argtypes=[wintypes.HANDLE,ctypes.c_void_p,ctypes.c_void_p,ctypes.c_size_t,ctypes.POINTER(ctypes.c_size_t)]
        self.kernel.ReadProcessMemory.restype=wintypes.BOOL
        self.handle=self.kernel.OpenProcess(0x410,False,pid) # QUERY_INFORMATION | VM_READ
        if not self.handle:raise ctypes.WinError(ctypes.get_last_error())
        try:
            modules=(wintypes.HMODULE*1024)();needed=wintypes.DWORD()
            psapi.EnumProcessModules.argtypes=[wintypes.HANDLE,ctypes.POINTER(wintypes.HMODULE),wintypes.DWORD,ctypes.POINTER(wintypes.DWORD)]
            psapi.GetModuleFileNameExW.argtypes=[wintypes.HANDLE,wintypes.HMODULE,wintypes.LPWSTR,wintypes.DWORD]
            if not psapi.EnumProcessModules(self.handle,modules,ctypes.sizeof(modules),ctypes.byref(needed)):
                raise ctypes.WinError(ctypes.get_last_error())
            path=ctypes.create_unicode_buffer(32768)
            if not psapi.GetModuleFileNameExW(self.handle,modules[0],path,len(path)) or Path(path.value).resolve()!=exe:
                raise RuntimeError('Read-only module identity differs from owned executable')
            self.addresses={name:int(modules[0])+address-image_base for name,address in addresses.items()}
            self.second=self.addresses['second']
        except Exception:
            self.close();raise

    def close(self):
        if getattr(self,'handle',None):self.kernel.CloseHandle(self.handle);self.handle=None

    def read(self,address,size):
        result=ctypes.create_string_buffer(size);count=ctypes.c_size_t()
        if not self.kernel.ReadProcessMemory(self.handle,address,result,size,ctypes.byref(count)) or count.value!=size:
            raise ctypes.WinError(ctypes.get_last_error())
        return result.raw

    def second_shots(self):
        # PlayerContext is fixed byte arrays: body158, shots1380, glowF0,
        # trails120, carB0. Socket requests can span whole native updates, so
        # they cannot bracket a host read during an attack. Require two equal
        # copies, a constant host world tick and idle projection guards using
        # read-only host reads instead. This is a stable private-context sample,
        # not an atomic claim about separately requested diagnostics/GPU frames.
        def guard():
            return tuple(struct.unpack('<I',self.read(self.addresses[name],4))[0]
                         for name in ('world_ticks','projected','busy'))
        for attempt in range(64):
            before=guard()
            if before[1:]==(0,0):
                raw=self.read(self.second,0x158+0x1380)
                repeated=self.read(self.second,len(raw));after=guard()
                if before==after and raw==repeated:
                    body=raw[:0x158];pool=raw[0x158:]
                    return dict(world=after[0],body=body.hex(),attempts=attempt+1,
                        observation='equal private-context copies, same host world tick, projection/busy guards zero',
                        shots=[dict(index=i,address=hex(0x80098120+i*0x9c),body=pool[i*0x9c:(i+1)*0x9c].hex()) for i in range(32) if pool[i*0x9c]])
            time.sleep(.001)
        raise RuntimeError('No stable private SHOTS read within 64 read-only host attempts')
