"""Verify installed tracked wave deaths, release barrier and advance ordering."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_magnar_finale_release.py')))
native = previous['native']
instructions = previous['instructions']
suffix = '@npc_magnar_icebreakerAI@npc_magnar_icebreaker@@'
death = '?SummonedCreatureDies' + suffix
advance = '?AdvanceWaveIfReady' + suffix
launch = '?LaunchEventWave' + suffix


def calls_to(body, dependency):
    target = native['symbol'](dependency)
    return [index for index, entry in enumerate(body) if entry.mnemonic in ('call', 'jmp')
            and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == target]


body = instructions(death)
assert body[0].op_str == 'rdx, rdx' and body[1].mnemonic == 'je' and body[1].operands[0].imm == body[-1].address
alive = next(index for index, entry in enumerate(body) if entry.op_str == 'dword ptr [rdx + 0xa18], 0')
assert body[alive + 1].op_str == 'r13, r8', 'Actual killer not retained across GUID accounting'
assert body[alive + 4].mnemonic == 'je', 'Living summon can consume a death'
erase = calls_to(body, '?_Erase@?$_Tree@V?$_Tset_traits@VObjectGuid@@')[0]
assert any(entry.op_str == 'rcx, [r15 + 0xd8]' for entry in body[erase - 5:erase]), 'Erase is not scoped to this actors active GUID set'
claimed = next(index for index, entry in enumerate(body[erase:], erase) if entry.mnemonic == 'test' and entry.op_str == 'rax, rax')
assert body[claimed + 1].mnemonic == 'je'
assert body[claimed + 2].op_str == 'eax, byte ptr [r15 + 0xd4]'
assert body[claimed + 3].op_str == 'al, al' and body[claimed + 4].mnemonic == 'je'
assert body[claimed + 1].operands[0].imm == body[claimed + 4].operands[0].imm == body[alive + 4].operands[0].imm
assert body[claimed + 5].mnemonic == 'dec' and body[claimed + 5].op_str == 'al'
assert body[claimed + 6].op_str == 'rdx, r13', 'Counted death lost killer ownership context'
assert body[claimed + 8].op_str == 'byte ptr [r15 + 0xd4], al' and calls_to(body, advance) == [claimed + 9]

policy = instructions(advance)
rejection = None
for displacement, branch in ((0xd5, 'jne'), (0xd4, 'jne'), (0xd1, 'je'), (0xd0, 'je')):
    position = next(index for index, entry in enumerate(policy) if entry.mnemonic == 'cmp' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == displacement)
    guarded = position + (3 if displacement == 0xd5 else 1)
    assert policy[guarded].mnemonic == branch
    destination = policy[guarded].operands[0].imm
    rejection = rejection or destination
    assert destination == rejection
assert any(entry.mnemonic == 'lea' and entry.op_str == 'eax, [rcx - 2]' for entry in policy)
limit = next(index for index, entry in enumerate(policy) if entry.mnemonic == 'cmp' and entry.op_str == 'al, 4')
assert policy[limit + 1].mnemonic == 'ja' and policy[limit + 1].operands[0].imm == rejection
assert policy[limit + 2].op_str == 'cl, 6' and policy[limit + 3].mnemonic == 'jae'
assert policy[limit + 4].op_str == 'eax, [rcx + 1]' and policy[limit + 7].op_str == 'byte ptr [rbx + 0xd3], al'
assert calls_to(policy, launch) == [limit + 11] and policy[limit + 11].mnemonic == 'jmp'
selection = calls_to(policy, '?GetEncounterPlayer' + suffix)[0]
assert policy[selection - 3].op_str == 'r8d, 3' and policy[selection - 2].op_str == 'rdx, rdi'
assert policy[selection + 1].op_str == 'rdi, rax' and policy[selection + 2].op_str == 'rax, rax'
assert policy[selection + 3].mnemonic == 'je' and policy[selection + 3].operands[0].imm == rejection, 'Missing recipient consumes wave completion'
for operand in ('eax, 0x17722', 'dword ptr [rax + 0xe0], 0x5d7'):
    position = next(index for index, entry in enumerate(policy) if entry.mnemonic == 'cmp' and entry.op_str == operand)
    assert policy[position + 1].mnemonic == 'jne' and policy[position + 1].operands[0].imm == rejection
claim = next(index for index, entry in enumerate(policy) if entry.mnemonic == 'inc' and entry.op_str == 'byte ptr [rbx + 0xd3]')
unprotect = calls_to(policy, '?RemoveFlag@Object@@')[0]
credit = calls_to(policy, '?UpdateAchievementCriteria@Player@@')[0]
assert selection < claim < unprotect < credit
assert policy[unprotect - 2].op_str == 'r8d, 0x2000180' and policy[unprotect - 1].op_str == 'word ptr [rbx + 0xd0], 0', 'Duel state must be claimed before reentrant flag or credit callbacks'
assert policy[credit - 2].op_str == 'r8d, 0xb55b' and policy[credit - 1].op_str == 'rcx, rdi'
assert any(entry.op_str == 'edx, 0x5c' for entry in policy[credit - 8:credit])
assert not calls_to(policy, '?FindNearestPlayer@WorldObject@@')

released = instructions(launch)
armed = next(index for index, entry in enumerate(released) if entry.op_str == 'byte ptr [r13 + 0xd5], 1')
finished = next(index for index, entry in enumerate(released) if entry.op_str == 'byte ptr [r13 + 0xd5], 0')
assert armed < finished < calls_to(released, advance)[0]
maps = calls_to(released, '?GetCreature@Map@@')
assert len(maps) == 2 and armed < maps[0] < maps[1] < finished
for position in maps:
    assert released[position + 2].op_str == 'rax, rax' and released[position + 3].mnemonic == 'je'
    assert released[position + 4].op_str == 'dword ptr [rax + 0xa18], 0' and released[position + 5].mnemonic == 'jne'
    assert released[position + 3].operands[0].imm == released[position + 5].operands[0].imm
assert any(entry.mnemonic == 'mov' and entry.op_str == 'ecx, 0x20' for entry in released[armed:maps[0]]), 'Release no longer allocates the independent list snapshot'
assert any(entry.mnemonic == 'movups' and entry.op_str == 'xmm0, xmmword ptr [rbx + 0x10]' for entry in released[armed:maps[0]]), 'Snapshot GUID payload copy missing'
insertions = {native['resolve'](address) for name, address in native['symbols'].items() if name.startswith('?_Insert_node@') and 'ObjectGuid' in name}
insert = next(index for index, entry in enumerate(released) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) in insertions)
assert maps[0] < insert < calls_to(released, '?RemoveFlag@Object@@')[0], 'Actor made attackable before active GUID registration'
values = {operand.imm for entry in released for operand in entry.operands if operand.type == CS_OP_IMM}
assert {191032, 191821, 64367, 96468} <= values and 46427 not in values
assert not calls_to(released, '?UpdateAchievementCriteria@Player@@'), 'Release switch still bypasses guarded stage3 completion'
assert any(entry.mnemonic in ('movss', 'mulss', 'divss', 'comiss') and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP
           and struct.unpack_from('<f', native['binary'], native['offset'](entry.address + entry.size + operand.mem.disp))[0] == 80.0
           for entry in released for operand in entry.operands), 'Original final80pct request changed'
switch = next(index for index, entry in enumerate(released) if entry.mnemonic == 'sub' and entry.op_str == 'edx, 1')
for offset, count in ((0, 2), (2, 2), (4, 1), (6, 2)):
    assert released[switch + offset].mnemonic == 'sub' and released[switch + offset].op_str == 'edx, 1'
    branch = released[switch + offset + 1]
    assert branch.mnemonic == 'je'
    target = next(entry for entry in released if entry.address == branch.operands[0].imm)
    assert target.mnemonic == 'mov' and target.op_str == f'r12b, {count}'
assert released[switch + 8].op_str == 'edx, 1' and released[switch + 8].mnemonic == 'cmp'
assert released[switch + 9].mnemonic == 'jne' and released[switch + 10].op_str == 'r12b, 3'
assert released[switch + 11].op_str == 'byte ptr [rbp + 0x6f], dl', 'Fifth wave no longer enables Windwall/health work'

updated = instructions('?UpdateAI' + suffix)
victim = calls_to(updated, '?UpdateVictim@CreatureAI@@')[0]
retry = calls_to(updated, advance)[0]
pending = next(index for index, entry in enumerate(updated) if entry.op_str == 'byte ptr [rcx + 0xd3], 6')
assert pending < retry < victim and updated[pending + 3].mnemonic == 'jne'
assert updated[pending + 3].operands[0].imm == updated[retry + 1].address and updated[retry - 1].op_str == 'edx, edx', 'Tick retry must be pending-wave-only with no retained raw killer pointer'
assert updated[victim + 1].op_str == 'al, al' and updated[victim + 2].mnemonic == 'je'
assert updated[victim + 3].op_str == 'byte ptr [rbx + 0xd0], 0' and updated[victim + 4].mnemonic == 'jne'
assert updated[victim + 2].operands[0].imm == updated[victim + 4].operands[0].imm
speech = calls_to(updated, '?CreateConversation@Unit@@')
assert len(speech) == 1 and updated[speech[0] - 1].op_str == 'edx, 0x1f2'
assert victim < calls_to(updated, '?ExecuteEvent@EventMap@@')[0] < speech[0], '498 no longer follows the resumed combat event queue'
start = calls_to(updated, launch)[0]
assert updated[start - 2].op_str == 'eax, [rdx + 1]' and updated[start - 1].op_str == 'byte ptr [rbx + 0xd3], al', 'Initial wave ID still increments after reentrant release'
reset = instructions('?Reset' + suffix)
assert all(any(entry.op_str == f'byte ptr [rbx + {hex(displacement)}], 0' for entry in reset) for displacement in (0xd5, 0xd4))
assert native['symbol']('?clear@?$_Tree@V?$_Tset_traits@VObjectGuid@@') in native['calls']('?Reset' + suffix)
tables = []
for entry in instructions('?GetAI@npc_magnar_icebreaker@@'):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            offset = native['offset'](entry.address + entry.size + operand.mem.disp)
            if struct.unpack_from('<Q', native['binary'], offset + 0x158)[0] == native['symbol'](death): tables.append(offset)
assert len(tables) == 1
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x158 for entry in instructions('?Kill@Unit@@'))
print('PASS: canonical tracked death callback forwards actual killer after once-only GUID consumption; bounded wave6 completion uses eligible owner-first stage3 selection before duel claim/flag release/46427, missing recipient stays pending, and native UpdateAI retries before victim/cast gates. Independent snapshot/release barrier,2/2/1/2/3, Windwall/80pct and resumed498 retained. Full actor recovery/group criteria/client VFX not certified.')
