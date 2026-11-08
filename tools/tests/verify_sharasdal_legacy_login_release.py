"""Selected linked Sharasdal legacy-receipt repair, with retained acquisition/return checks.

This is not a complete saved-character or client traversal.
"""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

c = runpy.run_path(str(Path(__file__).with_name('verify_tuure_legacy_login_release.py')))
n, ins, sites, find = (c[k] for k in ('n', 'ins', 'sites', 'find'))
prefix = '?RecoverLegacySharasdalEntrance@'
assert any(k.startswith(prefix) for k in n['symbols']), 'Release lacks legacy Sharasdal checkpoint repair'
h = ins(prefix)
p = ins('?PrepareScenarioLogin@Player@@')


def constant(o):
    a = next(a for a in o.operands if a.type == CS_OP_MEM and a.mem.base == X86_REG_RIP)
    at = n['offset'](o.address + o.size + a.mem.disp)
    return struct.unpack_from('<f', n['binary'], at)[0]


def near(o, expected):
    assert abs(constant(o) - expected) < max(1e-7, abs(expected) * 1e-7), (o.op_str, expected)


unchanged = h[find(h, 'al, 7', mnemonic='cmp') + 1].operands[0].imm
assert h[find(h, 'edx, ebp', mnemonic='cmp') + 1].operands[0].imm == unchanged
assert h[find(h, 'ebp, 0x640', mnemonic='mov')].mnemonic == 'mov'
assert h[find(h, 'dword ptr [rbx], ebp', mnemonic='cmp') + 1].operands[0].imm == unchanged
assert h[find(h, 'dword ptr [rbx + 4], 0x42a', mnemonic='cmp') + 1].operands[0].imm == unchanged
legacy = [i for i, o in enumerate(h) if o.mnemonic == 'subss' and 'rip' in o.op_str][:4]
for at, value in zip(legacy, (836.16, 1035.4, 48.61, 1.7935)):
    near(h[at], value)
    branch = next(o for o in h[at + 1:at + 8] if o.mnemonic == 'jbe')
    assert branch.operands[0].imm == unchanged
near(next(o for o in h[:legacy[0] + 8] if o.mnemonic == 'movss' and o.op_str.startswith('xmm7,') and 'rip' in o.op_str), .001)

refused = h[find(h, 'qword ptr [rbx + 0x10], 0', mnemonic='cmp') + 1].operands[0].imm
for text in ('qword ptr [rbx + 0x10], 0', 'rdi, rdi', 'rcx, rcx',
 'dword ptr [rdi + 8], 0x4cd', 'word ptr [rdi + 0x40], bp', 'byte ptr [rdi + 0x46], 0xc',
 'dword ptr [rcx], 0x4cd', 'word ptr [rcx + 0x20], bp', 'word ptr [rcx + 0x24], 0',
 'byte ptr [rcx + 0x31], 0xc', 'dword ptr [rcx + 0x14], 0x800', 'rsi, rsi', 'dword ptr [rsi], ebp'):
 at = find(h, text);assert h[at+1].mnemonic in ('je','jne') and h[at+1].operands[0].imm==refused,text
instance = find(h,'dword ptr [rbx + 8], 0',mnemonic='cmp')
assert h[instance+1].op_str=='rsi, rax' and h[instance+2].operands[0].imm==refused
assert len(sites(h, '?GetScenarioSteps@ScenarioMgr@@'))==1
portal,=sites(h,'?GetSpellTargetPosition@SpellMgr@@');assert h[portal-2].op_str=='edx, 0x32b43'
raw=[i for i,o in enumerate(h) if o.mnemonic=='subss' and 'rip' in o.op_str][4:8]
for at,value in zip(raw,(-629.42,808.68,247.67,6.2797)):
 near(h[at],value);assert next(o for o in h[at+1:at+6] if o.mnemonic=='jbe').operands[0].imm==refused
