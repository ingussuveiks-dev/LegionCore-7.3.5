"""Inspect registered sanctuary-door guards and the real native Use/door path."""
from pathlib import Path
import hashlib
import json
import re
import runpy
import struct

from capstone import CS_OP_MEM

# Retain the preceding intro's exact factory/cast/clock/event checks too.
prior = runpy.run_path(str(Path(__file__).with_name('verify_fuzan_temple_intro_release.py')))
n, require = prior['n'], prior['require']
references, installed_functions = prior['references'], prior['installed_functions']
sites, values = prior['sites'], prior['values']
first = len(prior['checked'])
suffix = '@go_fuzan_temple_sanctuary_door@@'
require('?Opened@FuZanTempleDoor@@', ('?GetAchievementMgr@Scenario@@',), (49141,))
require('?World'+suffix, ('?GetScenario@ScenarioMgr@@', '?IsInWorld@Object@@', '?IsPreDelete@Object@@'), (1137,))
require('?Contact'+suffix,
        ('?World'+suffix, '?GetInteractionDistance@GameObject@@', '?GetQuestStatus@Player@@',
         '?GetQuestRewardStatus@Player@@', '?IsWithinDistInMap@WorldObject@@', '?InSamePhase@WorldObject@@', '?CanContact@Player@@'),
        (42765, 10, 1, 3, 17))
require('?Eligible'+suffix, ('?Opened@FuZanTempleDoor@@', '?Contact'+suffix), (1,))
use = require('?GossipUse'+suffix,
              ('?Opened@FuZanTempleDoor@@', '?Contact'+suffix, '?Eligible'+suffix, '?GossipUse@SmartGameObjectAI@@'),
              (251591, 138378, 1625))
hello = require('?GossipHello'+suffix,
                ('?Opened@FuZanTempleDoor@@', '?Eligible'+suffix, '?GossipHello@SmartGameObjectAI@@', '?Reset@SmartGameObjectAI@@'),
                (251591, 138378, 1625))
native_sites = sites(n, hello, '?GossipHello@SmartGameObjectAI@@')
reserved, = [index for index, op in enumerate(hello) if op.mnemonic == 'mov' and op.op_str == 'byte ptr [rbx + 0x1b8], 1']
dispatch = min(index for index in native_sites if index > reserved)
assert len(native_sites) == 2, 'Retain scoped native dispatch and separate other-source delegation'
assert any(op.mnemonic == 'mov' and op.op_str == 'byte ptr [rbx + 0x1b8], 1' for op in hello[:dispatch]), 'Reserve before real SmartAI callback'
assert any(op.mnemonic == 'mov' and op.op_str == 'byte ptr [rbx + 0x1b8], 0' for op in hello[dispatch+1:]), 'Release after native callback'
changed = require('?OnStateChanged'+suffix,
                  ('?Opened@FuZanTempleDoor@@', '?OnStateChanged@SmartGameObjectAI@@', '?ResetDoorOrButton@GameObject@@', '?SetLootState@GameObject@@'),
                  (251591, 138378, 1625, 2, 1))
reset, = sites(n, changed, '?ResetDoorOrButton@GameObject@@')
ready, = sites(n, changed, '?SetLootState@GameObject@@')
assert reset < ready
assert any(op.mnemonic == 'mov' and op.op_str == 'edx, 1' for op in changed[reset+1:ready]), 'Rearm native GO_READY before generic deactivation/respawn'
assert any(op.mnemonic == 'mov' and op.op_str == 'byte ptr [rbx + 0x1b9], 1' for op in changed[:reset]), 'Own restoration/reentry guard'
factory_prefix = '?GetAI@?$GenericGameObjectScript@Ugo_fuzan_temple_sanctuary_door@@'
factory = require(factory_prefix)
registration = n['instructions']('?AddSC_instance_temple_of_the_jade_serpent@@')
assert n['symbol'](factory_prefix) in installed_functions(registration), 'Actual registered generic GO factory'
maptext = (n['args'].release/'worldserver.map').read_text(errors='replace')
tables = re.findall(r'\s+\w+:\w+\s+'+re.escape('??_7go_fuzan_temple_sanctuary_door@@6B@')+r'\s+([0-9a-fA-F]{16})', maptext)
assert len(tables) == 1
vtable = int(tables[0], 16)
assert vtable in references(factory), 'Factory installs the actual derived AI vtable'
slots = struct.unpack_from('<24Q', n['binary'], n['offset'](vtable))
resolved = [n['resolve'](at) if n['base'] <= at < n['base']+0x10000000 else at for at in slots]
for method in ('GossipUse', 'GossipHello', 'OnStateChanged'):
    assert n['symbol']('?'+method+suffix) in resolved, method
use_slot, = [index for index, address in enumerate(resolved) if address == n['symbol']('?GossipUse'+suffix)]
native_use = n['instructions']('?Use@GameObject@@')
assert any(op.mnemonic in ('call', 'jmp') and op.operands[0].type == CS_OP_MEM and op.operands[0].mem.disp == use_slot*8 for op in native_use), 'Actual GameObject::Use virtual gate'
# MSVC inlines UseDoorOrButton/SwitchDoorOrButton here. Inspect its actual
# physical block: set GO state, activate loot state2, then notify THIS GO AI
# through its real OnStateChanged vtable slot20 (not a Unit's same-offset call).
state_slot, = [index for index, address in enumerate(resolved) if address == n['symbol']('?OnStateChanged'+suffix)]
notifications = [index for index, op in enumerate(native_use)
                 if op.mnemonic == 'call' and op.operands[0].type == CS_OP_MEM and op.operands[0].mem.disp == state_slot*8
                 and any(x.mnemonic == 'mov' and x.op_str == 'edx, 2' for x in native_use[max(0,index-8):index])
                 and any(x.mnemonic == 'mov' and x.op_str == 'rcx, qword ptr [rsi + 0x758]' for x in native_use[max(0,index-8):index])]
physical, = notifications
activated, = [index for index in sites(n, native_use, '?SetLootState@GameObject@@') if 0 < physical-index < 20]
assert any(x.mnemonic == 'mov' and x.op_str == 'edx, 2' for x in native_use[activated-5:activated])
assert any(0 < activated-index < 10 for index in sites(n, native_use, '?SetGoState@GameObject@@')), 'Retained native physical door state before activation'
for body in (use, hello, changed):
    for target in ('?UpdateAchievementCriteria@Player@@', '?UpdateAchievementCriteria@Scenario@@', '?AddItem@Player@@',
                   '?KilledMonsterCredit@Player@@', '?CompleteQuest@Player@@', '?SetQuestObjectiveData@Player@@', '?TeleportTo@Player@@'):
        assert not sites(n, body, target), target
assert b'native sanctuary door event52452 did not acknowledge49141' in n['binary']
print('PASS linked sanctuary door: actual factory/vtable/native Use gate, exact source/player/stage, native205/49141 receipt/reopen, failed credit retry and rejected non-player reset+GO_READY; original physical door retained, no direct grants. Client display remains separate.')
print(json.dumps({'exe_sha256': hashlib.sha256(n['binary']).hexdigest(), 'door_functions': len(prior['checked'])-first, 'gossip_use_slot': use_slot}))
