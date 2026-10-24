"""Reject premature Skyhold credit in the deployed Warswords timer callback."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_presentation_release.py')))
native = previous['native']
instructions = previous['instructions']
timer = '?_Do_call@?$_Func_impl_no_alloc@V<lambda_1>@?7??OnLootStateChanged@go_art_warswords@@'
body = instructions(timer)
values = {operand.imm for entry in body for operand in entry.operands if operand.type == CS_OP_IMM}
assert 192085 in values and 103739 not in values, 'Timer still supplies arrival credit'
cast = '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@IW4TriggerCastFlags@@PEAVItem@@PEBVAuraEffect@@VObjectGuid@@'
assert native['symbol'](cast) in native['calls'](timer), 'Existing jump request disappeared'
assert native['symbol']('?KilledMonsterCredit@Player@@') not in native['calls'](timer), 'Arrival credit remains on cast attempt'
installed = []
for entry in instructions('?OnLootStateChanged@go_art_warswords@@'):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            address = entry.address + entry.size + operand.mem.disp
            offset = native['offset'](address)
            if struct.unpack_from('<Q', native['binary'], offset + 0x10)[0] == native['symbol'](timer):
                installed.append(address)
assert len(installed) == 1, 'Acquisition does not install the verified delayed callback'
assert native['symbol']('?RewardPlayerAndGroupAtEvent@Player@@') in native['calls']('?ProcessAction@SmartScript@@'), 'Existing native actor credit dispatcher missing'
assert native['symbol']('?GetTargets@SmartScript@@') in native['calls']('?ProcessAction@SmartScript@@'), 'Existing receiver target selection missing'
assert native['symbol']('?GetWorldObjectsInDist@SmartScript@@') in native['calls']('?GetTargets@SmartScript@@'), 'Native radius selection missing'
print('PASS: canonical installed Warswords timer retains192085 request but has no103739/kill-credit call; native SmartAI radius/player/group receiver consumers remain. Actual arrival/client traversal is not proved.')
