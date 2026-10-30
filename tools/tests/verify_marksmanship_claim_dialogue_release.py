"""Inspect the linked claim callback, native event bridge and bounded retry."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM

previous = runpy.run_path(str(Path(__file__).with_name('verify_marksmanship_return_release.py')))
native = previous['native']
instructions = previous['instructions']
finish_name = '?Finish@State@MarksmanshipPresentation@@'
retry_name = '?Retry@MarksmanshipClaim@@'
definition_name = '?Native@MarksmanshipClaim@@'
finish = instructions(finish_name)
retry = instructions(retry_name)
definition = instructions(definition_name)


def values(entries):
    return {operand.imm for entry in entries for operand in entry.operands if operand.type == CS_OP_IMM}


assert {1489, 40419, 128826, 972, 8, 100397, 52, 22, 500, 60000} <= values(retry), 'Missing scoped owner/stage/timer guards'
assert {100397, 52, 53, 22, 23, 5000, 61, 206, 1654, 7} <= values(definition), 'Native dialogue definition guard absent'
assert native['symbol'](definition_name) in native['calls'](retry_name), 'Custom actor validation is bypassed'
assert native['symbol']('?GetScript@SmartAIMgr@@') in native['calls'](definition_name), 'Native definitions not read'
dispatch = native['symbol']('?ProcessEventsFor@SmartScript@@')
assert dispatch in native['calls'](retry_name), 'Claim bypasses real SmartScript event dispatch'
assert native['symbol']('?AddDelayedEvent@FunctionProcessor@@') in native['calls'](retry_name), 'Missing native delayed retry scheduling'
assert native['symbol'](retry_name) in native['calls'](finish_name), 'Successful presentation does not queue dialogue'
assert 200262 in values(finish), 'Native existing-item equip lost'
cast = native['symbol']('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_NPEAVItem@@PEBVAuraEffect@@VObjectGuid@@')
cast_at = [index for index, entry in enumerate(finish) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == cast]
assert len(cast_at) == 1, 'Expected exactly one native equip attempt'
result = finish[cast_at[0] + 1:cast_at[0] + 5]
assert [entry.mnemonic for entry in result] == ['mov', 'test', 'je', 'call'], 'Reinspect equip-result branch'
success = result[2].operands[0].imm
success_index = next(index for index, entry in enumerate(finish) if entry.address == success)
equipment = finish[success_index:success_index + 16]
assert [entry.mnemonic for entry in equipment] == ['mov', 'mov', 'mov', 'call', 'test', 'je', 'mov', 'mov', 'call', 'cmp', 'jne', 'xor', 'mov', 'mov', 'call', 'jmp'], 'Reinspect actual equipment verification before claim'
assert equipment[0].op_str == 'r8b, 0xf' and equipment[1].op_str == 'dl, 0xff', 'Not checking native mainhand slot15/bag255'
assert native['resolve'](equipment[3].operands[0].imm) == native['symbol']('?GetItemByPos@Player@@QEBAPEAVItem@@EE@Z'), 'Slot lookup replaced by ownership check'
assert equipment[4].op_str == 'rax, rax', 'Missing empty-slot test'
assert equipment[6].op_str == 'edx, 9' and native['resolve'](equipment[8].operands[0].imm) == native['symbol']('?GetUInt32Value@Object@@'), 'Missing native item-entry field read'
assert equipment[9].op_str == 'eax, 0x1f73a', 'Wrong equipped item; expected128826'
assert equipment[5].operands[0].imm == equipment[10].operands[0].imm, 'Empty/wrong-slot outcomes must both skip claim'
assert native['resolve'](equipment[14].operands[0].imm) == native['symbol'](retry_name), 'Verified equipment does not queue bounded retry'
assert equipment[5].operands[0].imm > equipment[15].address, 'Equipment-failure branch can enter success callback'
assert sum(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol'](retry_name) for entry in finish) == 1, 'Unexpected additional claim callback'
# Wardstone's owned retry containers precede the existing text state, moving
# SmartScript fields by0x58. SmartScriptHolder::runOnce remains at0x99.
# Check actual field/value comparisons and the native writer as well as reads.
for displacement, size, expected in [(0x17c, 4, 22), (0x190, 4, 100397), (0x194, 1, 0)]:
    matches = [entry for entry in retry if entry.mnemonic == 'cmp' and len(entry.operands) == 2 and
               entry.operands[0].type == CS_OP_MEM and entry.operands[0].size == size and
               entry.operands[0].mem.disp == displacement and entry.operands[1].type == CS_OP_IMM and
               entry.operands[1].imm == expected]
    assert len(matches) == 1, ('Missing exact inlined pending-text comparison; reinspect layout', displacement, expected)
    assert any(operand.type == CS_OP_MEM and operand.size == size and operand.mem.disp == displacement
               for entry in instructions('?OnUpdate@SmartScript@@') for operand in entry.operands), 'Pending text has no matching native timer consumer'
    assert any(entry.mnemonic == 'mov' and entry.operands[0].type == CS_OP_MEM and
               entry.operands[0].size == size and entry.operands[0].mem.disp == displacement
               for entry in instructions('?ProcessAction@SmartScript@@')), 'Pending text has no matching native TALK producer'
assert any(entry.mnemonic == 'cmp' and entry.operands[0].type == CS_OP_MEM and
           entry.operands[0].size == 1 and entry.operands[0].mem.disp == 0x99
           for entry in retry), 'Missing independent holder runOnce reader'
assert native['symbol']('?GetConditionsForSmartEvent@ConditionMgr@@') in native['calls']('?ProcessEvent@SmartScript@@'), 'Native replay omits condition lookup'
assert b'native Vereesa claim dialogue unavailable after 60000ms' in native['binary'], 'Retry diagnostic missing'
assert b'was accepted but item 128826 is not equipped' in native['binary'], 'Accepted-but-unequipped diagnostic missing'
print('PASS: canonical Release requires accepted200262 AND actual mainhand128826 before claim; verifies native slot/entry reads, empty/wrong-item failure branches, retained diagnostics and scoped60000ms dialogue retry')
