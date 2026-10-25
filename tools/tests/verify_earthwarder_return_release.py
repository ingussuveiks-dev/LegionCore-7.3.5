"""Verify installed Protection return admission without attempted-cast arrival credit."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native = previous['native']
instructions = previous['instructions']
matches = [name for name in native['symbols'] if name.startswith('?_Do_call@') and 'go_art_earthwarder@@' in name]
assert len(matches) == 1
timer = matches[0]
body = instructions(timer)


def sites(prefix):
    return [index for index, item in enumerate(body) if item.mnemonic == 'call' and item.operands[0].type == CS_OP_IMM
            and native['resolve'](item.operands[0].imm) == native['symbol'](prefix)]


cast = sites('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@IW4TriggerCastFlags@@')[0]
end = body[cast + 1].address


def reject(position, branch):
    assert body[position + 1].mnemonic == branch and body[position + 1].operands[0].imm == end, 'Rejected Protection return reaches native jump'


for operands, branch in (('rbx, rbx', 'je'), ('dword ptr [rbx + 0xa18], 0', 'jne'),
                         ('byte ptr [rbx + 0x6848], 0', 'jne'), ('byte ptr [rbx + 0x6849], 0', 'jne'),
                         ('qword ptr [rbx + 8], 0', 'je'), ('dword ptr [rbx + 0xe0], 0x5d7', 'jne'),
                         ('dword ptr [rbx + 0x3b8], edi', 'jne')):
    check = next(index for index, item in enumerate(body[:cast]) if item.op_str == operands and item.mnemonic in ('cmp', 'test'))
    reject(check, branch)
for prefix, branch in (('?IsInWorld@Object@@', 'je'), ('?CanContact@Player@@', 'je'),
                       ('?GetQuestRewardStatus@Player@@', 'jne'), ('?HasItemCount@Player@@', 'je'), ('?IsCompleted@Scenario@@', 'je')):
    position = sites(prefix)[0]
    assert body[position + 1].op_str == 'al, al'
    reject(position + 1, branch)
klass = sites('?GetByteValue@Object@@')[0]
assert any(item.op_str == 'edx, 0x35' for item in body[klass - 3:klass]) and any(item.op_str == 'r8b, 1' for item in body[klass - 3:klass])
assert body[klass + 1].op_str == 'al, 1'
reject(klass + 1, 'jne')
statuses = sites('?GetQuestStatus@Player@@')
assert len(statuses) == 2 and all(any(item.op_str == 'edx, 0x9917' for item in body[index - 3:index]) for index in statuses)
assert body[statuses[0] + 1].op_str == 'eax, 3' and body[statuses[0] + 2].mnemonic == 'je'
assert body[statuses[1] + 1].op_str == 'eax, 1'
reject(statuses[1] + 1, 'jne')
owned = sites('?HasItemCount@Player@@')[0]
assert all(any(item.op_str == value for item in body[owned - 5:owned]) for value in ('edx, 0x1f521', 'r8d, 1', 'r9b, 1'))
scenario = sites('?GetScenario@ScenarioMgr@@')[0]
assert any(item.op_str == 'edi, dword ptr [rcx + 0x10]' for item in body[:scenario]) and any(item.op_str == 'edx, edi' for item in body[scenario - 3:scenario])
missing = next(index for index, item in enumerate(body[scenario:cast], scenario) if item.op_str == 'rax, rax' and item.mnemonic == 'test')
reject(missing, 'je')
identity = sites('?GetScenarioId@Scenario@@')[0]
assert body[identity + 1].op_str == 'eax, 0x38d'
reject(identity + 1, 'jne')
complete = sites('?IsCompleted@Scenario@@')[0]
assert body[complete - 2].op_str == 'edx, edx' and body[complete - 2].mnemonic == 'xor'
assert any(item.op_str == 'r8d, 0x2ee55' for item in body[complete:cast])
assert any(item.op_str == 'rcx, qword ptr [rsi + 8]' for item in body[complete:cast]) and any(item.op_str == 'rdx, rcx' for item in body[complete:cast])
assert native['symbol']('?KilledMonsterCredit@Player@@') not in native['calls'](timer)
assert 103739 not in {operand.imm for item in body for operand in item.operands if operand.type == CS_OP_IMM}

callback = instructions('?OnLootStateChanged@go_art_earthwarder@@')
capture = next(index for index, item in enumerate(callback) if item.op_str == 'eax, dword ptr [rdi + 0x3b8]' and item.mnemonic == 'mov')
assert callback[capture - 1].op_str == 'qword ptr [rsp + 0x58], rax' and callback[capture + 1].op_str == 'dword ptr [rsp + 0x60], eax'
assert any(item.op_str == 'edx, 0x2ee0' for item in callback[capture:])
installed = []
for item in callback:
    if item.mnemonic == 'lea' and len(item.operands) == 2 and item.operands[1].type == CS_OP_MEM and item.operands[1].mem.base == X86_REG_RIP:
        address = item.address + item.size + item.operands[1].mem.disp
        table = native['offset'](address)
        if struct.unpack_from('<Q', native['binary'], table + 0x10)[0] == native['symbol'](timer):
            installed.append(address)
assert len(installed) == 1 and native['symbol']('?AddDelayedEvent@FunctionProcessor@@') in native['calls']('?OnLootStateChanged@go_art_earthwarder@@')
print('PASS: canonical Earthwarder installs original Player/instance12000ms closure; live1495/39191/owned128289/completed909 admission dominates ordinary self192085 and every rejection reaches no-cast exit. No attempted-cast103739 credit. Actual loot/equip, manual portal/button and client arrival remain separate verification paths.')
