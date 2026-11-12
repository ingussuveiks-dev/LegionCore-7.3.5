from linked_player_abi import player_offset,player_operand
"""Select actual native wave contact, retry and completed seat acknowledgement."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM

p = runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_wave_release.py')))
n, ins = p['n'], p['n']['instructions']
assert any(k.startswith('?Enabled@SharasdalWaveRide@@') for k in n['symbols']), 'Release lacks acknowledged Sharasdal wave contact'

def sites(body, prefix):
    targets = {n['resolve'](v) for k, v in n['symbols'].items() if k.startswith(prefix)}
    return [i for i, x in enumerate(body) if x.mnemonic == 'call' and x.operands[0].type == CS_OP_IMM
            and n['resolve'](x.operands[0].imm) in targets]

def at(body, mnemonic, operands):
    return next(i for i, x in enumerate(body) if (x.mnemonic, x.op_str) == (mnemonic, operands))

def sequence(body, rows):
    pairs = [(x.mnemonic, x.op_str) for x in body]
    return next(i for i in range(len(pairs)-len(rows)+1) if pairs[i:i+len(rows)] == rows)

own, enabled = ins('?Owns@SharasdalWaveRide@@'), ins('?Enabled@SharasdalWaveRide@@')
for body in (own, enabled):
    for value in ('eax, 0x19b23', 'eax, 0x330ec', 'eax, 0x42a'):
        i = at(body, 'cmp', value)
        assert body[i+1].mnemonic == 'jne'
    assert len(sites(body, '?GetScenario@ScenarioMgr@@')) == 1
    assert len(sites(body, '?GetScenarioId@Scenario@@')) == 1
    assert len(sites(body, '??9ObjectGuid@@')) == 2  # full128-bit caster AND target
    assert sites(body, '?IsInWorld@Object@@') and sites(body, '?IsPreDelete@Object@@')
for operands in ('dword ptr [rbp + 0xf8], 0x3e8', 'qword ptr [rbp + 0x58], 1',
                 'dword ptr [rcx + 0x10], 1', 'dword ptr [rcx + 0x18], 0',
                 'dword ptr [rcx + 0x20], 0xb606', 'dword ptr [rcx + 0x1c], 0x102048',
                 'dword ptr [rcx + 0x4c], 1', 'byte ptr [rcx + 0x54], 0',
                 'dword ptr [rcx + 0x14], 5'):
    i = at(enabled, 'cmp', operands)
    assert enabled[i+1].mnemonic == 'jne'
    target = enabled[i+1].operands[0].imm
    assert any(x.address == target and (x.mnemonic, x.op_str) == ('xor', 'al, al') for x in enabled)

fill = ins('?FillCustomData@AreaTrigger@@')
owner, = sites(fill, '?Owns@SharasdalWaveRide@@')
native, = sites(fill, '?NativeAction@SharasdalWaveRide@@')
assert owner < native
for operands in ('dword ptr [rsi + 0x14], 1', 'dword ptr [rdi + 0x10], 0x17',
                 'dword ptr [rdi + 0x60], 0x33120', 'dword ptr [rdi + 0x14], 0x3e8'):
    assert native < at(fill, 'cmp', operands)
period = sequence(fill, [('mov', 'dword ptr [rsi + 0x14], 5'),
                        ('mov', 'dword ptr [rbx + 0x690], 0x3e8'),
                        ('mov', 'dword ptr [rbx + 0x558], 0x3e8')])
assert native < period  # native effect period, per-instance metadata AND initial timer

action = ins('?DoAction@AreaTrigger@@')
owner, = sites(action, '?Owns@SharasdalWaveRide@@')
native, = sites(action, '?NativeAction@SharasdalWaveRide@@')
passenger = next(i for i in sites(action, '?GetPassenger@Vehicle@@') if i > native)
empty, = sites(action, '?HasEmptySeat@Vehicle@@')
bases = sites(action, '?GetVehicleBase@Unit@@')
assert owner < native < passenger < bases[0] < empty < bases[1]
assert action[passenger-2].op_str == 'edx, edx'
assert [(x.mnemonic, x.op_str) for x in action[passenger+1:passenger+2]] == [('cmp', 'rax, r13')]
assert action[bases[0]+1].op_str == 'rax, r15'
seat = sequence(action, [('mov', 'r8b, 1'), ('mov', 'byte ptr [rsp + 0x50], r8b')])
assert bases[0] < seat < empty
late = at(action, 'cmp', 'dword ptr [r13 + 0xa18], 0')
end = action[late+1].operands[0].imm
for operands in ('qword ptr [r13 + 8], rax', player_operand('byte ptr [rax + 0x6848], 0'), player_operand('byte ptr [rax + 0x6849], 0')):
    i = at(action, 'cmp', operands)
    assert passenger < i < empty and action[i+1].mnemonic == 'jne' and action[i+1].operands[0].imm == end
assert action[empty+2].mnemonic == 'je' and action[empty+2].operands[0].imm == end
defer = sequence(action, [('cmp', 'byte ptr [rsp + 0x51], 0')])
assert action[defer+1].mnemonic == 'je' and action[defer+1].operands[0].imm == action[defer+4].address
assert action[defer+2].op_str == 'byte ptr [rsp + 0x50], 0'
assert action[defer+3].mnemonic == 'je' and action[defer+3].operands[0].imm == end
skip = at(action, 'movzx', 'eax, byte ptr [r14 + 0x712]')
assert action[skip+4].op_str == 'r8b, r8b' and action[skip+5].mnemonic == 'jne'
assert action[skip+5].operands[0].imm == action[defer-1].address
assert any(x.mnemonic == 'inc' and x.op_str == 'dword ptr [rax]' for x in action[defer+4:])
assert sites(action, '?CastSpell@Unit@@')  # original native dispatch retained

join = ins('?Execute@VehicleJoinEvent@@')
ack, = sites(join, '?AffectUnit@AreaTrigger@@')
gate, = sites(join, '?Enabled@SharasdalWaveRide@@')
lookup, = sites(join, '?GetAreaObject@Unit@@')
launch, = sites(join, '?Launch@MoveSplineInit@Movement@@')
vehicle = at(join, 'mov', 'qword ptr [rcx + 0x30d8], rax')
guid = at(join, 'movups', 'xmmword ptr [rax + 0x30], xmm0')
assert vehicle < guid < launch < lookup < gate < ack
assert at(join, 'cmp', 'eax, 0x19b23') < lookup
assert at(join, 'cmp', 'dword ptr [rcx + 0xe0], 0x640') < lookup
assert join[lookup-1].op_str == 'edx, 0x330ec'
assert [(x.mnemonic, x.op_str) for x in join[ack-3:ack]] == [
    ('mov', 'r8d, 1'), ('mov', 'rdx, qword ptr [rdi + 0x50]'), ('mov', 'rcx, rsi')]
assert join[ack-4].mnemonic == 'je' and join[ack-4].operands[0].imm == join[ack+1].address
callback = at(join, 'call', 'qword ptr [rax + 0x218]')
assert ack < callback and any(x.op_str == 'r9b, 1' for x in join[ack:callback])
pattern = re.compile(r'^\s+\w+:\w+\s+\?\?_7VehicleJoinEvent@@6B@\s+([0-9a-fA-F]{16})\s', re.M)
table, = [int(v, 16) for v in pattern.findall((n['args'].release/'worldserver.map').read_text(errors='replace'))]
assert struct.unpack_from('<4Q', n['binary'], n['offset'](table))[1] == n['symbol']('?Execute@VehicleJoinEvent@@')
for prefix in ('?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@',
               '?CompleteQuest@Player@@', '?AddItem@Player@@', '?TeleportTo@Player@@'):
    assert not sites(action, prefix) and not sites(join, prefix), prefix
print('PASS linked acknowledged Sharasdal wave:exact1066/1600/105251/6547/11160/209132/native46598 action;full-GUID ownership,1000ms native cadence and initial timer;actual seat0/base acknowledgement before hit consumption,late eligibility/pending/occupied refusal,completed native VehicleJoinEvent vtable/seat+GUID+spline before noncasting AT notification and content AI. Retained full wave/acquisition/recovery proof chain;physical moving path/client/camera/pet portrait/full1066 separate.')
