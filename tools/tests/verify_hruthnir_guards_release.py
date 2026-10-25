"""Check the installed scoped guard deaths and acknowledged companion handoff."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_offer_release.py')))
native = previous['native']
instructions = previous['instructions']
positions = previous['positions']
suffix = '@instance_shields_rest_InstanceMapScript@instance_shields_rest@@'
advance = '?TryAdvanceMagnarGuards' + suffix


def find(body, operand, mnemonic=None):
    return next(index for index, entry in enumerate(body)
                if entry.op_str == operand and (not mnemonic or entry.mnemonic == mnemonic))


factory = instructions('?GetInstanceScript@instance_shields_rest@@')
table_load = next(index for index, entry in enumerate(factory)
                  if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP)
entry = factory[table_load]
table = entry.address + entry.size + entry.operands[1].mem.disp
assert factory[table_load + 1].op_str == 'qword ptr [rdi], rax'
slots = struct.unpack_from('<64Q', native['binary'], native['offset'](table))
for method in ('?OnCreatureCreate', '?OnGameObjectCreate', '?OnUnitDeath', '?Initialize', '?Update'):
    assert slots.count(native['symbol'](method + suffix)) == 1, f'Factory does not select repaired {method}'


create = instructions('?OnCreatureCreate' + suffix)
assert find(create, 'rdi, [rbx + 0xa18]', 'lea') < find(create, 'qword ptr [rbx + 0x3a88], 0x583c4', 'cmp')
for spawn, guid, death, flag in ((361412, 0x314, 0x334, 'bpl'), (361413, 0x324, 0x335, 'dil')):
    gate = find(create, f'qword ptr [rbx + 0x3a88], {hex(spawn)}', 'cmp')
    store = find(create, f'byte ptr [rsi + {hex(death)}], {flag}', 'mov')
    assert gate < store and create[gate + 1].mnemonic == 'jne'
    segment = create[gate:store]
    old = find(segment, f'byte ptr [rsi + {hex(death)}], 0', 'cmp')
    clear = segment[old + 1].operands[0].imm
    assert segment[old + 1].mnemonic == 'je'
    alive = find(segment, 'dword ptr [rdi], 0', 'cmp')
    assert segment[alive + 1].mnemonic == 'je' and segment[alive + 1].operands[0].imm == clear
    assert segment[alive + 2].op_str == f'{flag}, 1'
    assert next(entry for entry in segment if entry.address == clear).op_str == f'{flag}, {flag}'
    assert len(positions(segment, '?GetGuidValue@Object@@')) == 1
    assert segment[-1].op_str == f'xmmword ptr [rsi + {hex(guid)}], xmm0'
assert find(create, 'eax, 0x178c7', 'cmp') < find(create, 'qword ptr [rbx + 0x3a88], 0x583c4', 'cmp')
assert find(create, 'eax, 0x178d4', 'cmp') < find(create, 'qword ptr [rbx + 0x3a88], 0x5835a', 'cmp') < find(create, 'xmmword ptr [rsi + 0x336], xmm0', 'movups')
objects = instructions('?OnGameObjectCreate' + suffix)
assert find(objects, 'eax, 0x3b8b9', 'cmp') < find(objects, 'rax, 0x20c1a', 'cmp') < find(objects, 'xmmword ptr [rdi + 0x346], xmm0', 'movups')

death = instructions('?OnUnitDeath' + suffix)
first = find(death, 'dword ptr [rdi + 0xa18], 0', 'cmp')
assert death[first + 1].mnemonic == 'je'
for operand, branch in (('qword ptr [rdi + 8], rcx', 'jne'), ('eax, 0x38d', 'jne'), ('eax, 2', 'ja')):
    gate = find(death[first:], operand, 'cmp') + first
    assert death[gate + 1].mnemonic == branch
for guid, state in ((0x314, 0x334), (0x324, 0x335)):
    begin = find(death, f'rdx, [rsi + {hex(guid)}]', 'lea')
    claim = find(death, f'byte ptr [rsi + {hex(state)}], 1', 'mov')
    assert positions(death[begin:claim], '??8ObjectGuid@@') and death[claim - 1].mnemonic == 'je'
assert any(entry.mnemonic in ('call', 'jmp') and entry.operands[0].type == CS_OP_IMM
           and native['resolve'](entry.operands[0].imm) == native['symbol'](advance) for entry in death)

body = instructions(advance)
for operand, branch in (('byte ptr [rcx + 0x357], 0', 'jne'), ('byte ptr [rcx + 0x358], 0', 'jne')):
    gate = find(body, operand, 'cmp')
    assert branch in [entry.mnemonic for entry in body[gate + 1:gate + 3]]
zero = find(body, 'edi, edi', 'xor')
stage = find(body[zero:], 'eax, 2', 'cmp') + zero
assert body[stage + 1].mnemonic == 'jne'
for displacement in (0x334, 0x335):
    gate = find(body, f'byte ptr [rbp + {hex(displacement)}], dil', 'cmp')
    assert zero < stage < gate and body[gate + 1].mnemonic == 'je'
    assert body[stage + 1].operands[0].imm > body[gate + 1].address
for operand in ('eax, 0x38d', 'eax, 2', 'eax, 3', 'byte ptr [rbp + 0x356], 0'):
    assert find(body, operand, 'cmp') < positions(body, '?UpdateAchievementCriteria@Player@@')[0]
for operand, branch in (('byte ptr [rsi + 0x4ff0], 1', 'jne'), ('byte ptr [rsi + 0x6848], 0', 'jne'),
                         ('byte ptr [rsi + 0x6849], 0', 'jne'), ('al, 1', 'jne'),
                         ('qword ptr [rsi + 8], rcx', 'jne'), ('edi, eax', 'jne')):
    gate = find(body, operand)
    assert body[gate + 1].mnemonic == branch
for dependency in ('?CanContact@Player@@', '?InSamePhase@WorldObject@@', '?IsWithinDistInMap@WorldObject@@',
                   '?GetQuestRewardStatus@Player@@', '?GetQuestStatus@Player@@'):
    assert positions(body, dependency)
radius = next(entry for entry in body if entry.mnemonic == 'movss' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP)
assert struct.unpack_from('<f', native['binary'], native['offset'](radius.address + radius.size + radius.operands[1].mem.disp))[0] == 150.0
credit = positions(body, '?UpdateAchievementCriteria@Player@@')[0]
assert find(body, 'byte ptr [rbp + 0x358], 1', 'mov') < find(body, 'byte ptr [rbp + 0x356], 1', 'mov') < credit
assert find(body, 'edx, 0x5c', 'mov') < find(body, 'r8d, 0xb516', 'mov') < credit
epoch = find(body, 'ebx, dword ptr [rbp + 0x35c]', 'cmp')
assert epoch > credit and body[epoch + 1].mnemonic == 'jne'
recheck = body[epoch:]
for operand, branch in (('eax, 0x38d', 'jne'), ('eax, 3', 'jne'), ('dword ptr [rbx + 0xa18], 0', 'jne'),
                         ('byte ptr [rbx + 0x39c5], 0', 'jne'), ('eax, 0x178d4', 'jne'),
                         ('byte ptr [rbx + 0x858], 0', 'je'), ('qword ptr [rbx + 0x9c0], 0', 'je'), ('eax, 0x3b8b9', 'jne')):
    gate = find(recheck, operand, 'cmp')
    assert recheck[gate + 1].mnemonic == branch
claim = find(body, 'byte ptr [rbp + 0x357], 1', 'mov')
door = find(body, 'qword ptr [rax + 0xa0]', 'call')
action = find(body, 'qword ptr [rax + 0x48]', 'call')
assert epoch < claim < door < action < find(body, 'byte ptr [rbp + 0x358], 0', 'mov')
assert body[door - 2].op_str == 'edx, 4' and body[action - 2].op_str == 'edx, 2'
reset = instructions('?Initialize' + suffix)
for operand in ('word ptr [rdi + 0x334], bp', 'word ptr [rdi + 0x356], bp', 'byte ptr [rdi + 0x358], bpl'):
    assert find(reset, operand, 'mov') >= 0
assert find(reset, 'dword ptr [rdi + 0x35c]', 'inc') >= 0
update = instructions('?Update' + suffix)
retry = positions(update, advance)[0]
assert update[retry - 1].op_str == 'dword ptr [rcx + 0x364], 0x1f4'
assert retry < find(update, 'eax, 0x42c', 'cmp')
print('PASS: installed shared instance tracks only361412/361413 actual GUIDs, accepts dead909 actors, gates2->3 on eligible39191 reference, retains500ms retry/epoch and once-only243897/Hruthnir action2. No Hunter door mutation or client traversal certificate.')
