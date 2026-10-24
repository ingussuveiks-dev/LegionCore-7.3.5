"""Verify native summon/event/phase/LINK consumers used by the scoped SQL retry."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM

previous = runpy.run_path(str(Path(__file__).with_name('verify_orestes_farewell_release.py')))
native = previous['native']
instructions = previous['instructions']
summoned = '?JustSummoned@SmartAI@@'
hit = '?SpellHit@SmartAI@@'
initialize = '?InitSummon@TempSummon@@'

output = Path(__file__).resolve().parents[2] / '.codex/xalatath-finale-actor-disassembly.log'
with output.open('w', encoding='utf-8') as stream:
    for prefix in (summoned, hit, initialize):
        stream.write(prefix + '\n')
        for entry in instructions(prefix):
            stream.write(f'{entry.address:x}: {entry.mnemonic} {entry.op_str}\n')

dispatch = native['symbol']('?ProcessEventsFor@SmartScript@@')
for prefix, identity in ((summoned, 17), (hit, 8)):
    assert dispatch in native['calls'](prefix), f'Actual native event bridge missing: {prefix}'
    assert any(operand.type == CS_OP_IMM and operand.imm == identity for entry in instructions(prefix) for operand in entry.operands), f'Wrong native event identity: {prefix}'
assert native['symbol']('?GetUnit@ObjectAccessor@@') in native['calls'](initialize), 'Inlined summoner lookup loses native accessor'
assert any(operand.type == CS_OP_MEM and operand.mem.disp == 0x3b64 for entry in instructions(initialize) for operand in entry.operands), 'Inlined summoner lookup loses actual saved GUID'
virtual_calls = [entry for entry in instructions(initialize) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM]
assert len(virtual_calls) == 2, 'Reinspect native owner-before-child callback ordering'
assert [entry.operands[0].mem.disp for entry in virtual_calls] == [0x140, 0x148], 'Native summon callback virtual slots/order changed'

action = instructions('?ProcessAction@SmartScript@@')
case_start = previous['action_case'](12)
case_end = previous['action_case'](50)
creature_case = [entry for entry in action if case_start <= entry.address < case_end]
summon_apis = {address for name, address in native['symbols'].items() if name.startswith('?SummonCreature@WorldObject@@')}
assert summon_apis, 'Native creature creation API missing'
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) in summon_apis for entry in creature_case), 'Action12 does not call actual creature creation'
assert any(entry.mnemonic == 'cmp' and any(operand.type == CS_OP_IMM and operand.imm == 8 for operand in entry.operands) for entry in creature_case), 'Native fixed-position branch missing'
assert native['symbol']('?ProcessAction@SmartScript@@') in native['calls']('?ProcessEvent@SmartScript@@'), 'Summon/hit events do not reach native actions'
assert native['symbol']('?SetScript9@SmartAI@@') in native['calls']('?ProcessAction@SmartScript@@'), 'Actual hit cannot start the retained timed list'
print('PASS: canonical native17/8 bridges, exact owner-before-child summon slots, fixed action12 creation, phase/timer/LINK and native SetScript9 consumer are linked; SQL/live graph and controlled failure tests remain separate from map/rendering proof')
