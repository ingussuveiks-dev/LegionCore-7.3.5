"""Bind five acquisition routes and the earned-owner fallback to linked x64 code.

This verifies selected machine-code paths, not full Character LoadFromDB, group
binding selection, asynchronous teleport arrival, or actual client traversal.
"""
import runpy
import struct
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = globals().get('previous') or runpy.run_path(str(Path(__file__).with_name('verify_warrior_scenario_resume_release.py')))
n, ins = previous['n'], previous['ins']


def sites(body, prefix):
    targets = {n['resolve'](v) for k, v in n['symbols'].items() if k.startswith(prefix)}
    return [i for i, o in enumerate(body) if o.mnemonic in ('call', 'jmp') and o.operands[0].type == CS_OP_IMM and n['resolve'](o.operands[0].imm) in targets]


def find(body, text, start=0, mnemonic=None):
    return next(i for i, o in enumerate(body[start:], start) if o.op_str == text and (mnemonic is None or o.mnemonic == mnemonic))


def rip(o):
    return next(o.address + o.size + a.mem.disp for a in o.operands if a.type == CS_OP_MEM and a.mem.base == X86_REG_RIP)


def ref(body, address):
    return any(o.mnemonic == 'lea' and any(a.type == CS_OP_MEM and a.mem.base == X86_REG_RIP for a in o.operands) and rip(o) == address for o in body)


# Exact leaf boundary; unlike non-leaf methods it has no Windows unwind row.
a = n['symbol']('?Find@AcquisitionScenarioRoute@@')
z = min(v for v in n['symbols'].values() if v > a)
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True
leaf = [o for o in decoder.disasm(n['binary'][n['offset'](a):n['offset'](z)], a) if o.mnemonic != 'int3']
marksman = None
if len(leaf) == 21:
    assert [o.mnemonic for o in leaf] == ['cmp', 'jne', 'lea', 'lea', 'cmp', 'je', 'add', 'cmp', 'jne', 'xor', 'ret', 'cmp', 'jne', 'cmp', 'jne', 'lea', 'ret', 'cmp', 'jne', 'lea', 'ret']
    assert leaf[13].op_str == 'ecx, 0x5d1' and leaf[14].operands[0].imm == leaf[17].address
    assert leaf[17].op_str == 'ecx, 0x5e7' and leaf[18].operands[0].imm == leaf[9].address
    marksman, hunter = rip(leaf[15]), rip(leaf[19])
    assert struct.unpack_from('<4I', n['binary'], n['offset'](marksman)) == (1489, 972, 40419, 201674)
else:
    assert [o.mnemonic for o in leaf] == ['cmp', 'jne', 'lea', 'lea', 'cmp', 'je', 'add', 'cmp', 'jne', 'xor', 'ret', 'cmp', 'jne', 'cmp', 'jne', 'lea', 'ret']
    assert leaf[13].op_str == 'ecx, 0x5e7' and leaf[14].operands[0].imm == leaf[9].address
    hunter = rip(leaf[15])
assert leaf[0].op_str == 'edx, 1' and leaf[1].operands[0].imm == leaf[11].address
assert leaf[4].op_str == 'dword ptr [rax], ecx' and leaf[6].op_str == 'rax, 0x10'
assert leaf[8].operands[0].imm == leaf[4].address and leaf[9].op_str == 'eax, eax'
assert leaf[11].op_str == 'edx, 3' and leaf[12].operands[0].imm == leaf[9].address
assert leaf[5].operands[0].imm == leaf[10].address
assert leaf[7].op_str == 'rax, rdx'
table, end = rip(leaf[2]), rip(leaf[3])
assert end - table == 48
assert struct.unpack_from('<12I', n['binary'], n['offset'](table)) == (1495, 909, 39191, 187803, 1511, 944, 40043, 193335, 1539, 1037, 41105, 203920)
assert struct.unpack_from('<4I', n['binary'], n['offset'](hunter)) == (1511, 973, 40385, 198950)

