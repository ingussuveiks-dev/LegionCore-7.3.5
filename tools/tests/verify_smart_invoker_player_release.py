"""Resolve actual linked switch tables for the opt-in server target130."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM

previous = runpy.run_path(str(Path(__file__).with_name('verify_marksmanship_arrival_release.py')))
native = previous['native']
instructions = previous['previous']['instructions']


def target_case(prefix, target):
    entries = instructions(prefix)
    maximum = next(index for index, entry in enumerate(entries) if entry.mnemonic == 'cmp' and any(operand.type == CS_OP_IMM and operand.imm == 130 for operand in entry.operands))
    tables = [operand.mem for entry in entries[maximum + 1:maximum + 10] for operand in entry.operands if operand.type == CS_OP_MEM and operand.mem.index and operand.mem.disp > 0]
    assert len(tables) == 2 and tables[0].scale == 1 and tables[1].scale == 4, 'Reinspect native compressed target switch'
    index = native['binary'][native['offset'](native['base'] + tables[0].disp + target)]
    destination = struct.unpack_from('<I', native['binary'], native['offset'](native['base'] + tables[1].disp + index * 4))[0]
    return native['base'] + destination


selector = '?GetTargets@SmartScript@@'
address = target_case(selector, 130)
assert address != target_case(selector, 7), 'Opt-in owner selector replaced or aliases raw invoker7'
entries = [entry for entry in instructions(selector) if address <= entry.address < address + 0xb1]
calls = {native['resolve'](entry.operands[0].imm) for entry in entries if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM}
for prefix in ['?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@', '?IsInWorld@Object@@', '?InSamePhase@WorldObject@@', '?IsWithinDistInMap@WorldObject@@']:
    assert native['symbol'](prefix) in calls, f'Missing owner eligibility operation {prefix}'
assert any(entry.mnemonic == 'cmp' and len(entry.operands) == 2 and all(operand.type == CS_OP_MEM for operand in entry.operands) for entry in entries) or any(entry.mnemonic == 'cmp' and any(operand.type == CS_OP_MEM and operand.mem.disp == 8 for operand in entry.operands) for entry in entries), 'Missing source/owner map comparison'
validation = '?IsTargetValid@SmartAIMgr@@'
assert target_case(validation, 130) == target_case(validation, 18) == target_case(validation, 21), 'New selector is not admitted through native positive-distance validation'
assert native['symbol'](selector) in native['calls']('?ProcessAction@SmartScript@@'), 'Real action dispatcher omits selected targets'
assert native['symbol']('?GenerateDynamicFieldData@Conversation@@') in native['calls']('?CreateConversation@Conversation@@'), 'Native conversation payload generation missing'
print('PASS: actual Release switch130 reaches inlined owner/map/phase/world/distance eligibility while raw7 remains distinct; real loader130 shares native nonzero-distance validation18/21 and action/conversation payload bridges remain linked')
