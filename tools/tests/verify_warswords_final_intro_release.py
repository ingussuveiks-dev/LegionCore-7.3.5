"""Verify scoped final Vigfus introduction and identity-bound delayed engagement."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_return_credit_release.py')))
native = previous['native']
instructions = previous['instructions']
actor = 'npc_vigfus_bladewind_lastAI@npc_vigfus_bladewind_last@@'
helper = '?IsIntroTarget@' + actor
opening = '?MoveInLineOfSight@' + actor
reset = '?Reset@' + actor
callbacks = [name for name in native['symbols'] if name.startswith('?_Do_call') and 'MoveInLineOfSight@' + actor in name]
assert len(callbacks) == 1, 'Delayed final intro callback is missing or ambiguous'
callback = callbacks[0]
values = {operand.imm for entry in instructions(helper) for operand in entry.operands if operand.type == CS_OP_IMM}
assert {1511, 40043, 944, 5} <= values, 'Final intro lost exact map/quest/scenario/stage'
for dependency in ('?CanContact@Player@@', '?GetQuestStatus@Player@@', '?GetQuestRewardStatus@Player@@', '?GetScenario@ScenarioMgr@@', '?GetScenarioId@Scenario@@', '?GetCurrentStep@Scenario@@'):
    assert native['symbol'](dependency) in native['calls'](helper), f'Missing live intro dependency: {dependency}'
assert native['symbol'](helper) in native['calls'](opening) and native['symbol'](helper) in native['calls'](callback), 'Missing initial or delayed live admission'
assert native['symbol']('?GetPlayer@ObjectAccessor@@') in native['calls'](callback), 'Delayed target is not resolved by captured identity'
assert native['symbol']('?FindNearestPlayer@WorldObject@@') not in native['calls'](callback), 'Delayed intro still adopts a nearby bystander'
assert native['symbol']('?CreateConversation@Unit@@') in native['calls'](opening), 'Native conversation request disappeared'
for prefix, expected in ((opening, {843, 195062, 8000}), (callback, {205424})):
    literals = {operand.imm for entry in instructions(prefix) for operand in entry.operands if operand.type == CS_OP_IMM}
    assert expected <= literals, f'Original intro actions changed: {prefix}'
decoded = instructions(callback)
assert any(entry.mnemonic == 'mov' and entry.op_str == 'eax, dword ptr [rcx + 0xc8]' for entry in decoded), 'Generation capture lost'
assert any(entry.mnemonic == 'cmp' and entry.op_str == 'dword ptr [rbx + 0x10], eax' and decoded[index + 1].mnemonic == 'jne' for index, entry in enumerate(decoded[:-1])), 'Stale generation can engage'
assert any(entry.mnemonic == 'cmp' and entry.op_str == 'byte ptr [rcx + 0xc7], 0' and decoded[index + 1].mnemonic == 'je' for index, entry in enumerate(decoded[:-1])), 'Inactive intro callback is admitted'
assert any(entry.mnemonic == 'inc' and entry.op_str == 'dword ptr [rcx + 0xc8]' for entry in instructions(reset)), 'Reset does not invalidate old callbacks'
tables = []
for entry in instructions('?GetAI@npc_vigfus_bladewind_last@@'):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            offset = native['offset'](entry.address + entry.size + operand.mem.disp)
            slots = struct.unpack_from('<128Q', native['binary'], offset)
            if native['symbol'](opening) in slots and native['symbol'](reset) in slots:
                tables.append(slots.index(native['symbol'](opening)) * 8)
assert len(tables) == 1, 'Final boss factory does not select repaired intro/reset'
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == tables[0] for entry in instructions('?MoveInLineOfSight_Safe@CreatureAI@@')), 'Native LOS dispatcher misses the selected override'
update = '?UpdateAI@' + actor
retry = '?UpdateIntro@' + actor if any(name.startswith('?UpdateIntro@' + actor) for name in native['symbols']) else update
if retry != update:
    assert native['symbol'](retry) in native['calls'](update), 'UpdateAI does not invoke the outlined retry'
retry_body = instructions(retry)
for dependency in ('?GetPlayerListInGrid@WorldObject@@', helper, '?canSeeOrDetect@WorldObject@@'):
    assert native['symbol'](dependency) in native['calls'](retry), f'Stationary retry lost native selection/admission: {dependency}'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'dword ptr [rcx + 0xcc], 0x3e8' for entry in retry_body), 'Retry poll is not bounded to one second'
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == tables[0] for entry in retry_body), 'Retry does not invoke the selected intro override'
update_body = instructions(update)
direct = [(index, native['resolve'](entry.operands[0].imm)) for index, entry in enumerate(update_body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM]
first_retry = retry if retry != update else '?GetPlayerListInGrid@WorldObject@@'
scan = next(index for index, target in direct if target == native['symbol'](first_retry))
victim = next(index for index, target in direct if target == native['symbol']('?UpdateVictim@CreatureAI@@'))
assert scan < victim, 'Intro retry remains behind the no-victim early return'
installed = []
for entry in instructions(opening):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            offset = native['offset'](entry.address + entry.size + operand.mem.disp)
            if struct.unpack_from('<Q', native['binary'], offset + 0x10)[0] == native['symbol'](callback):
                installed.append(offset)
assert len(installed) == 1, 'LOS does not install the verified delayed closure'
print('PASS: canonical final boss selects guarded LOS/reset, actual1511/944/5/40043 scope, captured-GUID lookup/live recheck, generation/pending rejection and 1s stationary retry before victim gate; native843/195062/205424/8s retained. Not full combat, early visibility, voice/visual or client proof.')