load = ins('?LoadFromDB@Player@@')
first, seed = sites(load, '?PrepareScenarioLogin@Player@@')
route, = sites(load, '?Find@AcquisitionScenarioRoute@@')
early, warlock, normal = sites(load, '?_LoadQuestStatus@Player@@')
lookup, = [i for i in sites(load, '?GetSpellTargetPosition@SpellMgr@@') if early < i < seed]
validate, = [i for i in sites(load, '?IsValidMapCoord@MapManager@@') if lookup < i < seed]
assert first < route < early < lookup < validate < seed < warlock < normal
unlock = find(load, 'ecx, 8', first - 20, 'bt')
assert unlock < first and load[unlock + 1].mnemonic == 'jb'
assert load[first - 3].op_str == 'r8d, dword ptr [rsp + 0x54]'
login = find(load, 'dword ptr [r12 + 0x6bd8], 0', first, 'cmp')
skip = load[login + 1].operands[0].imm
assert load[login + 1].mnemonic == 'jne'
assert load[route - 2].op_str == 'edx, al' and load[route - 1].op_str == 'ecx, ebx'
assert load[route + 2].op_str == 'rax, rax' and load[route + 3].operands[0].imm == skip
assert find(load, 'r8d, 6', route, 'mov') < early
assert load[early + 1].op_str == 'byte ptr [rsp + 0x50], 1'
status = find(load, 'eax, 3', early, 'cmp')
assert status < lookup and load[status + 1].operands[0].imm == skip
assert find(load, 'edx, dword ptr [r15 + 8]', early) < status
assert find(load, 'edx, dword ptr [r15 + 0xc]', status) < lookup
null = find(load, 'rax, rax', lookup, 'test')
mapping = find(load, 'dword ptr [rax], ebx', null, 'cmp')
assert load[null + 1].operands[0].imm == skip and load[mapping + 1].operands[0].imm == skip
zero = find(load, 'xmm0, xmm6', mapping, 'ucomiss')
zero = find(load, 'xmm0, xmm6', zero + 1, 'ucomiss')
assert load[zero + 2].mnemonic == 'je' and load[zero + 2].operands[0].imm > load[seed].address
assert load[validate + 2].op_str == 'al, al' and load[validate + 3].mnemonic == 'je' and load[validate + 3].operands[0].imm > load[seed].address
for text in ('dword ptr [rsp + 0x260], ebx', 'eax, dword ptr [r15 + 4]', 'dword ptr [rsp + 0x268], r13d', 'qword ptr [rsp + 0x270], r13', 'rax, qword ptr [rsp + 0xb0]', 'xmmword ptr [rax], xmm0', 'xmmword ptr [rax + 0x10], xmm2', 'qword ptr [rax + 0x20], xmm4'):
    assert validate < find(load, text, validate) < seed
assert find(load, 'r13d, r13d', validate, 'xor') < find(load, 'dword ptr [rsp + 0x268], r13d', validate)
assert [o.op_str for o in load[seed - 3:seed]] == ['r8d, r8d', 'edx, ebx', 'rcx, r12']
transport = next(i for i in sites(load, '?Reset@TransportInfo@MovementInfo@@') if seed < i < seed + 24)
selected = find(load, 'dword ptr [rax], 0', seed, 'cmp')
assert seed < selected < transport < warlock and load[selected + 1].mnemonic == 'je'
assert load[selected + 1].operands[0].imm > load[transport].address
assert load[warlock + 1].op_str == 'byte ptr [rsp + 0x50], 1'
once = find(load, 'byte ptr [rsp + 0x50], 0', normal - 20, 'cmp')
assert load[once + 1].mnemonic == 'jne' and load[once + 1].operands[0].imm == load[normal + 1].address
rewarded, = sites(load, '?_LoadQuestStatusRewarded@Player@@')
assert normal < rewarded

