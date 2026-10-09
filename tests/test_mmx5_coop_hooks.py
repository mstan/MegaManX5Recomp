"""Prevent rejected duplicate hooks and uninstrumented native boundaries."""
from pathlib import Path
import re
import tomllib

root = Path(__file__).resolve().parents[1]
source = (root/'src/mods/mmx5_coop_plugin.c').read_text(encoding='utf-8')
addresses = [int(value, 16) for value in re.findall(
    r'psx_mod_register_function_(?:entry|filter)_plugin\(ID,\s*(0x[0-9a-fA-F]+)', source)]
assert len(addresses)==len(set(addresses)), 'Framework rejects multiple hooks for one plugin/address'
with (root/'game.toml').open('rb') as stream:
    configured = {int(value, 16) for value in tomllib.load(stream)['recompiler']['mod_function_entry_funcs']}
assert set(addresses)==configured, 'Configured native hook sites and registered callbacks disagree'
instructions={int(value,16) for value in re.findall(
    r'psx_mod_register_instruction_plugin\(ID,\s*(0x[0-9a-fA-F]+)',source)}
with (root/'game.toml').open('rb') as stream:
    configured_instructions={int(value,16) for value in tomllib.load(stream)['recompiler'].get('mod_instruction_sites',[])}
assert instructions==configured_instructions, 'Instruction hooks lack generated coverage'
print('X5 co-op hook registration and codegen coverage pass')
