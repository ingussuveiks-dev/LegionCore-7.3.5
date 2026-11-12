from linked_player_abi import inherited_operand
"""Selected registered Holy cemetery and preserved native instance lifecycle."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

p = runpy.run_path(str(Path(__file__).with_name('verify_tuure_captain_release.py')))
n, instructions = p['n'], p['instructions']
prefix = '?GetClosestGraveYard@instance_tuure_InstanceMapScript@instance_tuure@@'
body = instructions(prefix)
calls = n['calls'](prefix)


def sites(name):
    targets = {n['resolve'](v) for k, v in n['symbols'].items() if k.startswith(name)}
    return [i for i, o in enumerate(body) if o.mnemonic in ('call', 'jmp') and o.operands[0].type == CS_OP_IMM and n['resolve'](o.operands[0].imm) in targets]


def at(text, mnemonic=None):
    return next(i for i, o in enumerate(body) if o.op_str == text and (mnemonic is None or o.mnemonic == mnemonic))


assert sites('?GetScenario@ScenarioMgr@@'), 'Release lacks scoped Holy cemetery recovery'
selected = []
scope = 'instance_tuure_InstanceMapScript@instance_tuure@@'
for reference in p['references']('?GetInstanceScript@instance_tuure@@'):
    table = struct.unpack_from('<40Q', n['binary'], n['offset'](reference))
    if all(table[slot] == n['symbol']('?' + method + '@' + scope) for method, slot in (('OnCreatureCreate', 3), ('CreatureDies', 5), ('OnUnitDeath', 15), ('Update', 29))):
        selected.append(table)
assert len(selected) == 1
address = n['symbol'](prefix)
assert selected[0].count(address) == 1, 'Factory does not select repaired cemetery'
slot = selected[0].index(address) * 8
dispatch = instructions('?GetClosestGraveYard@InstanceMap@@')
assert any(o.mnemonic == 'call' and o.operands[0].type == CS_OP_MEM and o.operands[0].mem.disp == slot for o in dispatch)
scoped = at('eax, 0x43d', 'cmp')
fallback = body[scoped + 1].operands[0].imm
assert body[scoped + 1].mnemonic == 'jne'
marksman = next((i for i, o in enumerate(body) if o.mnemonic == 'cmp' and o.op_str == 'eax, 0x3cc'), None)
if marksman is not None:
    assert any(o.address == fallback for o in body[marksman - 4:marksman]) and any(i in sites('?GetScenarioId@Scenario@@') for i in range(marksman - 4, marksman))
    assert body[marksman + 1].mnemonic == 'jne'
    fallback = body[marksman + 1].operands[0].imm
blood = next((i for i, o in enumerate(body) if o.mnemonic == 'cmp' and o.op_str == 'eax, 0x3ac'), None)
if blood is not None:
    assert marksman is not None and any(o.address == fallback for o in body[blood - 4:blood]) and any(i in sites('?GetScenarioId@Scenario@@') for i in range(blood - 4, blood))
    assert body[blood + 1].mnemonic == 'jne'
    fallback = body[blood + 1].operands[0].imm
hodir = next((i for i, o in enumerate(body) if o.mnemonic == 'cmp' and o.op_str == 'eax, 0x47a'), None)
if hodir is not None:
    assert blood is not None and any(o.address == fallback for o in body[hodir - 4:hodir]) and any(i in sites('?GetScenarioId@Scenario@@') for i in range(hodir - 4, hodir))
    assert body[hodir + 1].mnemonic == 'jne'
    fallback = body[hodir + 1].operands[0].imm
null_branch = next(o for o in body[sites('?GetScenario@ScenarioMgr@@')[0] + 1:scoped] if o.mnemonic == 'je')
assert null_branch.operands[0].imm == fallback
data_reg, map_reg, map_word = ('rsi', 'edi', 'di') if blood is not None else ('rdi', 'esi', 'si')
lookup = at(data_reg + inherited_operand(', qword ptr [rax + 0xab40]'), 'mov')  #5480*8
identity = at('dword ptr [' + data_reg + '], 0x1568', 'cmp')
failure = body[identity + 1].operands[0].imm
assert body[lookup + 2].operands[0].imm == failure
mapcheck = at('word ptr [' + data_reg + ' + 0x1c], ' + map_word, 'cmp')
assert body[mapcheck + 1].operands[0].imm == failure
at(map_reg + ', 0x5d1', 'mov')
zero = at('xmm0, xmm1', 'ucomiss')
assert any(o.mnemonic == 'je' and o.operands[0].imm == failure for o in body[zero:zero + 10])
coordinates = sites('?IsValidMapCoord@MapManager@@SA_NIMMMM@Z')
assert len(coordinates) == (5 if hodir is not None else 4 if blood is not None else 2 if marksman is not None else 1)
coord = coordinates[0]
assert scoped < lookup < identity < mapcheck < coord and body[coord + 2].operands[0].imm == failure
for register, expected in (('xmm6', 3.141592653589793), ('xmm7', 180.0)):
    o = next(o for o in body[mapcheck:coord] if o.mnemonic == 'movss' and o.op_str.startswith(register + ',') and 'rip' in o.op_str)
    operand = next(a for a in o.operands if a.type == CS_OP_MEM and a.mem.base == X86_REG_RIP)
    value, = struct.unpack_from('<f', n['binary'], n['offset'](o.address + o.size + operand.mem.disp))
    assert abs(value - expected) < 1e-5
relocation_tail = None
if hodir is not None:
    # The larger callback makes MSVC keep WorldRelocate out of line. Bind the
    # actual ABI arguments and native stores rather than accepting any call.
    expected_args = [
        ('movss', 'xmm0, dword ptr [rsi + 0x18]'), ('mulss', 'xmm0, xmm6'), ('divss', 'xmm0, xmm7'),
        ('movss', 'dword ptr [rsp + 0x28], xmm0'), ('movss', 'xmm0, dword ptr [rsi + 0x14]'),
        ('movss', 'dword ptr [rsp + 0x20], xmm0'), ('movss', 'xmm3, dword ptr [rsi + 0x10]'),
        ('movss', 'xmm2, dword ptr [rsi + 0xc]'), ('mov', 'edx, edi'), ('lea', 'rcx, [r14 + 0x2e0]')]
    assert [(o.mnemonic, o.op_str) for o in body[coord + 3:coord + 13]] == expected_args
    relocate, = sites('?WorldRelocate@WorldLocation@@')
    assert relocate == coord + 13 and body[relocate + 1].op_str == 'rax, [r14 + 0x2e0]'
    relocation_tail = body[coord + 11].address
    native_relocate = instructions('?WorldRelocate@WorldLocation@@')
    assert [(o.mnemonic, o.op_str) for o in native_relocate[:9]] == [
        ('push', 'rbx'), ('sub', 'rsp, 0x20'), ('movss', 'xmm0, dword ptr [rsp + 0x50]'),
        ('mov', 'rbx, rcx'), ('movss', 'dword ptr [rcx + 8], xmm0'), ('movss', 'xmm0, dword ptr [rsp + 0x58]'),
        ('movss', 'dword ptr [rcx], xmm2'), ('movss', 'dword ptr [rcx + 4], xmm3'), ('mov', 'dword ptr [rcx + 0x10], edx')]
    assert native_relocate[9].mnemonic == 'call' and n['resolve'](native_relocate[9].operands[0].imm) == n['symbol']('?NormalizeOrientation@Position@@')
    assert (native_relocate[10].mnemonic, native_relocate[10].op_str) == ('movss', 'dword ptr [rbx + 0xc], xmm0')
    stores = [(o.mnemonic, o.op_str) for o in body[coord + 3:] if o.op_str.startswith(('dword ptr [rbx]', 'dword ptr [rbx + 4]', 'dword ptr [rbx + 8]'))]
    assert stores == [('movss', 'dword ptr [rbx], xmm8'), ('movss', 'dword ptr [rbx + 4], xmm7'), ('movss', 'dword ptr [rbx + 8], xmm6')]
else:
    stores = [(o.mnemonic, o.op_str) for o in body[coord + 3:] if o.op_str.startswith(('dword ptr [rbx]', 'dword ptr [rbx + 4]', 'dword ptr [rbx + 8]'))]
    assert stores == [('movss', 'dword ptr [rbx], xmm1'), ('movss', 'dword ptr [rbx + 4], xmm2'), ('movss', 'dword ptr [rbx + 8], xmm3'), ('movss', 'dword ptr [rbx], xmm8'), ('movss', 'dword ptr [rbx + 4], xmm7'), ('movss', 'dword ptr [rbx + 8], xmm6')]
fallback_index = next(i for i, o in enumerate(body) if o.address == fallback)
assert body[fallback_index].op_str == 'rbx, [r14 + 0x2e0]'
assert body[fallback_index + 1].op_str == 'dword ptr [rbx + 0x10], 0x5d1'
refused = at('eax, eax', 'xor')
assert any(o.mnemonic == 'je' and o.operands[0].imm == body[refused].address for o in body[coord:refused])
assert b'Tuure graveyard: invalid native WorldSafeLocs 5480 for instance %u' in n['binary']
for forbidden in ('?TeleportTo@Player@@', '?ResurrectPlayer@Player@@', '?CompleteQuest@Player@@', '?AddItem@Player@@', '?KilledMonsterCredit@Player@@', '?UpdateAchievementCriteria@Player@@', '?SetCurrentStep@Scenario@@', '?SetHealth@Unit@@'):
    assert not any(name.startswith(forbidden) and n['resolve'](a) in calls for name, a in n['symbols'].items()), forbidden
print('PASS linked Tuure cemetery:actual registered instance/virtual dispatch,1085-only5480/map/zero/finite source guards,radian conversion/native XYZ writes,diagnostic/null failure,remaining shared/no-scenario death-pose fallback retained;separate972 branch inspected when present,no direct resurrection/progress grant. Retained first-fight/route/cage/captain native hooks;full client separate.')
