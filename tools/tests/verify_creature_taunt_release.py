"""Verify the native taunt iterator bound and dead-caster gate in the linked PE."""
from pathlib import Path
import bisect
import re
import runpy
import struct

native = runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
address = native['symbol']('?SelectVictim@Creature@@')
index = bisect.bisect_right(native['starts'], address-native['base'])-1
begin, end, _ = native['functions'][index]
assert begin <= address-native['base'] < end
at = native['offset'](address)
code = native['binary'][at:at+native['base']+end-address]
# Native x64 std::list head is Creature+0xC28. The old loop advanced RBX
# and dereferenced [RBX+0x10] before comparing RBX to this sentinel.
guard = re.search(rb'\x48\x8b\x1b\x48\x3b\x9f\x28\x0c\x00\x00\x0f\x84(.{4})\x48\x8b\x4b\x10\x48\x8b\x09\xe8', code, re.DOTALL)
assert guard, 'SelectVictim lacks a pre-dereference end-sentinel comparison'
branch_end = guard.start()+16
exit_offset = branch_end+struct.unpack('<i', guard[1])[0]
assert guard.end() < exit_offset < len(code), 'End iterator does not skip the aura dereference'
call = guard.end()-1
callee = native['resolve'](address+call+5+struct.unpack_from('<i', code, call+1)[0])
assert callee == native['symbol']('?GetCaster@Aura@@'), 'Guard does not protect the real aura-caster lookup'
assert re.search(rb'\x48\x85\xc0\x74.\x39\xb0\x18\x0a\x00\x00', code[call+5:call+40], re.DOTALL), 'Older taunt caster lacks a null/alive gate'
assert address in native['calls']('?UpdateVictim@CreatureAI@@'), 'Native AI does not call the repaired selector'
print('PASS: linked SelectVictim checks the list sentinel before dereference and rejects dead older taunt casters')
