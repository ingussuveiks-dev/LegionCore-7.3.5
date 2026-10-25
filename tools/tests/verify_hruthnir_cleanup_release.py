"""Verify owned497 cleanup in the selected Hruthnir AI and native lookup."""
from pathlib import Path
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_creation_release.py')))
native, instructions, positions = (previous[name] for name in ('native', 'instructions', 'positions'))
suffix = '@npc_hrutnirAI@npc_hrutnir@@'
cleanup_name = '?RemoveMagnarConversation' + suffix
clear_name = '?Clear@ObjectGuid@@'
empty_name = '??BObjectGuid@@QEBA_NXZ'
lookup_name = '?GetConversation@ObjectAccessor@@'
remove_name = '?Remove@Conversation@@'


def check_cleanup(name, owner):
    body = instructions(name)
    clear = positions(body, clear_name)[0]
    empty = positions(body, empty_name)[0]
    lookup = positions(body, lookup_name)[0]
    remove = positions(body, remove_name)[0]
    assert clear < empty < lookup < remove
    assert [entry.op_str for entry in body[clear - 2:clear]] == ['xmm0, xmmword ptr [rcx]', 'xmmword ptr [rsp + 0x20], xmm0']
    assert any(entry.op_str == 'rcx, [' + owner + ' + 0xad]' or entry.op_str == 'rcx, 0xad' for entry in body[:clear])
    assert body[empty - 1].op_str == 'rcx, [rsp + 0x20]'
    assert body[empty + 1].op_str == 'al, al' and body[empty + 2].mnemonic == 'je' and body[empty + 2].operands[0].imm > body[remove].address
    assert any(entry.op_str == 'xmm0, xmmword ptr [rsp + 0x20]' for entry in body[empty:lookup])
    assert any(entry.op_str == 'xmmword ptr [rsp + 0x30], xmm0' for entry in body[empty:lookup])
    assert any(entry.op_str == 'rdx, [rsp + 0x30]' for entry in body[empty:lookup])
    assert any(entry.op_str == 'rcx, qword ptr [' + owner + ' + 0x20]' for entry in body[empty:lookup])
    assert body[lookup + 1].op_str == 'rax, rax' and body[lookup + 2].mnemonic == 'je' and body[lookup + 2].operands[0].imm > body[remove].address
    assert body[remove - 1].op_str == 'rcx, rax'
    assert len(positions(body, clear_name)) == 1
    return body, clear, remove


check_cleanup(cleanup_name, 'rbx')
reset, clear, remove = check_cleanup('?Reset' + suffix, 'rdi')
assert positions(reset, '?RemoveFlag@Object@@')[0] < clear
destructor, clear, remove = check_cleanup('??_Gnpc_hrutnirAI@npc_hrutnir@@', 'rdi')
assert positions(destructor, '?CancelMagnarIntro' + suffix)[0] < clear
assert any(entry.op_str == 'rbx, qword ptr [rdi + 0x68]' for entry in destructor[remove:])
action = instructions('?DoAction' + suffix)
epoch = next(index for index, entry in enumerate(action) if entry.op_str == 'dword ptr [rbx + 0xa8], esi')
assert positions(action, cleanup_name) == [epoch + 1]
callback = instructions('??R<lambda_6>@?BO@??MovementInform' + suffix)
assert not positions(callback, lookup_name) and not positions(callback, remove_name)

address = native['symbol'](clear_name)
decoded = list(Cs(CS_ARCH_X86, CS_MODE_64).disasm(native['binary'][native['offset'](address):native['offset'](address) + 32], address))
assert [entry.mnemonic + ' ' + entry.op_str for entry in decoded[:3]] == ['xor eax, eax', 'mov qword ptr [rcx + 8], rax', 'mov qword ptr [rcx], rax']
assert decoded[3].mnemonic == 'ret'
factory = instructions('?GetAI@npc_hrutnir@@')
store = next(index for index, entry in enumerate(factory) if entry.op_str == 'xmmword ptr [rsi + 0xad], xmm0')
load = factory[store - 1]
assert load.mnemonic == 'movups' and load.operands[1].type == CS_OP_MEM and load.operands[1].mem.base == X86_REG_RIP
address = load.address + load.size + load.operands[1].mem.disp
binary = native['binary']
pe = struct.unpack_from('<I', binary, 0x3c)[0]
headers = pe + 24 + struct.unpack_from('<H', binary, pe + 20)[0]
zero_filled = False
for section in range(struct.unpack_from('<H', binary, pe + 6)[0]):
    virtual_size, rva, raw_size, raw = struct.unpack_from('<IIII', binary, headers + section * 40 + 8)
    relative = address - native['base'] - rva
    if 0 <= relative and relative + 16 <= virtual_size:
        zero_filled = relative >= raw_size or binary[raw + relative:raw + relative + 16] == bytes(16)
        break
assert zero_filled, 'Factory identity does not load16 bytes of native PE zero-initialized storage'
assert any(entry.op_str == 'ecx, 0xc0' for entry in factory[:store])

lookup = instructions(lookup_name)
find = positions(lookup, '?Find@?$HashMapHolder@VConversation@@@@')[0]
predelete = positions(lookup, '?IsPreDelete@Object@@')[0]
assert lookup[2].op_str == 'rdi, qword ptr [rcx + 8]'
assert lookup[3].op_str == 'rdi, rdi' and lookup[4].mnemonic == 'jne' and lookup[5].op_str == 'eax, eax' and lookup[8].mnemonic == 'ret'
assert lookup[find + 2].op_str == 'rax, rax' and lookup[find + 3].mnemonic == 'je'
assert lookup[find + 4].op_str == 'qword ptr [rax + 8], rdi' and lookup[find + 5].mnemonic == 'jne'
assert lookup[predelete - 1].op_str == 'rcx, rax'
assert lookup[predelete + 1].op_str == 'al, al' and lookup[predelete + 2].mnemonic == 'jne'
assert lookup[find + 3].operands[0].imm == lookup[find + 5].operands[0].imm == lookup[predelete + 2].operands[0].imm
death_name = '?JustDied' + suffix
address = native['symbol'](death_name)
decoded = list(Cs(CS_ARCH_X86, CS_MODE_64).disasm(binary[native['offset'](address):native['offset'](address) + 16], address))
assert [entry.mnemonic + ' ' + entry.op_str for entry in decoded[:2]] == ['mov rax, qword ptr [rcx]', 'jmp qword ptr [rax + 0x28]']
selection = previous
while 'selected_tables' not in selection:
    selection = selection['previous']
assert len(selection['selected_tables']('?GetAI@npc_hrutnir@@', ((0x130, death_name), (0x28, '?Reset' + suffix)))) == 1
kill = instructions('?Kill@Unit@@')
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x130 for entry in kill)
print('PASS: selected Hruthnir AI initializes an empty128-bit owned497 identity and publishes only successful creation. Reset, new1060 epoch and actual deleting destructor copy/clear before native map-local lookup and remove only the found object; empty/missing/foreign-map/predeleted guards and native Clear verified. Native Unit::Kill slot130 selects the new killer-independent death hook, which tail-dispatches this selected Reset slot28. Successful6000ms cue does not cut dialogue short. Disabled-AI death, implicit owner abandonment/map-unload and client playback remain separate.')
