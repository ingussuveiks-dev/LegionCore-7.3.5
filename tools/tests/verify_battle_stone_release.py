"""Check actual effect204, prelaunch admission and display opcode consumers."""
from pathlib import Path
import re
import runpy
import struct
p = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n = p['native']
def needs(method, *dependencies):
    for dependency in dependencies:
        assert n['symbol'](dependency) in n['calls'](method), (method, dependency)
effect = '?EffectUpgradeBattlePet@Spell@@'
check = '?CheckBattlePetUpgrade@Spell@@'
needs('?CheckCast@Spell@@', check)
needs(effect, check, '?UpdateStats@BattlePet@@', '?DestroyItemCount@Player@@QEAAXPEAVItem@@AEAI_N', '?SendBattlePetUpdates@WorldSession@@')
assert not any(n['resolve'](a) in n['calls'](effect) for name,a in n['symbols'].items() if name.startswith('?DestroyItem@Player@@'))
needs(check, '?GetSummonedBattlePet@Player@@', '?GetBattlePet@Player@@')
mapping = (n['args'].release/'worldserver.map').read_text(errors='replace')
match = re.search(r'\?SpellEffects@@\S+\s+([0-9a-fA-F]{16})\s', mapping)
assert match
pointer = struct.unpack_from('<Q', n['binary'], n['offset'](int(match[1],16)) + 204*8)[0]
assert n['resolve'](pointer) == n['symbol'](effect)
handler = '?HandleBattlePetUpdateDisplayNotify@WorldSession@@'
call = '?Call@?$PacketHandler@VNullCmsg@BattlePet@WorldPackets@@$1?HandleBattlePetUpdateDisplayNotify@WorldSession@@'
register = '??$ValidateAndSetClientOpcode@P8WorldSession@@EAAXAEAVNullCmsg@BattlePet@WorldPackets@@@Z$1?HandleBattlePetUpdateDisplayNotify@1@'
needs('?Initialize@OpcodeTable@@', register)
needs(call, handler)
needs(handler, '?GetBattlePet@Player@@', '?GetSummonedBattlePet@Player@@', '?SendBattlePetUpdates@WorldSession@@')
print('PASS actual effect204 -> guarded upgrade/stat/one-unit consumers, native CheckCast admission and registered typed display-notification callback with authoritative own journal/summon lookup. Client notification payload/rendering remains a retest.')
