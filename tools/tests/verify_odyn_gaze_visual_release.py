"""Verify native script-created area visual selection in the deployed PE."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_stromkar_objectives_release.py')))
native = previous['native']
instructions = previous['instructions']
body = instructions('?CreateAreaTrigger@AreaTrigger@@')
lookup = native['symbol']('?GetSpellInfo@SpellMgr@@')
select = native['symbol']('?GetSpellXSpellVisualId@SpellInfo@@')
write = native['symbol']('?SetUInt32Value@Object@@')
def is_call(entry, target):
    return entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == target
position = [index for index, entry in enumerate(body) if is_call(entry, lookup)]
assert len(position) == 1, 'Reinspect script-created visual lookup'
position = position[0]
assert body[position - 2].op_str == 'edx, dword ptr [rdx + 0x6d8]', 'Lookup no longer consumes area VisualID'
assert body[position + 1].op_str == 'rbx, rax' and body[position + 2].mnemonic == 'test'
assert body[position + 3].mnemonic == 'je', 'Raw visual fallback no longer depends on missing spell'
assert body[position + 4].op_str == 'edx, 0x23' and body[position + 5].op_str == 'r8d, dword ptr [rax]'
assert is_call(body[position + 7], write), 'Spell-for-visuals field not written from resolved native spell'
assert body[position + 10].op_str == 'rcx, rbx' and is_call(body[position + 11], select), 'Selected native spell does not supply visual'
assert body[position + 12].op_str == 'r8d, eax' and body[position + 13].op_str == 'edx, 0x24'
assert is_call(body[position + 15], write), 'Actual selected XSpellVisual result not written to public area field'
assert body[position + 3].operands[0].imm > body[position + 15].address, 'Raw fallback no longer skips spell result branch'
for prefix in ('?IsUnitMeetCondition@Unit@@', '?IsPlayerMeetingCondition@ConditionMgr@@'):
    candidates = {native['resolve'](address) for name, address in native['symbols'].items() if name.startswith(prefix)}
    assert candidates & native['calls']('?GetSpellXSpellVisualId@SpellInfo@@'), 'Native visual admission consumer missing'
outbound = instructions('?BuildValuesUpdate@AreaTrigger@@')
assert any(entry.mnemonic == 'cmp' and any(operand.type == CS_OP_IMM and operand.imm == 0x24 for operand in entry.operands) for entry in outbound), 'Outbound update no longer handles actual XSpellVisual field'
print('PASS: canonical no-spell AT branch looks up configured VisualID as a spell, selects its real native visual with admission checks and writes result to public field0x24; outbound update handles that field. SQL55/native model chain are separate checks, not client rendering')
