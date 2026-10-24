"""Verify live admission in the installed, original-instance Warswords departure."""
from pathlib import Path
import runpy
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_return_credit_release.py')))
native = previous['native']
instructions = previous['instructions']
body = instructions(previous['timer'])


def call_sites(decoded, prefix):
    return [index for index, entry in enumerate(decoded) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol'](prefix)]


cast = call_sites(body, '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@IW4TriggerCastFlags@@')[0]
exit_address = body[cast + 1].address


def rejects_after(position, branch):
    assert body[position + 1].mnemonic == branch and body[position + 1].operands[0].imm == exit_address, 'Invalid departure can reach the jump'


for condition, branch in (('rbx, rbx', 'je'), ('dword ptr [rbx + 0xa18], 0', 'jne'),
                          ('byte ptr [rbx + 0x6848], 0', 'jne'), ('byte ptr [rbx + 0x6849], 0', 'jne'),
                          ('qword ptr [rbx + 8], 0', 'je'), ('dword ptr [rbx + 0xe0], 0x5e7', 'jne'),
                          ('dword ptr [rbx + 0x3b8], edi', 'jne')):
    position = next(index for index, entry in enumerate(body[:cast]) if entry.op_str == condition and entry.mnemonic in ('test', 'cmp'))
    rejects_after(position, branch)

for prefix, branch in (('?IsInWorld@Object@@', 'je'), ('?CanContact@Player@@', 'je'),
                       ('?GetQuestRewardStatus@Player@@', 'jne'), ('?HasItemCount@Player@@', 'je'),
                       ('?IsCompleted@Scenario@@', 'je')):
    position = call_sites(body, prefix)[0]
    assert position < cast and body[position + 1].mnemonic == 'test' and body[position + 1].op_str == 'al, al', 'Live departure check missing'
    rejects_after(position + 1, branch)

klass = call_sites(body, '?GetByteValue@Object@@')[0]
assert any(entry.op_str == 'edx, 0x35' for entry in body[klass - 3:klass]) and any(entry.op_str == 'r8b, 1' for entry in body[klass - 3:klass]), 'Native class field selection changed'
assert body[klass + 1].op_str == 'al, 1', 'Non-Warrior departure admitted'
rejects_after(klass + 1, 'jne')
reward = call_sites(body, '?GetQuestRewardStatus@Player@@')[0]
statuses = call_sites(body, '?GetQuestStatus@Player@@')
assert len(statuses) == 2 and any(entry.op_str == 'edx, 0x9c6b' for entry in body[reward - 3:reward]), 'Wrong acquisition quest checked'
assert all(any(entry.op_str == 'edx, 0x9c6b' for entry in body[position - 3:position]) for position in statuses), 'Status lookup changed quest'
assert body[statuses[0] + 1].op_str == 'eax, 3' and body[statuses[0] + 2].mnemonic == 'je', 'Incomplete quest rejected'
assert body[statuses[1] + 1].op_str == 'eax, 1', 'Complete active quest rejected'
rejects_after(statuses[1] + 1, 'jne')
owned = call_sites(body, '?HasItemCount@Player@@')[0]
assert all(any(entry.op_str == expected for entry in body[owned - 5:owned]) for expected in ('edx, 0x1f78c', 'r8d, 1', 'r9b, 1')), 'Departure no longer requires real128908 possession'
scenario = call_sites(body, '?GetScenario@ScenarioMgr@@')[0]
assert any(entry.op_str == 'edi, dword ptr [rcx + 0x10]' for entry in body[:scenario]) and body[scenario - 2].op_str == 'edx, edi', 'Departure substitutes a new instance for its capture'
missing = next(index for index, entry in enumerate(body[scenario:cast], scenario) if entry.op_str == 'rax, rax' and entry.mnemonic == 'test')
rejects_after(missing, 'je')
identity = call_sites(body, '?GetScenarioId@Scenario@@')[0]
assert body[identity + 1].op_str == 'eax, 0x3b0', 'Wrong live scenario admitted'
rejects_after(identity + 1, 'jne')
complete = call_sites(body, '?IsCompleted@Scenario@@')[0]
assert body[complete - 2].op_str == 'edx, edx' and body[complete - 2].mnemonic == 'xor', 'Departure uses bonus or unfinished completion'
assert any(entry.op_str == 'rcx, qword ptr [rsi + 8]' for entry in body[complete:cast]) and any(entry.op_str == 'rdx, rcx' for entry in body[complete:cast]), 'Jump no longer self-casts from captured player'
assert any(entry.op_str == 'r8d, 0x2ee55' for entry in body[complete:cast]), 'Native192085 jump changed'

registration = instructions('?OnLootStateChanged@go_art_warswords@@')
capture = next(index for index, entry in enumerate(registration) if entry.op_str == 'eax, dword ptr [rdi + 0x3b8]' and entry.mnemonic == 'mov')
assert registration[capture + 1].op_str == 'dword ptr [rsp + 0x60], eax' and registration[capture - 1].op_str == 'qword ptr [rsp + 0x58], rax', 'Closure does not retain original GO instance and player'
timer = call_sites(registration, '?AddDelayedEvent@FunctionProcessor@@')[0]
assert capture < timer and registration[timer - 1].op_str == 'edx, 0x2ee0' and registration[timer - 2].op_str == 'r8, [rsp + 0x50]', 'Original12s delay or verified closure changed'

decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True
address = native['symbol']('?IsCompleted@Scenario@@')
start = native['offset'](address)
leaf = []
for entry in decoder.disasm(native['binary'][start:start + 160], address):
    if entry.mnemonic == 'int3':
        break
    leaf.append(entry)
assert all(any(entry.op_str == expected for entry in leaf) for expected in ('r11d, byte ptr [rcx + 0x270]', 'dl, dl', 'dl, 1', 'r11b, r8b')), 'Native completion no longer compares step against nonbonus count'
assert sum(entry.mnemonic == 'ret' for entry in leaf) == 2 and any(entry.mnemonic == 'cmovne' for entry in leaf), 'Native bonus exclusion/completion paths changed'
print('PASS: installed12s Warswords closure captures original Player/instance, revalidates world/contact/life/teleport/class/map/quest/ownership and live944 nonbonus completion before self192085, without arrival credit or GO capture. Failed-cast retry, rapid same-instance abandon/reaccept and full button/visual/client traversal remain separate.')
