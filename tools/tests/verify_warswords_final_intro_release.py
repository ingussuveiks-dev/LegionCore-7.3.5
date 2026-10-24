"""Verify scoped final Vigfus introduction and identity-bound delayed engagement."""
from pathlib import Path
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_return_credit_release.py')))
native = previous['native']
instructions = previous['instructions']
actor = 'npc_vigfus_bladewind_lastAI@npc_vigfus_bladewind_last@@'
helper = '?IsIntroTarget@' + actor
opening = '?MoveInLineOfSight@' + actor
reset = '?Reset@' + actor
conversation = '?StartIntroConversation@' + actor
if not any(name.startswith(conversation) for name in native['symbols']):
    conversation = opening
callbacks = [name for name in native['symbols'] if name.startswith('?_Do_call') and 'MoveInLineOfSight@' + actor in name]
assert len(callbacks) == 2, 'Late blessing and engagement callbacks are missing or ambiguous'
outer_wrapper = next(name for name in callbacks if '???R' not in name)
callback = next(name for name in callbacks if '???R' in name)
outer = next(name for name in native['symbols'] if name.startswith('??R') and 'MoveInLineOfSight@' + actor in name)
wrapper_address = native['symbol'](outer_wrapper)
wrapper_decoder = Cs(CS_ARCH_X86, CS_MODE_64)
wrapper_decoder.detail = True
wrapper = list(wrapper_decoder.disasm(native['binary'][native['offset'](wrapper_address):native['offset'](wrapper_address) + 16], wrapper_address))
assert wrapper[0].mnemonic == 'add' and wrapper[0].op_str == 'rcx, 8' and wrapper[1].mnemonic == 'jmp' and wrapper[1].operands[0].imm == native['symbol'](outer), 'Installed late callback does not execute the verified closure'


def reachable(body, start):
    positions = {entry.address: index for index, entry in enumerate(body)}
    pending = [start]
    visited = set()
    while pending:
        address = pending.pop()
        if address in visited or address not in positions:
            continue
        visited.add(address)
        entry = body[positions[address]]
        if entry.mnemonic in ('ret', 'int3', 'ud2'):
            continue
        if entry.mnemonic.startswith('j') and entry.operands[0].type == CS_OP_IMM:
            pending.append(entry.operands[0].imm)
        if entry.mnemonic != 'jmp':
            pending.append(entry.address + entry.size)
    return visited


values = {operand.imm for entry in instructions(helper) for operand in entry.operands if operand.type == CS_OP_IMM}
assert {1511, 40043, 944, 5} <= values, 'Final intro lost exact map/quest/scenario/stage'
for dependency in ('?CanContact@Player@@', '?GetQuestStatus@Player@@', '?GetQuestRewardStatus@Player@@', '?GetScenario@ScenarioMgr@@', '?GetScenarioId@Scenario@@', '?GetCurrentStep@Scenario@@'):
    assert native['symbol'](dependency) in native['calls'](helper), f'Missing live intro dependency: {dependency}'
for consumer in (opening, outer, callback):
    assert native['symbol'](helper) in native['calls'](consumer), 'Missing initial or delayed live admission'
for consumer in (outer, callback):
    assert native['symbol']('?GetPlayer@ObjectAccessor@@') in native['calls'](consumer), 'Delayed target is not resolved by captured identity'
    assert native['symbol']('?GetCreature@ObjectAccessor@@') in native['calls'](consumer), 'Delayed Helya is not resolved by captured identity'
    assert native['symbol']('?FindNearestPlayer@WorldObject@@') not in native['calls'](consumer), 'Delayed intro still adopts a nearby bystander'