# Actual selected-map guard, after map selection and before map-login admission.
create = sites(load, '?CreateMap@MapManager@@')[0]
binding = find(load, 'al, 3', create, 'cmp')
scenario_lookup = next(i for i in sites(load, '?GetScenario@ScenarioMgr@@') if i > binding)
assert create < binding < scenario_lookup < normal
if marksman:
    m1 = find(load, 'ebx, 0x5e7', binding, 'cmp')
    s1 = find(load, 'dword ptr [rax], 0x3cd', m1, 'cmp')
    m2 = find(load, 'ebx, 0x5d1', s1, 'cmp')
    s2 = find(load, 'dword ptr [rax], 0x3cc', m2, 'cmp')
    common = find(load, 'dword ptr [r12 + 0x6bb8], 0', s2, 'cmp')
    skip_binding = load[common + 1].operands[0].imm
    assert binding < m1 < s1 < m2 < s2 < common < scenario_lookup
    assert load[m1 + 1].mnemonic == 'jne' and load[m1 + 1].operands[0].imm == load[m2].address
    assert load[s1 + 1].mnemonic == 'je' and load[s1 + 1].operands[0].imm == load[common].address
    assert load[s1 + 2].mnemonic == 'jmp' and load[s1 + 2].operands[0].imm == skip_binding
    for at in (m2, s2):
        assert load[at + 1].mnemonic == 'jne' and load[at + 1].operands[0].imm == skip_binding
    tests = ('dword ptr [r12 + 0x6bb8], 0', 'qword ptr [r12 + 0x6bc0], 0')
else:
    tests = ('ebx, 0x5e7', 'dword ptr [rax], 0x3cd', 'dword ptr [r12 + 0x6bb8], 0', 'qword ptr [r12 + 0x6bc0], 0')
for text in tests:
    at = find(load, text, binding, 'cmp')
    assert at < scenario_lookup and load[at + 1].mnemonic == 'jne' and load[at + 1].operands[0].imm > load[scenario_lookup].address
clear = find(load, 'word ptr [r12 + 0x6ba8], r13w', scenario_lookup, 'mov')
assert load[scenario_lookup + 2].op_str == 'rax, rax' and load[scenario_lookup + 3].mnemonic == 'je' and load[scenario_lookup + 3].operands[0].imm > load[clear].address
assert scenario_lookup < find(load, 'dword ptr [r12 + 0x6bdc], r13d', scenario_lookup, 'mov') < clear

prepare = ins('?PrepareScenarioLogin@Player@@')
admit = ins('?CanResumeScenarioLogin@Player@@')
for body in (prepare, admit):
    assert ref(body, table) and ref(body, hunter)
    if marksman: assert ref(body, marksman)
bad = find(prepare, 'ecx, dword ptr [rax + 4]', 0, 'cmp')
assert prepare[bad + 1].mnemonic == 'je'
assert bad < find(prepare, 'qword ptr [rsi + 0x6bd8], 0', bad, 'mov') < find(prepare, 'word ptr [rsi + 0x6ba8], di', bad, 'mov')
quest, = sites(admit, '?GetQuestRewardStatus@Player@@')
statuses = sites(admit, '?GetQuestStatus@Player@@')
assert len(statuses) == 2
scenario = find(admit, 'esi, dword ptr [rbx + 4]', 0, 'cmp')
reject = admit[scenario + 1].operands[0].imm
assert scenario < quest < statuses[0] < statuses[1]
assert admit[quest + 2].operands[0].imm == reject
assert admit[statuses[0] + 1].op_str == 'eax, 3' and admit[statuses[1] + 1].op_str == 'eax, 1' and admit[statuses[1] + 2].operands[0].imm == reject
claims = sites(admit, '?GetQuestObjectiveData@Player@@')
assert len(claims) == (2 if marksman else 1)
claim = claims[0]
for text in ('dword ptr [rbx + 4], 0x3cd', 'dword ptr [rdi + 0x6bdc], 0'):
    at = find(admit, text, statuses[-1], 'cmp')
    assert at < claim and admit[at + 1].mnemonic == 'jne' and admit[at + 1].operands[0].imm == admit[claim + 3].address
