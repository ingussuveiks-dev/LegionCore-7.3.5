"""Verify installed tracked wave deaths, release barrier and advance ordering."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP, X86_REG_RDX

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
assert body[alive + 3].mnemonic == 'je', 'Living summon can consume a death'
erase = calls_to(body, '?_Erase@?$_Tree@V?$_Tset_traits@VObjectGuid@@')[0]
assert any(entry.op_str == 'rcx, [r15 + 0xd8]' for entry in body[erase - 5:erase]), 'Erase is not scoped to this actors active GUID set'
claimed = next(index for index, entry in enumerate(body[erase:], erase) if entry.mnemonic == 'test' and entry.op_str == 'rax, rax')
assert body[claimed + 1].mnemonic == 'je'
assert body[claimed + 2].op_str == 'eax, byte ptr [r15 + 0xd4]'
assert body[claimed + 3].op_str == 'al, al' and body[claimed + 4].mnemonic == 'je'
assert body[claimed + 1].operands[0].imm == body[claimed + 4].operands[0].imm == body[alive + 3].operands[0].imm
assert body[claimed + 5].mnemonic == 'dec' and body[claimed + 5].op_str == 'al'
assert body[claimed + 7].op_str == 'byte ptr [r15 + 0xd4], al' and calls_to(body, advance) == [claimed + 8]

address = native['symbol'](advance)
following = min(value for value in native['symbols'].values() if value > address)
assert 0 < following - address <= 96, 'Unexpected leaf policy bounds'
decoder = previous['previous']['decoder']
policy = list(decoder.disasm(native['binary'][native['offset'](address):native['offset'](following)], address))
rejection = None
for displacement, branch in ((0xd5, 'jne'), (0xd4, 'jne'), (0xd1, 'je'), (0xd0, 'je')):
    position = next(index for index, entry in enumerate(policy) if entry.mnemonic == 'cmp' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == displacement)
    assert policy[position + 1].mnemonic == branch
    destination = policy[position + 1].operands[0].imm
    rejection = rejection or destination
    assert destination == rejection
assert any(entry.mnemonic == 'lea' and entry.op_str == 'eax, [rdx - 2]' for entry in policy)
limit = next(index for index, entry in enumerate(policy) if entry.mnemonic == 'cmp' and entry.op_str == 'al, 4')
assert policy[limit + 1].mnemonic == 'ja' and policy[limit + 1].operands[0].imm == rejection
assert policy[limit + 2].op_str == 'eax, [rdx + 1]' and policy[limit + 3].op_str == 'byte ptr [rcx + 0xd3], al'
assert calls_to(policy, launch) == [limit + 4] and policy[limit + 4].mnemonic == 'jmp'

released = instructions(launch)
armed = next(index for index, entry in enumerate(released) if entry.op_str == 'byte ptr [rbx + 0xd5], 1')
finished = next(index for index, entry in enumerate(released) if entry.op_str == 'byte ptr [rax + 0xd5], 0')
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
assert {191032, 191821, 46427, 64367, 96468} <= values
assert any(entry.mnemonic in ('movss', 'mulss', 'divss', 'comiss') and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP
           and struct.unpack_from('<f', native['binary'], native['offset'](entry.address + entry.size + operand.mem.disp))[0] == 80.0
           for entry in released for operand in entry.operands), 'Original final80pct request changed'
dispatch = next(entry for entry in released if entry.mnemonic == 'mov' and len(entry.operands) == 2
                and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.index == X86_REG_RDX and entry.operands[1].mem.scale == 4)
targets = struct.unpack_from('<6I', native['binary'], native['offset'](native['base'] + dispatch.operands[1].mem.disp))
assert targets[0] == targets[1] == targets[3]
for target, count in zip(targets[:5], (2, 2, 1, 2, 3)):
    entry = next(entry for entry in released if entry.address == native['base'] + target)
    assert entry.mnemonic == 'mov' and entry.op_str == f'r12b, {count}'

updated = instructions('?UpdateAI' + suffix)
victim = calls_to(updated, '?UpdateVictim@CreatureAI@@')[0]
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
print('PASS: canonical selected Magnar death callback consumes only nonliving active GUIDs once before bounded advancement; claimed wave IDs and independent snapshot/release barrier survive synchronous deaths, original2/2/1/2/3 and Windwall/80pct/46427 retained. Full actor recovery/group criteria/client VFX not certified.')
