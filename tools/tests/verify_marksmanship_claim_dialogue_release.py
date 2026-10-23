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
success_entries = [entry for entry in finish if success <= entry.address <= success + 20]
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol'](retry_name) for entry in success_entries), 'Success branch does not call bounded retry'
assert sum(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol'](retry_name) for entry in finish) == 1, 'Unexpected additional claim callback'
for displacement, size in [(0x124, 4), (0x138, 4), (0x13c, 1), (0x99, 1)]:
    assert any(operand.type == CS_OP_MEM and operand.size == size and operand.mem.disp == displacement for entry in retry for operand in entry.operands), 'Missing inlined text-timer/runOnce reader; reinspect layout'
assert native['symbol']('?GetConditionsForSmartEvent@ConditionMgr@@') in native['calls']('?ProcessEvent@SmartScript@@'), 'Native replay omits condition lookup'
assert b'native Vereesa claim dialogue unavailable after 60000ms' in native['binary'], 'Retry diagnostic missing'
print('PASS: canonical Release queues claim dialogue only on native equip-cast success, revalidates owner/scenario/custom actor, reads native text-timer/runOnce, dispatches real TEXT_OVER and bounds delayed retries to60000ms')