assert admit[claim - 3].op_str == 'edx, 0x9dc1' and admit[claim - 2].op_str == 'r8d, 0x18710'
assert admit[claim + 1].op_str == 'eax, 1' and admit[claim + 2].mnemonic == 'jge' and admit[claim + 2].operands[0].imm == reject

if marksman:
    bow = claims[1]
    for text in ('dword ptr [rbx + 4], 0x3cc', 'dword ptr [rdi + 0x6bb8], 0', 'qword ptr [rdi + 0x6bc0], 0'):
        at = find(admit, text, claim, 'cmp')
        assert at < bow and admit[at + 1].mnemonic == 'jne' and admit[at + 1].operands[0].imm == admit[bow + 3].address
    assert admit[bow - 3].op_str == 'edx, 0x9de3' and admit[bow - 2].op_str == 'r8d, 0x19442'
    assert admit[bow + 1].op_str == 'eax, 1' and admit[bow + 2].mnemonic == 'jge' and admit[bow + 2].operands[0].imm == reject
    # This is the zero-instance+boot seed only: an existing live/fresh receipt
    # still reaches native resume and owned bow replay with its native return.
    for forbidden in ('?KilledMonsterCredit@Player@@', '?CompleteQuest@Player@@', '?TeleportTo@Player@@', '?CastSpell@Unit@@', '?AddItem@Player@@'):
        assert not sites(admit, forbidden)
    print('PASS linked Marksmanship empty-checkpoint login:1489/972/40419/201674 fifth route and selected-map guard;actual late unrecorded-seed103490 refusal only at zero receipt instance+boot;recorded live/fresh claims retain native admission. No return102548/item/teleport grant. Source68/native64 and full character/group/client/earned-return remain separate.')

exit_body = ins('?ExitCompletedTalonclawLogin@Player@@')
inventory, = sites(exit_body, '?GetItemByEntry@Player@@')
owner, = sites(exit_body, '?GetOwnerGUID@Item@@')
compare, = sites(exit_body, '??9ObjectGuid@@')
count, = sites(exit_body, '?GetCount@Item@@')
spell, = sites(exit_body, '?GetSpellInfo@SpellMgr@@')
target, = sites(exit_body, '?GetSpellTargetPosition@SpellMgr@@')
effect, = sites(exit_body, '?HasEffect@SpellInfo@@')
coord, = sites(exit_body, '?IsValidMapCoord@MapManager@@')
zone, = sites(exit_body, '?GetZoneId@MapManager@@')
teleport, = sites(exit_body, '?TeleportTo@Player@@')
screen, = sites(exit_body, '?RemoveAurasDueToSpell@Unit@@')
assert inventory < owner < compare < count < spell < target < effect < coord < zone < teleport < screen
failure = exit_body[teleport + 2].operands[0].imm
assert exit_body[teleport + 1].op_str == 'al, al' and exit_body[teleport + 2].mnemonic == 'je'
for text, mnemonic in (('dword ptr [rcx + 0xe0], 0x5e7', 'cmp'), ('al, 3', 'cmp'), ('byte ptr [rdi + 0x4ff0], 1', 'test'), ('eax, 0x3cd', 'cmp')):
    at = find(exit_body, text, 0, mnemonic)
    assert at < inventory and exit_body[at + 1].operands[0].imm == failure
reward, = sites(exit_body, '?GetQuestRewardStatus@Player@@')
assert exit_body[reward - 2].op_str == 'edx, 0x9dc1' and exit_body[reward + 2].operands[0].imm == failure
receipts = sites(exit_body, '?GetQuestObjectiveData@Player@@')
assert len(receipts) == 3
for at, receipt, branch in zip(receipts, (100111, 100112, 103798), ('jl', 'jl', 'jge')):
    assert exit_body[at - 3].op_str == 'edx, 0x9dc1' and exit_body[at - 2].op_str == f'r8d, {hex(receipt)}'
    assert exit_body[at + 1].op_str == 'eax, 1' and exit_body[at + 2].mnemonic == branch and exit_body[at + 2].operands[0].imm == failure
