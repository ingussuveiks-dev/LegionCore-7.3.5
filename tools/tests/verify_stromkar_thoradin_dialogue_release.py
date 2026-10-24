"""Verify the selected Thoradin vtable, queued handoff and native text bridges."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_stromkar_thoradin_presence_release.py')))
native = previous['native']
instructions = previous['instructions']
summoned = '?IsSummonedBy@npc_stromkar_thoradin@@'
data = '?SetData@npc_stromkar_thoradin@@'
factory = '?GetAI@?$GenericCreatureScript@Unpc_stromkar_thoradin@@@@'


def transfers(prefix, dependency):
    target = native['symbol'](dependency)
    return [entry for entry in instructions(prefix) if entry.mnemonic in ('call', 'jmp') and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == target]


assert transfers(summoned, '?IsSummonedBy@SmartAI@@'), 'Native JUST_SUMMONED delegation missing'
assert transfers(data, '?SetData@SmartAI@@'), 'Native DATA_SET delegation missing'
scope = {operand.imm for entry in instructions(summoned) for operand in entry.operands if operand.type == CS_OP_IMM}
assert {104307, 1539, 1037, 9} <= scope, 'Scoped initial summon context missing'
for dependency in ('?GetScenario@ScenarioMgr@@', '?GetScenarioId@Scenario@@', '?GetCurrentStep@Scenario@@'):
    assert transfers(summoned, dependency), f'Missing initial context: {dependency}'
body = instructions(data)
for displacement, branch in ((0x202, 'je'), (0x203, 'je'), (0x204, 'jne')):
    position = next(index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == displacement)
    assert body[position + 1].mnemonic == branch, 'Ready/pending/exactly-once conjunction changed'
claimed = next(entry for entry in body if entry.mnemonic == 'mov' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x204 and entry.operands[-1].type == CS_OP_IMM and entry.operands[-1].imm == 1)
assert claimed.address < transfers(data, '?SetData@SmartAI@@')[0].address, 'Exactly-once state must precede reentrant native dispatch'
tables = []
for entry in instructions(factory):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            offset = native['offset'](entry.address + entry.size + operand.mem.disp)
            if struct.unpack_from('<Q', native['binary'], offset + 0x78)[0] == native['symbol'](data):
                tables.append(offset)
assert len(tables) == 1, 'Actual ghost factory does not select the queued handoff vtable'
assert struct.unpack_from('<Q', native['binary'], tables[0] + 0x148)[0] == native['symbol'](summoned), 'Actual summon slot omits initial context'
assert struct.unpack_from('<Q', native['binary'], tables[0] + 0x28)[0] == native['symbol']('?Reset@SmartAI@@'), 'Native reset lifecycle was replaced'
assert transfers(factory, '??0SmartAI@@'), 'Selected ghost is not a SmartAI delegate'
dispatch = '?ProcessEventsFor@SmartScript@@'
for prefix, identity in (('?IsSummonedBy@SmartAI@@', 54), ('?SetData@SmartAI@@', 38), ('?OnUpdate@SmartScript@@', 52)):
    assert transfers(prefix, dispatch), f'Native event bridge missing: {prefix}'
    assert any(operand.type == CS_OP_IMM and operand.imm == identity for entry in instructions(prefix) for operand in entry.operands), f'Wrong native event: {prefix}'
action_case = previous['actors']['previous']['action_case']
action = instructions('?ProcessAction@SmartScript@@')
set_data = [entry for entry in action if action_case(45) <= entry.address < action_case(46)]
slot = next(entry for entry in set_data if entry.mnemonic == 'mov' and entry.op_str == 'r9, qword ptr [rax + 0x78]')
invoke = next(entry for entry in set_data if entry.mnemonic == 'call' and entry.op_str == 'r9')
assert slot.address < invoke.address, 'Actual action45 does not invoke its selected SetData slot'
for argument in ('r8d, dword ptr [r15 + 0x5c]', 'edx, dword ptr [r15 + 0x58]'):
    assert any(slot.address < entry.address < invoke.address and entry.mnemonic == 'mov' and entry.op_str == argument for entry in set_data), 'Action45 no longer supplies native field/data arguments'
assert b'npc_stromkar_thoradin' in native['binary'], 'Ghost registration name missing'
print('PASS: canonical ghost factory selects scoped summon/SetData overrides; ready+pending+not-delivered guards claim once before native dispatch, retain SmartAI reset and actual54/38/52/action45 bridges. Native hosts and SQL verify timing/data separately; no client chat/audio or departure proof')
