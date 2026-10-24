"""Verify both native Warswords interaction gates and their registered dispatch."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP

phase = runpy.run_path(str(Path(__file__).with_name('verify_warswords_weapon_phase_release.py')))
retreat = phase['retreat']
native = phase['native']
instructions = retreat['instructions']
helper = '?CanTake@WarswordsAcquisition@@'
gossip = '?OnGossipHello@go_art_warswords@@'
loot = '?OnLootStateChanged@go_art_warswords@@'
gossip_dispatch = '?OnGossipHello@ScriptMgr@@QEAA_NPEAVPlayer@@PEAVGameObject@@'
loot_dispatch = '?OnGameObjectLootStateChanged@ScriptMgr@@'
body = instructions(helper)
values = {operand.imm for entry in body for operand in entry.operands if operand.type == CS_OP_IMM}
assert {248832, 1511, 40043, 944, 6} <= values, 'Deployed pickup scope/quest/stage changed'
for dependency in ('?CanContact@Player@@', '?IsWithinDistInMap@WorldObject@@', '?GetInteractionDistance@GameObject@@',
                   '?GetQuestStatus@Player@@', '?GetQuestRewardStatus@Player@@', '?GetScenario@ScenarioMgr@@',
                   '?GetScenarioId@Scenario@@', '?GetCurrentStep@Scenario@@'):
    assert native['symbol'](dependency) in native['calls'](helper), f'Native pickup gate missing: {dependency}'
assert native['symbol'](helper) in native['calls'](gossip), 'Opening bypasses live admission'
assert native['symbol'](helper) in native['calls'](loot), 'Activation bypasses live recheck'
callback = instructions(loot)
calls = [(index, native['resolve'](entry.operands[0].imm)) for index, entry in enumerate(callback)
         if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM]
gate = next(index for index, target in calls if target == native['symbol'](helper))
progression = next(index for index, target in calls if target == native['symbol']('?UpdateAchievementCriteria@Player@@'))
assert gate < progression and any(entry.mnemonic in ('je', 'jz') for entry in callback[gate + 1:progression]), 'Rejected activation reaches progression'
tables = []
for entry in instructions('?AddSC_Warswords@@'):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            address = entry.address + entry.size + operand.mem.disp
            offset = native['offset'](address)
            if (struct.unpack_from('<Q', native['binary'], offset + 0x10)[0] == native['symbol'](gossip)
                    and struct.unpack_from('<Q', native['binary'], offset + 0x50)[0] == native['symbol'](loot)):
                tables.append(address)
assert len(tables) == 1, 'Registration does not select both repaired object callbacks'
for dispatcher, slot in ((gossip_dispatch, 0x10), (loot_dispatch, 0x50)):
    dispatch = instructions(dispatcher)
    direct = any(entry.mnemonic in ('call', 'jmp') and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == slot
                 for entry in dispatch)
    loaded = any(entry.mnemonic == 'mov' and entry.op_str == 'r10, qword ptr [rcx + 0x50]'
                 and dispatch[index + 1].op_str == 'rcx, rax' and dispatch[index + 2].mnemonic == 'call'
                 and dispatch[index + 2].operands[0].type == CS_OP_REG and dispatch[index + 2].op_str == 'r10'
                 for index, entry in enumerate(dispatch[:-2])) if slot == 0x50 else False
    assert direct or loaded, f'Native script dispatch slot changed: {dispatcher}'
assert native['symbol'](gossip_dispatch) in native['calls']('?Use@GameObject@@QEAAXPEAVUnit@@'), 'Native Use skips opening gate'
assert native['symbol']('?Use@GameObject@@QEAAXPEAVUnit@@') in native['calls']('?HandleGameObjectUse@WorldSession@@'), 'Cached-GUID Use path changed'
print('PASS: canonical registered weapon callbacks gate cached-GUID native Use and activation before progression, requiring live1511/944/6 active Warrior40043 and native interaction context. Not full-bag/loot/equip/return or client proof.')
