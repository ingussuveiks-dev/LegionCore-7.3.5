"""Verify the installed guarded Truthguard travel click and native empty-row dispatch."""
from pathlib import Path
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_truthguard_credit_cast_release.py')))
native = previous['native']
instructions = previous['instructions']
suffix = '@npc_sr_argent_hippogryphAI@npc_sr_argent_hippogryph@@'
helper = '?SummonForPlayer' + suffix
callback = '?OnSpellClick' + suffix
body = instructions(helper)
cast_target = native['symbol']('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N')
casts = [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
         and native['resolve'](entry.operands[0].imm) == cast_target]
assert len(casts) == 1
cast = casts[0]
assert body[cast - 1].op_str == 'rcx, r13' and body[cast - 2].op_str == 'rdx, r13'
assert body[cast - 4].op_str == 'r9b, 1', 'Not a native triggered player-self cast'
assert body[cast + 1].op_str == 'esi, eax' and body[cast + 2].op_str == 'eax, eax'
assert body[cast + 3].mnemonic == 'je', 'Native zero/OK result does not select success'
accepted = body[cast + 3].operands[0].imm
assert not any(entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax + 0x38]' for entry in body), 'Cast admission still hides source'
vehicle = next(index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
               and native['resolve'](entry.operands[0].imm) == native['symbol']('?GetVehicleBase@Unit@@'))
assert vehicle < cast and body[vehicle + 1].op_str == 'rax, rax' and body[vehicle + 2].mnemonic == 'jne'
assert body[vehicle + 2].operands[0].imm == accepted
assert any(entry.op_str == 'r12, [rbx + 0x40]' for entry in body[:cast])
assert any(entry.op_str == 'dword ptr [rax + 0x2c], 0x1388' for entry in body[:cast]), 'Request deadline not published before reentrant native cast'
assert any(entry.op_str == 'xmmword ptr [rax + 0x1c], xmm0' and entry.mnemonic == 'movups' for entry in body[:cast]), 'Request key truncated instead of copying full native GUID'
assert any(entry.mnemonic == 'je' and entry.operands[0].imm == accepted for entry in body[vehicle + 3:cast]), 'Duplicate GUID request does not skip cast'


def reachable(start):
    by_address = {entry.address: entry for entry in body}
    pending = [start]
    seen = set()
    while pending:
        address = pending.pop()
        if address in seen:
            continue
        assert address in by_address, 'Helper branches outside checked runtime bounds'
        seen.add(address)
        entry = by_address[address]
        if entry.mnemonic in ('ret', 'int3'):
            continue
        if entry.mnemonic.startswith('j'):
            assert entry.operands[0].type == CS_OP_IMM
            pending.append(entry.operands[0].imm)
            if entry.mnemonic == 'jmp':
                continue
        pending.append(entry.address + entry.size)
    return seen


assert body[cast].address not in reachable(accepted)
assert b'Truthguard travel: summon cast %u rejected with result %u' in native['binary']
click = instructions(callback)
helper_calls = [index for index, entry in enumerate(click) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
                and native['resolve'](entry.operands[0].imm) == native['symbol'](helper)]
assert len(helper_calls) == 2
for index, spell in zip(helper_calls, (209884, 210358)):
    assert click[index - 3].op_str == f'r8d, {hex(spell)}'
    assert click[index - 2].op_str == 'rdx, rbx' and click[index - 1].op_str == 'rcx, rdi'
for operand in ('byte ptr [rdx + 0x16], 4', 'eax, 0x19d9b', 'edx, 0xa421', 'esi, 3', 'eax, 0x19edb', 'edx, 6', 'eax, 7'):
    assert any(entry.op_str == operand for entry in click), f'Original live player/entry/quest/event gate missing: {operand}'
assert cast_target not in native['calls'](callback), 'A direct duplicate summon cast remains outside the helper'
def click_find(operand, mnemonic=None):
    return next(index for index, entry in enumerate(click) if entry.op_str == operand and (not mnemonic or entry.mnemonic == mnemonic))

def click_call(prefix):
    target = native['symbol'](prefix)
    return next(index for index, entry in enumerate(click) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
                and native['resolve'](entry.operands[0].imm) == target)