assert exit_body[inventory - 3].op_str == 'r8b, 1' and exit_body[inventory - 2].op_str == 'edx, 0x1f728'
for at, branch in ((compare, 'jne'), (count, 'je'), (effect, 'je'), (coord, 'je')):
    assert exit_body[at + 2].mnemonic == branch and exit_body[at + 2].operands[0].imm == failure
for at in (spell, target):
    assert exit_body[at - 2].op_str == 'edx, 0x2e886'
assert exit_body[effect - 2].op_str == 'edx, 0xfc'
assert find(exit_body, 'dword ptr [rbx], 0x4c4', effect, 'cmp') < coord
assert exit_body[zone + 1].op_str == 'eax, 0x1d4e' and exit_body[zone + 2].operands[0].imm == failure
assert exit_body[screen - 2].op_str == 'edx, 0x32315'
for text in ('qword ptr [rdi + 0x6bd8], rsi', 'word ptr [rdi + 0x6ba8], si', 'xmmword ptr [rdi + 0x6bb0], xmm0'):
    assert find(exit_body, text, screen) > screen
# Pending near/far guards branch around the only real transfer/cleanup.
near = find(exit_body, 'byte ptr [rdi + 0x6848], 0', zone, 'cmp')
far = find(exit_body, 'byte ptr [rdi + 0x6849], 0', near, 'cmp')
assert zone < near < far < teleport and exit_body[near + 1].operands[0].imm > exit_body[screen].address and exit_body[far + 1].operands[0].imm > exit_body[screen].address
distance, = sites(exit_body, '?GetExactDist@Position@@')
assert distance > screen and find(exit_body, 'dword ptr [rcx + 0x10], eax', screen, 'cmp') < distance
epsilon = next(o for o in exit_body[distance + 1:] if o.mnemonic == 'movss' and any(a.type == CS_OP_MEM and a.mem.base == X86_REG_RIP for a in o.operands))
assert abs(struct.unpack_from('<f', n['binary'], n['offset'](rip(epsilon)))[0] - 0.01) < 1e-8
for forbidden in ('?AddItem@Player@@', '?KilledMonsterCredit@Player@@', '?UpdateAchievementCriteria@Player@@', '?CompleteQuest@Player@@', '?SetQuestObjectiveData@Player@@', '?DestroyItem@Player@@', '?ResurrectPlayer@Player@@'):
    assert not sites(exit_body, forbidden), forbidden

callers = []
for name in n['symbols']:
    if 'HandlePlayerLogin@WorldSession@@' not in name:
        continue
    try:
        caller = ins(name)
    except (AssertionError, ValueError, IndexError):
        continue
    if sites(caller, '?ExitCompletedTalonclawLogin@Player@@'):
        callers.append(caller)
caller, = callers
sequence = [sites(caller, prefix)[0] for prefix in ('?CheckInstanceLoginValid@Player@@', '?ExitCompletedDarkRidersLogin@Player@@', '?ExitCompletedBrokenShoreLogin@Player@@', '?ExitCompletedSheilunLogin@Player@@', '?ExitCompletedTalonclawLogin@Player@@')]
assert sequence == sorted(sequence)
for at in sequence:
    assert caller[at + 1].op_str == 'al, al' and caller[at + 2].mnemonic == 'jne' and caller[at + 2].operands[0].imm == caller[sequence[-1] + 2].operands[0].imm
assert caller[sequence[-1] + 2].operands[0].imm > caller[sequence[-1] + 3].address
print('PASS linked acquisition login: exact Warrior3+Hunter1511/973/40385/198950 route tables;early native-source/quest/saved-transport ordering;selected live-map refusal;late status/reward and fresh-claim guard;earned owned128808/loading-bank scan plus real flight/claim,190598/effect252/map1220/zone7502,failed/pending transfer guards,205589-only cleanup after accepted native TeleportTo;actual delayed invalid-login caller preserves live/Dark Riders/Shore/Sheilun. No item/credit/resurrection. Source68/native120 and full character/group/client/arrival remain separate.')
