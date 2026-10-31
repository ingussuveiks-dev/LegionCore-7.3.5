"""Inspect actual leave dispatch and player-scoped raid cleanup in the x64 PE."""
from pathlib import Path
import re
import runpy
import struct

from sharasdal_erunak_entry_pe import sites, values

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
ins = n['instructions']
maptext = (n['args'].release / 'worldserver.map').read_text(errors='replace')


def slot(table, target):
    rows = re.findall(r'\s+\w+:\w+\s+' + re.escape(table) + r'\s+([0-9a-fA-F]{16})', maptext)
    assert len(rows) == 1, table
    entries = struct.unpack_from('<80Q', n['binary'], n['offset'](int(rows[0], 16)))
    expected = n['symbol'](target)
    found = []
    for i, entry in enumerate(entries):
        try:
            if n['resolve'](entry) == expected:
                found.append(i * 8)
        except (ValueError, IndexError, struct.error):
            pass
    assert len(found) == 1, (table, target, found)
    return found[0]


leave_slot = slot('??_7Instance@instance_dark_riders_artifact@@6B@',
                  '?OnPlayerLeave@Instance@instance_dark_riders_artifact@@')
challenge_slot = slot('??_7Instance@instance_dark_riders_artifact@@6B@',
                      '?OnPlayerLeaveForScript@InstanceScript@@')
world_slot = slot('??_7Player@@6BUnit@@@', '?RemoveFromWorld@Player@@')
body = ins('?RemovePlayerFromMap@Map@@')
map_call, = sites(n, body, '?OnPlayerLeaveMap@ScriptMgr@@')
# MSVC loads the two real InstanceScript virtual slots into r8, then calls r8.
def virtual_register_call(displacement):
    loads = [i for i, x in enumerate(body) if x.mnemonic == 'mov' and
             x.op_str == f'r8, qword ptr [rcx + {displacement:#x}]']
    index, = loads
    assert body[index + 2].mnemonic == 'call' and body[index + 2].op_str == 'r8'
    return index + 2


leave = virtual_register_call(leave_slot)
challenge = virtual_register_call(challenge_slot)
world, = [i for i, x in enumerate(body) if x.mnemonic == 'call' and
          x.op_str == f'qword ptr [rax + {world_slot:#x}]']
destroy, = [i for i, x in enumerate(body) if x.mnemonic == 'call' and x.op_str == 'rax']
assert leave < challenge < map_call < world < destroy
assert len([x for x in body if x.mnemonic == 'call' and x.op_str == 'r8']) == 2
assert 1191 not in values(body), 'Leave dispatch must not remain Ashran-only'
assert body[leave - 5].mnemonic == 'je' and body[leave - 5].operands[0].imm == body[challenge + 1].address
assert body[map_call - 2].op_str == 'rdx, rbp' and body[map_call - 3].op_str == 'r8, rsi'
assert body[world + 1].mnemonic == 'test' and body[world + 1].op_str == 'r14b, r14b', 'remove flag must branch after both callbacks'

for name in ('instance_heart_of_fear', 'instance_siege_of_orgrimmar'):
    callback = ins('?OnPlayerLeave@' + name + '_InstanceMapScript@' + name + '@@')
    assert not sites(n, callback, '?DoRemoveAurasDueToSpellOnPlayers@InstanceScript@@')
    player_aura, pet_aura = sites(n, callback, '?RemoveAurasDueToSpell@Unit@@')
    pet, = sites(n, callback, '?GetPet@Player@@')
    assert player_aura < pet < pet_aura
    assert callback[pet + 1].op_str == 'rax, rax' and callback[pet + 2].mnemonic == 'je'
    assert any(x.op_str == 'rcx, rax' for x in callback[pet + 3:pet_aura])
    if name == 'instance_heart_of_fear':
        assert sum(x.op_str == 'edx, 0x1eccf' for x in callback) == 2  # 126159
        assert callback[player_aura - 1].op_str == 'rcx, rbx'
    else:
        assert 30 in values(callback) and callback[player_aura - 2].op_str == 'rcx, rsi'
        assert any(x.mnemonic == 'jne' and x.operands[0].imm < x.address for x in callback[pet_aura:])

print(f'PASS linked PE: original live Player/map leave virtual slot {leave_slot // 8}, retained challenge slot {challenge_slot // 8}, single MapScript callback before world removal/deletion and remove-flag split; both raid callbacks remove only departing player/primary-pet auras. Native integration and client traversal remain separate.')