quest = click_call('?GetQuestStatus@Player@@')
player_class = click_call('?GetByteValue@Object@@')
rewarded = click_call('?GetQuestRewardStatus@Player@@')
same_map = click_call('?IsInMap@WorldObject@@')
same_phase = click_call('?InSamePhase@WorldObject@@')
assert quest < player_class < rewarded < same_map < same_phase < helper_calls[0]
assert any(entry.op_str == 'edx, 0x35' for entry in click[quest:player_class]) and any(entry.op_str == 'r8b, 1' for entry in click[quest:player_class])
rejected = click[player_class + 2].operands[0].imm
assert click[player_class + 1].op_str == 'al, 2' and click[player_class + 2].mnemonic == 'jne'
life = click_find('dword ptr [rbx + 0xa18], 0', 'cmp')
assert click[life + 2].mnemonic == 'jne' and click[life + 2].operands[0].imm == rejected
for gate in (rewarded + 1, click_find('eax, 0xfffffffd', 'test')):
    assert click[gate + 1].mnemonic == 'jne' and click[gate + 1].operands[0].imm == rejected
assert click_find('eax, [rsi - 1]', 'lea') < same_map
for gate in (same_map, same_phase):
    assert click[gate + 2].mnemonic == 'je' and click[gate + 2].operands[0].imm == rejected
    assert [entry.op_str for entry in click[gate - 2:gate]] == ['rcx, qword ptr [rdi + 0x20]', 'rdx, rbx']
contact = click_call('?CanContact@Player@@')
npc = click_call('?GetNPCIfCanInteractWith@Player@@')
visible = click_call('?canSeeOrDetect@WorldObject@@')
los = click_call('?IsWithinLOSInMap@WorldObject@@')
assert same_phase < contact < npc < visible < los < helper_calls[0]
assert click[contact - 1].op_str == 'rcx, rbx' and click[contact + 1].op_str == 'al, al'
assert click[contact + 2].mnemonic == 'je' and click[contact + 2].operands[0].imm == rejected
for operand in ('byte ptr [rbx + 0x6848], 0', 'byte ptr [rbx + 0x6849], 0'):
    gate = click_find(operand, 'cmp')
    assert contact < gate < npc and click[gate + 1].mnemonic == 'jne' and click[gate + 1].operands[0].imm == rejected
assert [entry.op_str for entry in click[npc - 7:npc]] == ['rbp, qword ptr [rdi + 0x20]', 'rdx, [rsp + 0x20]', 'r9d, r9d', 'r8d, 0x1000000', 'rcx, rbx', 'xmm0, xmmword ptr [rax]', 'xmmword ptr [rsp + 0x20], xmm0']
assert click[npc + 1].op_str == 'rax, rbp' and click[npc + 2].mnemonic == 'jne'
late_rejected = click[npc + 2].operands[0].imm
assert [entry.op_str for entry in click[visible - 4:visible]] == ['r9d, r9d', 'r8d, r8d', 'rdx, rbp', 'rcx, rbx']
assert [entry.op_str for entry in click[los - 3:los]] == ['rdx, qword ptr [rdi + 0x20]', 'r8d, r8d', 'rcx, rbx']
for gate in (visible, los):
    assert click[gate + 1].op_str == 'al, al' and click[gate + 2].mnemonic == 'je' and click[gate + 2].operands[0].imm == late_rejected
