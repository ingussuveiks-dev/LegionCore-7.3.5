"""Inspect the linked manual pickup, boarded return and logout/resume guards."""
from pathlib import Path
import hashlib
import json
import re
import runpy
import struct

from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
ins = n['instructions']
checked = []


def require(prefix, targets=(), constants=()):
    body = ins(prefix)
    for target in targets:
        assert sites(n, body, target), (prefix, target)
    assert set(constants) <= values(body), (prefix, set(constants) - values(body))
    checked.append(prefix)
    return body


def no_grants(body):
    for target in ('?AddItem@Player@@', '?KilledMonsterCredit@Player@@',
                   '?CompleteQuest@Player@@', '?SetQuestObjectiveData@Player@@'):
        assert not sites(n, body, target), target


getter = require('?GetOriginalTargetGUID@SpellScript@@')
assert getter[0].op_str == 'rax, qword ptr [rcx + 0x20]'
assert getter[1].op_str == 'xmm0, xmmword ptr [rax + 0x260]'
assert getter[3].op_str == 'xmmword ptr [rdx], xmm0', 'Copy the full native original GUID, not a cached Unit pointer'
require('?Check@spell_sheilun_manual_claim@SheilunAcquisition@@',
        ('?GetOriginalTargetGUID@SpellScript@@', '?GetCreature@Map@@',
         '??8ObjectGuid@@', '?Source@Instance@SheilunAcquisition@@'), (102479,))

death = require('?JustDied@npc_aspersiyAI@npc_aspersiy@@', ('?Defeated@Instance@SheilunAcquisition@@',))
opener = require('?OnGossipHello@go_art_sheylun@@', ('?Open@Instance@SheilunAcquisition@@',))
for body in (death, opener):
    no_grants(body)
    assert not sites(n, body, '?CastSpell@Unit@@'), 'No old timed self-claim/teleport shortcut'
open_body = require('?Open@Instance@SheilunAcquisition@@',
                    ('?Source@Instance@SheilunAcquisition@@', '?InventoryArtifact@SheilunAcquisition@@',
                     '?SendLoot@Player@@'), (102479, 256913))
no_grants(open_body)
require('?Stored@Instance@SheilunAcquisition@@',
        ('?GetItemByGuid@Player@@', '?Source@Instance@SheilunAcquisition@@', '?GetPersonalLoot@Player@@'),
        (128937, 256913))
require('?SpawnSource@Instance@SheilunAcquisition@@',
        ('?SummonCreature@WorldObject@@', '?SummonGameObject@WorldObject@@', '?BindQuestObject@Player@@'),
        (102479, 202385, 256913, 41003))

summon = require('?SummonCreature@Map@@', ('?BindQuestObject@Player@@',), (102648, 1541, 1514, 41003, 10))
bindings = sites(n, summon, '?BindQuestObject@Player@@')
early, = [at for at in bindings if any(x.op_str == 'edx, 0xa02b' for x in summon[max(0, at-4):at])]
map_add, = sites(n, summon, '??$AddToMap@VCreature@@@Map@@')
# MSVC places the scoped binding in a later basic block and jumps back into
# the common construction path. Check control flow rather than address order.
addresses = {op.address: index for index, op in enumerate(summon)}
pending_nodes, visited = [early+1], set()
while pending_nodes:
    index = pending_nodes.pop()
    if index in visited or not 0 <= index < len(summon):
        continue
    visited.add(index)
    op = summon[index]
    if op.mnemonic == 'ret' or index == map_add:
        continue
    if op.mnemonic.startswith('j') and op.operands and op.operands[0].type == 2:
        pending_nodes.append(addresses[op.operands[0].imm])
        if op.mnemonic == 'jmp':
            continue
    pending_nodes.append(index+1)
assert map_add in visited, 'Scoped binding must enter the common pre-AddToWorld construction path'
driver = require('?KiteDriver@SheilunAcquisition@@',
                 ('?GetPassenger@Vehicle@@', '?GetVehicleBase@Unit@@', '?CanTargetQuestObject@WorldObject@@'), (101880,))
passenger, = sites(n, driver, '?GetPassenger@Vehicle@@')
assert any(x.op_str in ('edx, 1', 'dl, 1') for x in driver[max(0, passenger-3):passenger]), 'Native int8 driver seat1'
boarding = require('?Hit@spell_sheilun_return_boarding@SheilunAcquisition@@',
                   ('?EarnedItem@Instance@SheilunAcquisition@@', '?KiteDriver@SheilunAcquisition@@',
                    '?GetVehicleBase@Unit@@', '?GetPassenger@Vehicle@@', '?PreventHitDefaultEffect@SpellScript@@'))
assert sites(n, boarding, '?KiteDriver@SheilunAcquisition@@')[0] < sites(n, boarding, '?GetPassenger@Vehicle@@')[0]
require('?EndReturnCast@Instance@SheilunAcquisition@@',
        ('?GetQuestObjectiveData@Player@@QEBAHPEBVQuest@@', '?KiteDriver@SheilunAcquisition@@'), (281599, 101880))
require('?TravelUpdate@Instance@SheilunAcquisition@@',
        ('?SetClientControl@Player@@', '?MoveSmoothFlyPath@MotionMaster@@', '?ExitVehicle@Unit@@',
         '?FlightClear@SheilunAcquisition@@', '?CastSpell@Unit@@'), (202828, 46598, 202831))
