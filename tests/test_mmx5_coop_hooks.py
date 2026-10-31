"""Prevent rejected duplicate hooks and uninstrumented native boundaries."""
from pathlib import Path
import re
import tomllib

root = Path(__file__).resolve().parents[1]
source = '\n'.join(path.read_text(encoding='utf-8') for path in (root/'src/mods').glob('mmx5_coop*.c'))
addresses = [int(value, 16) for value in re.findall(
    r'psx_mod_register_function_(?:entry|filter)_plugin\(ID,\s*(0x[0-9a-fA-F]+)', source)]
assert len(addresses)==len(set(addresses)), 'Framework rejects multiple hooks for one plugin/address'
with (root/'game.toml').open('rb') as stream:
    configured = {int(value, 16) for value in tomllib.load(stream)['recompiler']['mod_function_entry_funcs']}
assert set(addresses)<=configured, 'Co-op callbacks lack configured native hook sites'
# Co-op opening policy uses this exact native reunion/replay boundary.
assert 0x800F8170 in addresses, 'Missing intro reunion skip callback'
# Owner U8: the native intro moving-support admission must stay instrumented.
assert 0x800F65C8 in addresses, 'Missing intro structure admission callback'
# A configured hook is ineffective until the original body is regenerated.
# Inspect available main-EXE shards too: the build's existence guard alone
# accepts stale generated bodies. Overlay hooks are generated separately.
shards=list((root/'generated').glob('SLUS_013.34_full_*.c'))
if not shards and (root/'generated/SLUS_013.34_full.c').is_file():
    shards=[root/'generated/SLUS_013.34_full.c']
if shards:
    emitted=set(); functions=set()
    for shard in shards:
        body=shard.read_text(encoding='utf-8')
        emitted.update(int(value,16) for value in re.findall(
            r'psx_mod_function_entry\(cpu,\s*(0x[0-9a-fA-F]+)',body))
        functions.update(int(value,16) for value in re.findall(r'void func_([0-9a-fA-F]+)\(CPUState',body))
    # Interior aliases use their owning function's dispatch rather than a
    # distinct function prologue; they cannot establish an emitted entry hook.
    required=set(addresses)&functions
    assert required<=emitted, 'Regenerate original game code: native hook bodies are stale: '+', '.join(hex(v) for v in sorted(required-emitted))
# Renderer, loading and diagnostics have their own hooks. They share several
# addresses with co-op and must retain instrumentation after integration.
for name in ('mmx5_diagnostics.cpp', 'mmx5_widescreen_plugin.c', 'mmx5_resident.cpp', 'mmx5_alia_plugin.c'):
    text=(root/'src/mods'/name).read_text(encoding='utf-8')
    required={int(value,16) for value in re.findall(
        r'psx_mod_register_function_(?:entry|filter)_plugin\([^,]+,\s*(0x[0-9a-fA-F]+)',text)}
    assert required<=configured, f'{name}: callbacks lack generated coverage'
instructions={int(value,16) for value in re.findall(
    r'psx_mod_register_instruction_plugin\((?:ID|"mmx5\.coop"),\s*(0x[0-9a-fA-F]+)',source)}
assert 0x800EEC0C in instructions, 'Missing native intro layer command publication'
# The native form callbacks are shared by three independently active plugins.
# Their immutable tables are registered for each plugin; include those actual
# table sites instead of counting only literal ID/address registrations.
forms=(root/'src/mods/mmx5_coop_forms.c').read_text(encoding='utf-8')
for table in ('black_reads', 'ultimate_reads'):
    body=re.search(r'\b'+table+r'\[\]\s*=\s*\{(.*?)\n\};',forms,re.S)
    if body:
        sites={int(value,16) for value in re.findall(r'\{\s*(0x[0-9a-fA-F]+)[uU]?\s*,',body[1])}
        assert sites, f'{table}: form instruction table is empty'
        instructions.update(sites)
with (root/'game.toml').open('rb') as stream:
    configured_instructions={int(value,16) for value in tomllib.load(stream)['recompiler'].get('mod_instruction_sites',[])}
assert instructions==configured_instructions, 'Instruction hooks lack generated coverage'
print('X5 co-op hook registration and codegen coverage pass')