coord = sites(h, '?IsValidMapCoord@MapManager@@SA_NIMMMM@Z')
assert len(coord) == 1 and h[coord[0] + 2].operands[0].imm == refused
near(next(o for o in h if o.mnemonic == 'comiss' and 'rip' in o.op_str), .0001)
assert next(o for o in h if o.mnemonic=='comiss' and 'rip' in o.op_str).mnemonic=='comiss' # native squared1cm delta; facing already radians
writes = [o.op_str for o in h if o.mnemonic in ('mov', 'movss', 'movups') and o.op_str.startswith(('dword ptr [rbx', 'qword ptr [rbx', 'xmmword ptr [rbx'))]
assert writes == ['dword ptr [rbx + 0x18], xmm3', 'dword ptr [rbx + 0x1c], eax', 'dword ptr [rbx + 0x20], eax', 'dword ptr [rbx + 0x24], eax']
assert next(o for o in h if o.address == refused).op_str == 'eax, 2'
assert next(o for o in h if o.address == unchanged).op_str == 'eax, eax'
find(h, 'eax, 1', mnemonic='mov')

call = sites(p, prefix)
assert len(call) == 1
at = call[0]
assert p[at+1].op_str=='dword ptr [rsp + 0x30], eax'
compare=at+2;assert p[compare].op_str=='eax, 2' and p[compare+1].mnemonic=='jne'
reset=find(p,'qword ptr [rsi + 0x6bd8], 0',compare)
selector = find(p, 'word ptr [rsi + 0x6ba8], di', reset)
live = sites(p, '?GetScenario@ScenarioMgr@@')[0]
assert at < reset < selector < live
# Optimized logging branches share the later function epilogue. Walk both
# conditional edges in the refusal path instead of assuming a linear ret.
by_address = {o.address: i for i, o in enumerate(p)}
pending = [compare + 2]
seen = set()
returns = 0
continuation = p[compare + 1].operands[0].imm
while pending:
    i = pending.pop()
    if i in seen:
        continue
    seen.add(i)
    o = p[i]
    assert o.address != continuation and i != live, 'Refused source reaches normal login'
    if o.mnemonic == 'ret':
        returns += 1
        continue
    if o.mnemonic == 'int3':
        continue
    if o.mnemonic.startswith('j'):
        pending.append(by_address[o.operands[0].imm])
        if o.mnemonic == 'jmp':
            continue
    pending.append(i + 1)
assert returns and reset in seen and selector in seen
complete = sites(p, '?IsCompleted@Scenario@@')[0]
keep = find(p, 'dword ptr [rsi + 0x6bdc], eax', complete)
repair=find(p,'dword ptr [rsp + 0x30], 1',keep,'cmp')
live_sub=[i for i,o in enumerate(p[repair:repair+12],repair) if o.mnemonic=='subss' and 'rip' in o.op_str];assert len(live_sub)==2
for i,value in zip(live_sub,(836.16,1035.4)):near(p[i],value)
relocate=next(i for i in sites(p,'?Relocate@Position@@') if live_sub[-1]<i<live_sub[-1]+20)
assert [(o.mnemonic,o.op_str) for o in p[relocate-6:relocate]]==[
 ('movss','xmm0, dword ptr [r14]'),('movss','dword ptr [rsp + 0x20], xmm0'),
 ('movss','xmm3, dword ptr [r12]'),('movss','xmm2, dword ptr [r13]'),('movss','xmm1, dword ptr [rdi]'),('lea','rcx, [rsi + 0xd0]')]
for body in (h, p):
    for forbidden in ('?TeleportTo@', '?KilledMonsterCredit@', '?AddItem@', '?CompleteQuest@', '?UpdateAchievementCriteria@', '?SetQuestStatus@'):
        assert not sites(body, forbidden), forbidden
print('PASS linked Sharasdal legacy receipt:exact1600/7/1066/old tuple,persisted identity,actual1229/difficulty/flags/207683/map/known coordinates/finite1cm bound,XYZ/O-only writes,refused reset,completed/live-safe controls and bad-XY-only recovery;complete retained Tuure/acquisition/return chain. Client traversal separate.')