require('?UpdateTransit@SheilunAcquisition@@',
        ('?getQuestStatus@Player@@', '?GetQuestStatus@Player@@', '?IsQuestRewarded@Player@@',
         '?TransitItem@SheilunAcquisition@@', '?UpdateAbort@SheilunAcquisition@@',
         '?KiteDriver@SheilunAcquisition@@', '?FlightClear@SheilunAcquisition@@',
         '?MoveSmoothFlyPath@MotionMaster@@', '?ExitVehicle@Unit@@', '?SetClientControl@Player@@'))
abort = require('?UpdateAbort@SheilunAcquisition@@',
                ('?RestoreControl@SheilunAcquisition@@', '?ExitVehicle@Unit@@', '?Landing@SheilunAcquisition@@', '?TeleportTo@Player@@'))
no_grants(abort)

pending = require('?HasPendingSheilunReturn@Player@@',
                  ('?IsQuestRewarded@Player@@', '?GetQuestStatus@Player@@', '?GetItemByEntry@Player@@',
                   '?GetQuestObjectiveData@Player@@QEBAHPEBVQuest@@'), (1541, 10, 41003, 102479, 128937, 281599, 101880))
no_grants(pending)
require('?RecordScenarioEntrance@Player@@',
        ('?GetItemByEntry@Player@@', '?GetQuestObjectiveData@Player@@QEBAHPEBVQuest@@'),
        (1007, 1541, 10, 41003, 128937, 281599))  # Same-TU predicate is inlined.
for prefix in ('?CanPlayerEnter@MapManager@@', '?SaveToDB@Player@@'):
    require(prefix, ('?HasPendingSheilunReturn@Player@@',), (1007,))

logout = require('?LogoutPlayer@WorldSession@@',
                 ('?OnPlayerBeforeLogout@ScriptMgr@@', '?SaveToDB@Player@@', '?OnPlayerLogout@ScriptMgr@@'))
before, = sites(n, logout, '?OnPlayerBeforeLogout@ScriptMgr@@')
save, = sites(n, logout, '?SaveToDB@Player@@')
after, = sites(n, logout, '?OnPlayerLogout@ScriptMgr@@')
assert before < save < after, 'Retain the native final-save order and late cleanup notification'
dispatch = require('?OnPlayerBeforeLogout@ScriptMgr@@')
assert any(x.mnemonic == 'call' and x.op_str == 'qword ptr [rax + 0x160]' for x in dispatch)
hook = require('?OnBeforeLogout@PlayerEvents@SheilunAcquisition@@',
               ('?ReadTransit@SheilunAcquisition@@', '?TransitItem@SheilunAcquisition@@',
                '?CancelTransit@SheilunAcquisition@@', '?Landing@SheilunAcquisition@@'), (202831, 1514))
no_grants(hook)
assert not sites(n, hook, '?TeleportTo@Player@@'), 'Logout must supply the saved destination, not ignored TeleportTo'
assert any(x.op_str == 'dword ptr [rbx + 0x6840], 0x5ea' for x in hook), 'Native saved destination map1514'
assert any(x.op_str == 'byte ptr [rbx + 0x6849], 1' for x in hook), 'Native far-teleport save selector'
for offset in (0x6830, 0x6834, 0x6838, 0x683c):
    assert any(x.mnemonic == 'movss' and x.op_str.startswith(f'dword ptr [rbx + {offset:#x}],') for x in hook)

maptext = (n['args'].release / 'worldserver.map').read_text(errors='replace')
def table(name):
    matches = re.findall(r'\s+\w+:\w+\s+' + re.escape(name) + r'\s+([0-9a-fA-F]{16})', maptext)
    assert len(matches) == 1, name
    return int(matches[0], 16)

events = table('??_7PlayerEvents@SheilunAcquisition@@6B@')
entry, = struct.unpack_from('<Q', n['binary'], n['offset'](events + 44*8))
assert n['resolve'](entry) == n['symbol']('?OnBeforeLogout@PlayerEvents@SheilunAcquisition@@')
factory = require('?GetInstanceScript@?$GenericInstanceMapScript@Uinstance_sheylun@@@@')
derived = table('??_7instance_sheylun@@6B@')
assert any(op.type == CS_OP_MEM and op.mem.base == X86_REG_RIP and
           x.address + x.size + op.mem.disp == derived for x in factory for op in x.operands)
resolved_entries = []
for value in struct.unpack_from('<64Q', n['binary'], n['offset'](derived)):
    try:
        resolved_entries.append(n['resolve'](value))
    except ValueError:
        pass
assert n['symbol']('?Update@Instance@SheilunAcquisition@@') in resolved_entries

print('PASS linked PE: manual native loot/source/original full GUID, pre-create kite scope, actual driver/player boarding guards and native flight/exit paths; completed-outro admission and before-save safe destination; appended PlayerScript slot44 and real instance factory. Native tests/client evidence remain separate.')
print(json.dumps({'exe_sha256': hashlib.sha256(n['binary']).hexdigest(), 'checked_functions': len(checked), 'before_logout_slot': 44}))