assert same_phase < click_find('dword ptr [rax + 0xe0], 0x4c4', 'cmp') < helper_calls[0]
registry = click_call('?GetScenario@ScenarioMgr@@')
identity = click_call('?GetScenarioId@Scenario@@')
assert helper_calls[0] < click_find('dword ptr [rax + 0xe0], 0x5d7', 'cmp') < click_call('?instance@ScenarioMgr@@') < registry < identity < helper_calls[1]
assert [entry.op_str for entry in click[registry - 3:registry]] == ['rdx, qword ptr [rdi + 0x20]', 'rcx, rax', 'edx, dword ptr [rdx + 0x3b8]'], 'Registry key is not the actual clicked source instance'
assert click[registry + 1].op_str == 'rax, rax' and click[registry + 2].mnemonic == 'je' and click[registry + 2].operands[0].imm == late_rejected
assert click[identity + 1].op_str == 'eax, 0x43a' and click[identity + 2].mnemonic == 'jne' and click[identity + 2].operands[0].imm == late_rejected
owned = click_call('?HasItemCount@Player@@')
assert identity < owned < click_call('?GetInstanceScript@WorldObject@@')
assert [entry.op_str for entry in click[owned - 4:owned]] == ['r9b, 1', 'edx, 0x1f762', 'r8d, 1', 'rcx, rbx']
assert click[owned + 1].op_str == 'al, al' and click[owned + 2].mnemonic == 'je' and click[owned + 2].operands[0].imm == late_rejected
assert click[click_find('eax, 7', 'cmp') + 1].operands[0].imm == late_rejected
for target in (rejected, late_rejected):
    assert all(entry.mnemonic in ('mov', 'add', 'pop', 'ret') for entry in click if entry.address >= target), 'Admission rejection reaches a summon request'
interaction = instructions('?GetNPCIfCanInteractWith@Player@@')
interaction_calls = native['calls']('?GetNPCIfCanInteractWith@Player@@')
for prefix in ('?GetCreatureOrPetOrVehicle@ObjectAccessor@@', '?IsInWorld@Object@@', '?isInFlight@Unit@@', '?GetGuidValue@Object@@', '?IsHostileTo@Unit@@', '?IsWithinDistInMap@WorldObject@@', '?CanHaveReputation@FactionEntry@@', '?GetRank@ReputationMgr@@'):
    assert native['symbol'](prefix) in interaction_calls, f'Native NPC admission lost {prefix}'
distance = next(index for index, entry in enumerate(interaction) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
                and native['resolve'](entry.operands[0].imm) == native['symbol']('?IsWithinDistInMap@WorldObject@@'))
constant = interaction[distance - 5]
assert constant.mnemonic == 'movss' and constant.operands[1].type == CS_OP_MEM and constant.operands[1].mem.base == X86_REG_RIP
assert struct.unpack_from('<f', native['binary'], native['offset'](constant.address + constant.size + constant.operands[1].mem.disp))[0] == 5.0
assert [entry.op_str for entry in interaction[distance - 4:distance]] == ['r9b, 1', 'rdx, rdi', 'byte ptr [rsp + 0x20], 0', 'rcx, rbx']
assert interaction[distance + 3].mnemonic == 'cmove' and interaction[distance + 3].op_str == 'rbx, rcx'
for operand in ('byte ptr [rcx + 0x1c1], 2', 'byte ptr [rax + 0x1c1], 0x80', 'edx, 0x7d', 'edx, 0x7e', 'edx, 0x18', 'r8d, 0x4000', 'eax, 2'):
    assert any(entry.op_str == operand for entry in interaction), f'Native flags/death/reputation gate missing: {operand}'
assert native['symbol']('?GetCurrentStep@Scenario@@') not in native['calls'](callback), 'Completion step6 is incorrectly fenced at pickup step5'
assert not any(operand.type == CS_OP_MEM and operand.mem.disp == 0x6ba8 for entry in click for operand in entry.operands), 'Consumed Player selector substituted for actual scenario'
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
lookup_address = native['symbol']('?GetScenario@ScenarioMgr@@')
lookup = []
for entry in decoder.disasm(native['binary'][native['offset'](lookup_address):native['offset'](lookup_address) + 160], lookup_address):
    if entry.mnemonic == 'int3':
        break
    lookup.append(entry)
assert lookup[0].op_str == 'qword ptr [rcx + 8], 0' and lookup[-2].op_str == 'eax, eax' and lookup[-2].mnemonic == 'xor' and lookup[-1].mnemonic == 'ret'
assert any(entry.op_str == 'dword ptr [rax + 0x20], edx' for entry in lookup) and any(entry.op_str == 'edx, dword ptr [rcx + 0x20]' for entry in lookup)
assert any(entry.op_str == 'rax, qword ptr [rcx + 0x28]' for entry in lookup), 'Native registry lookup no longer returns mapped Scenario or null'

