"""Check the actual existing duration producer and expiry consumer in Release."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG

previous = runpy.run_path(str(Path(__file__).with_name('verify_smart_invoker_player_release.py')))
native = previous['native']
instructions = previous['instructions']
create_name = '?CreateConversation@Conversation@@'
update_name = '?Update@Conversation@@'
create = instructions(create_name)
update = instructions(update_name)
for caller, callee in [
    (create_name, '?GenerateDynamicFieldData@Conversation@@'),
    (create_name, '?SetUInt32Value@Object@@'),
    (update_name, '?GetUInt32Value@Object@@'),
    (update_name, '?Remove@Conversation@@'),
    (update_name, '?IsQuestObjectExpired@WorldObject@@'),
]:
    assert native['symbol'](callee) in native['calls'](caller), f'Missing duration/cleanup bridge {caller} -> {callee}'

padding = [index for index, entry in enumerate(create) if entry.mnemonic == 'add' and any(operand.type == CS_OP_IMM and operand.imm == 5000 for operand in entry.operands)]
assert len(padding) == 1, 'Native5s presentation padding changed'
position = padding[0]
register = create[position].operands[0].reg
assert any(entry.mnemonic == 'mov' and any(operand.type == CS_OP_IMM and operand.imm == 30000 for operand in entry.operands) for entry in create[max(0, position - 4):position]), 'Zero-duration fallback changed'
assert any(entry.mnemonic == 'mov' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x4dc and entry.operands[1].type == CS_OP_REG and entry.operands[1].reg == register for entry in create[position + 1:position + 8]), 'Actual SetDuration store missing after padding'
assert any(entry.mnemonic == 'mov' and any(operand.type == CS_OP_IMM and operand.imm == 12 for operand in entry.operands) for entry in create[position + 1:position + 5]), 'Native last-line field index changed'
assert any(entry.mnemonic == 'cmp' and any(operand.type == CS_OP_MEM and operand.mem.disp == 0x4dc for operand in entry.operands) for entry in update), 'Native Update ignores duration'
assert any(entry.mnemonic == 'sub' for entry in update) and any(entry.mnemonic == 'jbe' for entry in update), 'Elapsed-time/deadline comparison changed'
assert any(entry.mnemonic == 'mov' and any(operand.type == CS_OP_IMM and operand.imm == 12 for operand in entry.operands) for entry in update), 'Expiry reads the wrong conversation deadline field'
print('PASS: canonical Release generates native payload duration, retains30s fallback/5s padding, stores actual last-line/lifetime fields and expires from playback elapsed time with scope cleanup')
