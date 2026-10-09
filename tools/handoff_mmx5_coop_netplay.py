"""Release owned preparation overrides and arrange the two human playtest windows."""
import ctypes as C
from ctypes import wintypes as W
import argparse
import json
from pathlib import Path
import sys
import time

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT/'tools'))
from coop_debug import Runtime
from test_mmx5_coop_netplay import require, parse_crc_log

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('directory',type=Path)
parser.add_argument('--native-state',choices=('selector','save-prompt'),default='selector')
options=parser.parse_args()
out = options.directory.resolve()
session = json.loads((out/'SESSION.json').read_text())
require(session['success'], 'Selector preparation did not succeed')
runtimes = [Runtime(p['tcp']) for p in session['peers']]
expected=b'\x04\x04\x00' if options.native_state=='selector' else b'\x0e\x01'
require(all(r.read(0x800D1C00, len(expected)) == expected for r in runtimes), 'Not at requested native menu')
user = C.WinDLL('user32', use_last_error=True)
kernel = C.WinDLL('kernel32', use_last_error=True)
kernel.OpenProcess.restype=W.HANDLE
kernel.QueryFullProcessImageNameW.argtypes=[W.HANDLE,W.DWORD,W.LPWSTR,C.POINTER(W.DWORD)]
kernel.CloseHandle.argtypes=[W.HANDLE]
for peer in session['peers']:
    handle=kernel.OpenProcess(0x1000,False,peer['pid'])
    require(bool(handle),'Owned game process is no longer running')
    try:
        name=C.create_unicode_buffer(32768);length=W.DWORD(len(name))
        require(kernel.QueryFullProcessImageNameW(handle,0,name,C.byref(length)), 'Cannot verify owned executable')
        require(Path(name.value).resolve()==Path(peer['argv'][0]).resolve(), 'PID no longer belongs to this playtest')
    finally:
        kernel.CloseHandle(handle)
user.SetProcessDpiAwarenessContext.argtypes = [W.HANDLE]
user.SetProcessDpiAwarenessContext(W.HANDLE(-4))
user.IsWindowVisible.argtypes = [W.HWND]
user.GetWindowThreadProcessId.argtypes = [W.HWND, C.POINTER(W.DWORD)]
user.GetWindowThreadProcessId.restype = W.DWORD
user.GetWindowTextW.argtypes = [W.HWND, W.LPWSTR, C.c_int]
user.GetWindowRect.argtypes = [W.HWND, C.POINTER(W.RECT)]
user.ShowWindow.argtypes = [W.HWND, C.c_int]
user.MoveWindow.argtypes = [W.HWND, C.c_int, C.c_int, C.c_int, C.c_int, W.BOOL]
user.SetWindowTextW.argtypes = [W.HWND, W.LPCWSTR]
user.SetForegroundWindow.argtypes = [W.HWND]
user.BringWindowToTop.argtypes = [W.HWND]
user.GetForegroundWindow.restype = W.HWND
user.AttachThreadInput.argtypes = [W.DWORD, W.DWORD, W.BOOL]
callback_type = C.WINFUNCTYPE(W.BOOL, W.HWND, W.LPARAM)
user.EnumWindows.argtypes = [callback_type, W.LPARAM]
owned = {p['pid']: p['seat'] for p in session['peers']}
windows = {}
@callback_type
def collect(hwnd, ignored):
    pid = W.DWORD()
    user.GetWindowThreadProcessId(hwnd, C.byref(pid))
    if pid.value in owned and user.IsWindowVisible(hwnd):
        title = C.create_unicode_buffer(512)
        user.GetWindowTextW(hwnd, title, len(title))
        if 'Mega Man X5' in title.value or 'MegaManX5' in title.value:
            windows[owned[pid.value]] = hwnd
    return True
user.EnumWindows(collect, 0)
require(set(windows) == {0, 1}, f'Missing visible owned game windows: {windows}')
rect = W.RECT()
user.SystemParametersInfoW(0x30, 0, C.byref(rect), 0)  # primary desktop work area
half = (rect.right-rect.left)//2
width = min(half-30, int((rect.bottom-rect.top-100)*4/3))
height = int(width*.75)+45
for seat, hwnd in windows.items():
    user.ShowWindow(hwnd, 9)
    x = rect.left + seat*half + (half-width)//2
    y = rect.top + max(15, (rect.bottom-rect.top-height)//2)
    require(user.MoveWindow(hwnd, x, y, width, height, True), 'Cannot arrange game window')
    label=session['peers'][seat].get('label')
    user.SetWindowTextW(hwnd, label or ('Mega Man X5 - P1 - Sony Controller' if seat == 0 else 'Mega Man X5 - P2 - Keyboard (arrows, X/Z/S)'))

for runtime in runtimes:
    runtime.request('clear_input')
time.sleep(.25)
status = [r.request('netplay_status') for r in runtimes]
pads = [r.request('pad_status') for r in runtimes]
require(all(s.get('active') == 1 and s.get('desync') == 0 for s in status), 'Netplay health failed')
require(all(p.get('host_layer') is False and p.get('override') == -1 and p.get('override_p2') == -1 for p in pads), 'Input override not released')
logs = [parse_crc_log(Path(p['directory'])/'runtime.log') for p in session['peers']]
common = sorted(set(logs[0]) & set(logs[1]))
require(all(logs[0][tick] == logs[1][tick] for tick in common), 'Netplay digest mismatch')
session.update(input_released=True, handed_off_at=time.strftime('%Y-%m-%d %H:%M:%S'),
               status=status, pad_status=pads, matched_through=common[-1], windows={k:int(v) for k,v in windows.items()})
(out/'SESSION.json').write_text(json.dumps(session, indent=2)+'\n')
# Ordinary playtests focus the keyboard peer; save-prompt QA focuses menu owner P1.
focus_seat=0 if options.native_state=='save-prompt' else 1
foreground = user.GetForegroundWindow()
foreground_thread = user.GetWindowThreadProcessId(foreground, None) if foreground else 0
this_thread = kernel.GetCurrentThreadId()
attached = False
if foreground_thread and foreground_thread != this_thread:
    attached = bool(user.AttachThreadInput(this_thread, foreground_thread, True))
try:
    user.BringWindowToTop(windows[focus_seat])
    focused = bool(user.SetForegroundWindow(windows[focus_seat]))
finally:
    if attached:
        user.AttachThreadInput(this_thread, foreground_thread, False)
print(json.dumps(dict(ready=True, pids=list(owned), keyboard_focused=focused, matched_through=common[-1], session=str(out/'SESSION.json'))))