assert conversation == opening or native['symbol'](conversation) in native['calls'](opening), 'Intro does not consume acknowledged conversation creation'
assert native['symbol']('?CreateConversation@Conversation@@') in native['calls'](conversation), 'Missing actual native conversation factory'
assert native['symbol']('?Remove@Conversation@@') in native['calls'](conversation), 'Canceled successful introduction is not removed'
assert native['symbol']('?CreateConversation@Unit@@') not in native['calls'](opening), 'Intro still discards the void creation result'
conversation_body = instructions(conversation)
creation = next(index for index, entry in enumerate(conversation_body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol']('?CreateConversation@Conversation@@'))
assert conversation_body[creation - 2].op_str == 'r8d, 0x34b' and conversation_body[creation - 3].op_str == 'r9, r14' and conversation_body[creation - 1].op_str == 'rcx, r15', 'Native843 is not created for the captured Player'
assert conversation_body[creation + 1].op_str == 'al, al' and conversation_body[creation + 2].mnemonic == 'jne', 'Native factory result is discarded'
success_address = conversation_body[creation + 2].operands[0].imm
success = next(index for index, entry in enumerate(conversation_body) if entry.address == success_address)
failed = conversation_body[creation + 3:success]
assert any(entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax]' for entry in failed) and any(entry.op_str == 'edx, 1' for entry in failed), 'Failed construction does not delete the unowned object'
deletion = next(index for index, entry in enumerate(failed) if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax]')
assert [entry.op_str for entry in failed[deletion - 3:deletion]] == ['rax, qword ptr [r15]', 'edx, 1', 'rcx, r15'], 'Failed factory deletes a different allocation'
failure = next(index for index, entry in enumerate(conversation_body) if entry.op_str == 'word ptr [rdi + 0xc6], 0' and entry.mnemonic == 'mov')
rollback = conversation_body[failure - 4:failure + 1]
assert [entry.op_str for entry in rollback[::2]] == ['r12d, dword ptr [rdi + 0xc8]', 'byte ptr [rdi + 0xc7], 0', 'word ptr [rdi + 0xc6], 0'], 'Factory rollback lost captured epoch, pending or both state bytes'
assert rollback[1].mnemonic == 'jne' and rollback[3].mnemonic == 'je' and all(entry.operands[0].imm > conversation_body[failure].address for entry in (rollback[1], rollback[3])), 'Obsolete/inactive failure can clear newer intro state'
assert conversation_body[success].op_str == 'r12d, dword ptr [rdi + 0xc8]' and conversation_body[success + 1].mnemonic == 'jne', 'Successful creation lacks post-construction epoch guard'
timer = next(entry.address for entry in conversation_body if entry.mnemonic == 'mov' and entry.op_str == 'edx, 0x5dc0')
failed_flow = reachable(conversation_body, conversation_body[creation + 3].address)
assert success_address < timer and timer not in failed_flow and success_address not in failed_flow, 'Failed factory still reaches successful creation/timer installation'
remove = next(index for index, entry in enumerate(conversation_body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol']('?Remove@Conversation@@'))
cancel_address = conversation_body[remove - 1].address
post_create = conversation_body[success:success + 16]
assert sum(entry.mnemonic in ('je', 'jne') and entry.operands[0].imm == cancel_address for entry in post_create) == 3, 'Post-create epoch/pending/eligible-player failures do not all remove the obsolete conversation'
for prefix, expected in ((opening, {24000}), (conversation, {843}), (outer, {195062, 7000}), (callback, {205424})):
    literals = {operand.imm for entry in instructions(prefix) for operand in entry.operands if operand.type == CS_OP_IMM}
    assert expected <= literals, f'Original intro actions changed: {prefix}'
decoded = instructions(callback)
assert any(entry.mnemonic == 'mov' and entry.op_str == 'eax, dword ptr [rcx + 0xc8]' for entry in decoded), 'Generation capture lost'
assert any(entry.mnemonic == 'cmp' and entry.op_str == 'dword ptr [rdi + 0x10], eax' and decoded[index + 1].mnemonic == 'jne' for index, entry in enumerate(decoded[:-1])), 'Stale generation can engage'
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
            if struct.unpack_from('<Q', native['binary'], offset + 0x10)[0] == native['symbol'](outer_wrapper):
                installed.append(offset)
assert len(installed) == 1, 'LOS does not install the verified late blessing closure'
installed = []
for entry in instructions(outer):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            offset = native['offset'](entry.address + entry.size + operand.mem.disp)
            if struct.unpack_from('<Q', native['binary'], offset + 0x10)[0] == native['symbol'](callback):
                installed.append(offset)
assert len(installed) == 1, 'Acknowledged blessing does not install the verified engagement closure'
cleanup = '?RemoveIntroConversation@' + actor
cleanup_body = instructions(cleanup)
lookup = next(index for index, entry in enumerate(cleanup_body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol']('?GetConversation@ObjectAccessor@@'))
remove = next(index for index, entry in enumerate(cleanup_body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol']('?Remove@Conversation@@'))
clear = next(index for index, entry in enumerate(cleanup_body) if entry.op_str == 'xmmword ptr [rbx + 0x105], xmm0' and entry.mnemonic == 'movups')
assert clear < lookup < remove and cleanup_body[0].mnemonic == 'push', 'Tracked identity is not cleared before native removal'
assert any(entry.op_str == 'xmm0, xmmword ptr [rcx + 0x105]' for entry in cleanup_body[:clear]) and any(entry.op_str == 'xmmword ptr [rsp + 0x20], xmm0' for entry in cleanup_body[:clear]), 'Cleanup does not capture the original128-bit identity'
assert cleanup_body[lookup - 4].op_str == 'xmm0, xmmword ptr [rsp + 0x20]' and cleanup_body[lookup - 3].op_str == 'rdx, [rsp + 0x30]' and cleanup_body[lookup - 2].op_str == 'rcx, qword ptr [rbx + 0x20]', 'Cleanup lookup lost captured identity or actual creature map context'
assert cleanup_body[lookup + 1].op_str == 'rax, rax' and cleanup_body[lookup + 2].mnemonic == 'je' and cleanup_body[lookup + 2].operands[0].imm > cleanup_body[remove].address, 'Missing conversation can reach Remove'
assert any(entry.op_str == 'al, al' and cleanup_body[index + 1].mnemonic == 'je' and cleanup_body[index + 1].operands[0].imm > cleanup_body[remove].address for index, entry in enumerate(cleanup_body[clear:lookup - 1], start=clear)), 'Empty tracked identity is not rejected'
constructor = native['symbol']('??0ObjectGuid@@QEAA@XZ')
assert constructor in native['calls'](cleanup), 'Clear does not use the native empty GUID constructor'
factory_body = instructions('?GetAI@npc_vigfus_bladewind_last@@')
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == constructor and factory_body[index - 1].op_str == 'rcx, [rsi + 0x105]' for index, entry in enumerate(factory_body[1:], start=1)), 'Selected factory does not initialize the tracked GUID before Reset'
decoded_constructor = list(Cs(CS_ARCH_X86, CS_MODE_64).disasm(native['binary'][native['offset'](constructor):native['offset'](constructor) + 16], constructor))
assert [entry.mnemonic + ' ' + entry.op_str for entry in decoded_constructor[:4]] == ['xor eax, eax', 'mov qword ptr [rcx], rax', 'mov qword ptr [rcx + 8], rax', 'mov rax, rcx'] and decoded_constructor[4].mnemonic == 'ret', 'Native default GUID is not empty'
store = next(index for index, entry in enumerate(conversation_body) if entry.op_str == 'xmmword ptr [rdi + 0x105], xmm0' and entry.mnemonic == 'movups')
assert conversation_body[success].address < conversation_body[store].address < timer and conversation_body[store].address not in failed_flow and conversation_body[store - 3].op_str == 'rcx, r15' and conversation_body[store - 4].op_str == 'edx, edx', 'Successful captured843 GUID is not tracked after post-create guards'
assert conversation_body[store - 2].mnemonic == 'call' and native['resolve'](conversation_body[store - 2].operands[0].imm) == native['symbol']('?GetGuidValue@Object@@'), 'Tracked identity does not come from the successful native conversation'
for consumer in (reset, outer, callback, '?JustDied@' + actor, '?UpdateLifecycle@' + actor):
    assert native['symbol'](cleanup) in native['calls'](consumer), 'Missing selected dialogue cleanup consumer: ' + consumer
cleanup_edge = next(entry for entry in decoded if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol'](cleanup))
admission = next(index for index, entry in enumerate(decoded) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol'](helper))
assert decoded[admission + 2].mnemonic == 'je' and decoded[admission + 2].operands[0].imm < cleanup_edge.address, 'Invalid delayed admission bypasses cancellation'
engagement = next(entry for entry in decoded if entry.mnemonic == 'jmp' and entry.op_str == 'qword ptr [rax + 0x10]')
helya_check = next(index for index, entry in enumerate(decoded) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol']('?IsIntroHelya@' + actor))
helya_branch = next(index for index in range(helya_check + 1, helya_check + 5) if decoded[index].mnemonic.startswith('j'))
assert decoded[helya_branch].mnemonic == 'je' and decoded[helya_branch].operands[0].imm == decoded[admission + 2].operands[0].imm, 'Participant/Helya failures do not share guarded cancellation'
success_flow = reachable(decoded, decoded[helya_branch + 1].address)
assert engagement.address in success_flow and cleanup_edge.address not in success_flow, 'Successful engagement can fall through into cancellation'
print('PASS: selected843 factory acknowledgement and128-bit ownership survive the two installed24000/7000ms closures. Captured player/Helya identities and epoch/pending gates are retained; Reset, both cancellation paths and death/lifecycle remove only the tracked map-local conversation. Client timing/rendering and full traversal remain separate.')