tables = []
for entry in instructions('?GetAI@npc_sr_argent_hippogryph@@'):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            table = native['offset'](entry.address + entry.size + operand.mem.disp)
            slots = struct.unpack_from('<80Q', native['binary'], table)
            if native['symbol'](callback) in slots:
                tables.append(slots)
assert len(tables) == 1 and tables[0][0x218 // 8] == native['symbol'](callback), 'Factory does not select the repaired OnSpellClick'
poll_name = '?UpdateBoardingRequests' + suffix
poll = instructions(poll_name)
def find(operand, mnemonic=None):
    return next(index for index, entry in enumerate(poll) if entry.op_str == operand and (not mnemonic or entry.mnemonic == mnemonic))

def calls(prefix):
    target = native['symbol'](prefix)
    return [index for index, entry in enumerate(poll) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == target]

lookup = calls('?GetPlayer@ObjectAccessor@@')[0]
quest = calls('?GetQuestStatus@Player@@')[0]
rewarded = calls('?GetQuestRewardStatus@Player@@')[0]
ride = calls('?GetVehicleBase@Unit@@')[0]
summoner = calls('?GetSummoner@TempSummon@@')[0]
assert lookup < quest < rewarded < ride < summoner
assert poll[lookup - 1].op_str == 'rcx, qword ptr [r14 + 0x20]' and poll[lookup - 2].op_str == 'rdx, [rsp + 0x60]'
assert find('dword ptr [rsi + 0xa18], 0', 'cmp') < ride and find('eax, 0xfffffffd', 'test') < ride
assert poll[lookup + 2].op_str == 'rax, rax' and poll[lookup + 3].mnemonic == 'je'
revoked = poll[lookup + 3].operands[0].imm
for gate, condition in ((find('dword ptr [rsi + 0xa18], 0', 'cmp'), 'jne'), (rewarded + 1, 'jne'), (find('eax, 0xfffffffd', 'test'), 'jne')):
    assert poll[gate + 1].mnemonic == condition and poll[gate + 1].operands[0].imm == revoked
maps, phases = calls('?IsInMap@WorldObject@@'), calls('?InSamePhase@WorldObject@@')
assert len(maps) == len(phases) == 2 and maps[0] < phases[0] < ride < maps[1] < phases[1]
for gate in (maps[0], phases[0]):
    assert poll[gate + 2].mnemonic == 'je' and poll[gate + 2].operands[0].imm == revoked
waiting = poll[ride + 2].operands[0].imm
assert poll[ride + 1].op_str == 'rax, rax' and poll[ride + 2].mnemonic == 'je'
assert find('byte ptr [rax + 0x30e8], 1', 'test') < summoner
assert poll[summoner + 1].op_str == 'rax, rsi' and poll[summoner + 2].mnemonic == 'jne' and poll[summoner + 2].operands[0].imm == waiting
for gate in (maps[1], phases[1]):
    assert poll[gate + 2].mnemonic == 'je' and poll[gate + 2].operands[0].imm == waiting
for operand in ('eax, 0x19d9b', 'eax, 0x19d9e', 'eax, 0x19edb', 'eax, 0x19edd'):
    assert find(operand, 'cmp') > summoner
hide = next(index for index, entry in enumerate(poll) if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax + 0x38]')
assert [entry.op_str for entry in poll[hide - 3:hide]] == ['rcx, qword ptr [r14 + 0x20]', 'rax, qword ptr [rcx]', 'rdx, rsi']
assert poll[hide - 5].mnemonic == 'call' and poll[hide - 4].op_str == 'rbx, rax', 'Acknowledged request not erased before hide'
outgoing, outgoing_bird = find('eax, 0x19d9b', 'cmp'), find('eax, 0x19d9e', 'cmp')
returning, returning_bird = find('eax, 0x19edb', 'cmp'), find('eax, 0x19edd', 'cmp')
assert outgoing < outgoing_bird < returning < returning_bird < hide
assert poll[outgoing + 1].mnemonic == 'jne' and poll[outgoing + 1].operands[0].imm == poll[outgoing_bird + 2].address
assert poll[outgoing_bird + 1].mnemonic == 'je' and poll[outgoing_bird + 1].operands[0].imm == poll[hide - 7].address
for gate in (returning, returning_bird):
    assert poll[gate + 1].mnemonic == 'jne' and poll[gate + 1].operands[0].imm == waiting
assert poll[returning_bird + 2].address == poll[hide - 7].address
loop = poll[hide + 1].operands[0].imm
assert poll[hide + 1].mnemonic == 'jmp'
timeout = next(index for index, entry in enumerate(poll) if entry.address == waiting)
assert [entry.op_str for entry in poll[timeout:timeout + 2]] == ['eax, dword ptr [rbx + 0x2c]', 'eax, r12d']
assert poll[timeout + 2].mnemonic == 'ja'
subtract = find('eax, r12d', 'sub')
assert poll[timeout + 2].operands[0].imm == poll[subtract].address and poll[subtract + 1].op_str == 'dword ptr [rbx + 0x2c], eax'
by_address = {entry.address: entry for entry in poll}
for start in (waiting, revoked):
    pending, seen = [start], set()
    while pending:
        address = pending.pop()
        if address == loop or address in seen:
            continue
        assert address in by_address
        seen.add(address)
        entry = by_address[address]
        if entry.mnemonic in ('ret', 'int3'):
            continue
        if entry.mnemonic.startswith('j'):
            assert entry.operands[0].type == CS_OP_IMM
            pending.append(entry.operands[0].imm)
            if entry.mnemonic == 'jmp':
                continue
        pending.append(entry.address + entry.size)
    assert poll[hide].address not in seen, 'Unacknowledged/revoked request hides source in its iteration'
assert cast_target not in native['calls'](poll_name), 'Boarding polling invents automatic casts'
assert b'Truthguard travel: boarding not observed for player %u at source %u' in native['binary']
update = instructions('?UpdateAI' + suffix)
assert tables[0][0x18 // 8] == native['symbol']('?UpdateAI' + suffix)
assert native['symbol'](poll_name) in native['calls']('?UpdateAI' + suffix), 'Selected update does not poll native boarding'
dispatch = instructions('?HandleSpellClick@Unit@@')
ai_call = next(entry for entry in dispatch if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax + 0x218]')
empty = next(index for index, entry in enumerate(dispatch) if entry.op_str == 'rbx, qword ptr [rsp + 0xc8]' and entry.mnemonic == 'cmp')
assert dispatch[empty + 1].mnemonic == 'je'
assert dispatch[empty + 1].operands[0].imm < ai_call.address
empty_target = next(index for index, entry in enumerate(dispatch) if entry.address == dispatch[empty + 1].operands[0].imm)
segment = [entry for entry in dispatch[empty_target:] if entry.address < ai_call.address]
assert not any(entry.mnemonic == 'ret' or entry.mnemonic == 'jmp' for entry in segment), 'Empty rows bypass the AI'
assert any(entry.op_str == 'sil, sil' and entry.mnemonic == 'test' and entry.address > ai_call.address for entry in dispatch), 'Native result is consulted before AI'
visibility = instructions('?canSeeSpellClickOn@Player@@')
empty = next(index for index, entry in enumerate(visibility) if entry.op_str == 'rbx, qword ptr [rsp + 0x38]' and entry.mnemonic == 'cmp')
assert visibility[empty + 1].mnemonic == 'je'
target = next(entry for entry in visibility if entry.address == visibility[empty + 1].operands[0].imm)
assert target.op_str == 'al, 1', 'Explicit static spellclick flag is hidden without DB rows'
assert any(entry.op_str == 'r8d, 0x1000000' for entry in visibility[:empty]), 'Native visibility no longer checks the actual flag'
assert native['symbol']('?HandleSpellClick@Unit@@') in native['calls']('?HandleSpellClick@WorldSession@@'), 'Packet handler bypasses native AI dispatch'
print('PASS: selected click retains source class/quest/map/phase/scenario/owned-parent admission, then checks native contact, near/far transfer, exact NPC/GUID primary spellclick admission, visibility and LOS before either summon. Native NPC path retains5-yard3D/object-size distance, flight/death/charm/hostility and reputation gates. Per-GUID boarding/packet paths retained; actual geometry, renderer, faction gameplay and vehicle/client traversal remain separate.')
