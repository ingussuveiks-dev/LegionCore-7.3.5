"""Verify selected, installed Warswords entrance and late-landing admission."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native = previous['native']
instructions = previous['instructions']
participant = '?IsParticipant@WarswordsArrival@@'
suffix = '@instance_warswords_InstanceMapScript@instance_warswords@@'
entry = '?OnPlayerEnter' + suffix
update = '?Update' + suffix
timer = '?_Do_call@?$_Func_impl_no_alloc@V<lambda_1>@?1??OnPlayerEnter' + suffix


def sites(body, prefix):
    return [index for index, item in enumerate(body) if item.mnemonic in ('call', 'jmp')
            and item.operands[0].type == CS_OP_IMM and native['resolve'](item.operands[0].imm) == native['symbol'](prefix)]


body = instructions(participant)
rejected = body[next(index for index, item in enumerate(body) if item.op_str == 'rcx, rcx' and item.mnemonic == 'test') + 1].operands[0].imm


def rejection(decoded, position, branch, destination):
    assert decoded[position + 1].mnemonic == branch and decoded[position + 1].operands[0].imm == destination, 'Rejected entrance can continue into arrival'


for operands, branch in (('dword ptr [rbx + 0xa18], 0', 'jne'), ('byte ptr [rbx + 0x6848], 0', 'jne'),
                         ('byte ptr [rbx + 0x6849], 0', 'jne'), ('qword ptr [rbx + 8], 0', 'je'),
                         ('dword ptr [rbx + 0xe0], 0x5e7', 'jne'), ('dword ptr [rbx + 0x3b8], edi', 'jne')):
    position = next(index for index, item in enumerate(body) if item.op_str == operands and item.mnemonic == 'cmp')
    rejection(body, position, branch, rejected)
for prefix, branch in (('?IsInWorld@Object@@', 'je'), ('?CanContact@Player@@', 'je'), ('?GetQuestRewardStatus@Player@@', 'jne')):
    position = sites(body, prefix)[0]
    assert body[position + 1].op_str == 'al, al'
    rejection(body, position + 1, branch, rejected)
script = sites(body, '?GetInstanceScript@WorldObject@@')[0]
assert body[script + 1].op_str == 'rax, rsi'
rejection(body, script + 1, 'jne', rejected)
klass = sites(body, '?GetByteValue@Object@@')[0]
assert any(item.op_str == 'edx, 0x35' for item in body[klass - 3:klass]) and any(item.op_str == 'r8b, 1' for item in body[klass - 3:klass])
assert body[klass + 1].op_str == 'al, 1'
rejection(body, klass + 1, 'jne', rejected)
statuses = sites(body, '?GetQuestStatus@Player@@')
assert len(statuses) == 2 and all(any(item.op_str == 'edx, 0x9c6b' for item in body[index - 3:index]) for index in statuses)
assert body[statuses[0] + 1].op_str == 'eax, 3' and body[statuses[0] + 2].mnemonic == 'je'
assert body[statuses[1] + 1].op_str == 'eax, 1'
rejection(body, statuses[1] + 1, 'jne', rejected)
identity = sites(body, '?GetScenarioId@Scenario@@')[0]
assert body[identity + 1].op_str == 'eax, 0x3b0'
rejection(body, identity + 1, 'jne', rejected)
assert any(item.op_str == 'edx, edi' for item in body[sites(body, '?GetScenario@ScenarioMgr@@')[0] - 3:identity])
assert any(item.address >= rejected and item.op_str == 'al, al' and item.mnemonic == 'xor' for item in body)

def verify_landing(decoded, cast):
    admit = sites(decoded, participant)[-1]
    scenario = sites(decoded, '?GetScenario@ScenarioMgr@@')[0]
    stage = sites(decoded, '?GetCurrentStep@Scenario@@')[0]
    assert admit < scenario < stage < cast and not sites(decoded, '?HasItemCount@Player@@'), 'Fresh owned-item replay is blocked or native stage admission is missing'
    assert decoded[stage + 1].op_str == 'eax, eax' and decoded[stage + 1].mnemonic == 'test'
    rejected = decoded[stage + 2].operands[0].imm
    assert rejected > decoded[cast].address
    rejection(decoded, stage + 1, 'jne', rejected)
    rejection(decoded, admit + 1, 'je', rejected)
    assert decoded[scenario + 1].op_str == 'rax, rax'
    rejection(decoded, scenario + 1, 'je', rejected)
    return admit

body = instructions(timer)
admit = sites(body, participant)[0]
first_cast = sites(body, '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@IW4TriggerCastFlags@@')[0]
land = verify_landing(body, first_cast)
assert admit < land < first_cast
assert body[admit + 2].mnemonic == body[land + 2].mnemonic == 'je'
assert body[admit + 2].operands[0].imm > body[first_cast].address and body[land + 2].operands[0].imm > body[first_cast].address
assert all(any(item.op_str == value for item in body[:admit]) for value in ('r8, qword ptr [rcx + 0x10]', 'edx, dword ptr [rcx + 0x18]', 'rcx, qword ptr [rcx + 8]'))
assert any(item.op_str == 'byte ptr [rax + 0x300], 1' for item in body[admit:land])
assert not any(item.op_str == 'byte ptr [rax + 0x300], 0' for item in body)
values = {operand.imm for item in body for operand in item.operands if operand.type == CS_OP_IMM}
assert {205534, 189695, 5000} <= values and native['symbol']('?MoveJump@MotionMaster@@QEAAXMMMMMMI') in native['calls'](timer)

registration = instructions(entry)
assert any(item.op_str == 'edx, 0xbb8' for item in registration)
assert all(any(item.op_str == value for item in registration) for value in ('qword ptr [rsp + 0x48], rbx', 'qword ptr [rsp + 0x50], rdi', 'dword ptr [rsp + 0x58], eax'))
tables = []
for item in registration:
    if item.mnemonic == 'lea' and len(item.operands) == 2 and item.operands[1].type == CS_OP_MEM and item.operands[1].mem.base == X86_REG_RIP:
        address = item.address + item.size + item.operands[1].mem.disp
        table = native['offset'](address)
        if struct.unpack_from('<Q', native['binary'], table + 0x10)[0] == native['symbol'](timer):
            tables.append(address)
assert len(tables) == 1 and native['symbol']('?AddDelayedEvent@FunctionProcessor@@') in native['calls'](entry)

body = instructions(update)
cast = sites(body, '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@MMMIW4TriggerCastFlags@@')[0]
gate = verify_landing(body, cast)
conversation = sites(body, '?CreateConversation@Unit@@')[0]
assert gate < cast < conversation and body[gate + 2].mnemonic == 'je' and body[gate + 2].operands[0].imm > body[conversation].address
assert any(item.op_str == 'dword ptr [rsp + 0x20], 0x323b0' for item in body[gate:cast])
assert any(item.op_str == 'edx, 0x346' for item in body[cast:conversation])

selected = []
for item in instructions('?GetInstanceScript@instance_warswords@@'):
    if item.mnemonic == 'lea' and len(item.operands) == 2 and item.operands[1].type == CS_OP_MEM and item.operands[1].mem.base == X86_REG_RIP:
        table = native['offset'](item.address + item.size + item.operands[1].mem.disp)
        slots = struct.unpack_from('<80Q', native['binary'], table)
        if native['symbol'](entry) in slots and native['symbol'](update) in slots:
            selected.append(table)
assert len(selected) == 1, 'Actual instance factory does not select guarded arrival callbacks'
print('PASS: canonical instance factory and installed3000ms closure retain original Player/script/instance capture, live participation before bootstrap, fresh-step0 landing before native casts/jump without blocking owned-item replay, and independent live admission before205744/838; unrelated entrants cannot clear the instance. No real flight, retry, aura cleanup or client rendering proof.')
