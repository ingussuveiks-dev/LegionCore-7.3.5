"""Bind shared-map Warrior entrance policy to the actual linked Release body."""
import runpy
from pathlib import Path
from capstone import CS_OP_IMM

p = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n, ins = p['native'], p['instructions']
body = ins('?RecordScenarioEntrance@Player@@')

def site(prefix):
    targets = {n['resolve'](address) for name,address in n['symbols'].items() if name.startswith(prefix)}
    return [i for i,o in enumerate(body) if o.mnemonic in ('call','jmp') and o.operands[0].type == CS_OP_IMM and n['resolve'](o.operands[0].imm) in targets]

def find(text, start=0, mnemonic=None):
    return next(i for i,o in enumerate(body[start:],start) if o.op_str == text and (mnemonic is None or o.mnemonic == mnemonic))

route, = site('?ResumeDungeon@')
lookup, = site('?GetSpellTargetPosition@SpellMgr@@')
coordinates = site('?IsValidMapCoord@MapManager@@')
assert len(coordinates) == 3 and route < lookup < coordinates[0] < coordinates[1] < coordinates[2]
assert body[route+2].op_str == 'rax, rax' and body[route+3].mnemonic == 'je'
exit_address = body[route+3].operands[0].imm
cls = find('al, 1',route,'cmp')
assert route < cls < lookup
assert find('r12d, r12d',0,'xor') < route and route < find('ebx, r12d',route,'mov') < cls
assert body[cls-1].mnemonic == 'call' and body[cls-4].op_str == 'r8b, 1'
assert find('edx, 0x35',route) < cls
foreign = body[cls+1].operands[0].imm
assert body[cls+1].mnemonic == 'jne'
m1,m2 = find('dword ptr [rsi], 0x5e7',cls),find('dword ptr [rsi], 0x603',cls)
s1,s2 = find('eax, 0x3b0',m1),find('eax, 0x40d',m2)
assert cls < m1 < s1 < m2 < s2 < lookup
assert body[m1+1].mnemonic == 'jne' and body[m1+1].operands[0].imm == body[m2].address
assert body[s1+1].op_str == 'ecx, 0x2f337' and body[s1+2].mnemonic == 'cmove' and body[s1+2].op_str == 'ebx, ecx'
assert body[s2+2].op_str == 'ebx, 0x31c90'
assert body[lookup-2].op_str == 'edx, ebx'
null = find('rax, rax',lookup,'test')
mapping = find('dword ptr [rax], ecx',null,'cmp')
zero = find('xmm0, xmm1',mapping,'ucomiss')
zero = find('xmm0, xmm1',zero+1,'ucomiss')
for i in (null,mapping):
    assert body[i+1].mnemonic in ('je','jne') and body[i+1].operands[0].imm == exit_address
assert body[zero+2].mnemonic == 'je' and body[zero+2].operands[0].imm == exit_address
assert body[coordinates[0]+1].op_str == 'al, al' and body[coordinates[0]+2].mnemonic == 'je' and body[coordinates[0]+2].operands[0].imm == exit_address
assert foreign > body[coordinates[0]].address and foreign < body[coordinates[1]].address
assert find('xmm6, dword ptr [rbx + 4]',coordinates[0]) < coordinates[1]
assert find('xmm9, dword ptr [rbx + 8]',coordinates[0]) < coordinates[1]
assert find('xmm8, dword ptr [rbx + 0xc]',coordinates[0]) < coordinates[1]
assert find('xmm0, dword ptr [rbx + 0x10]',coordinates[0]) < coordinates[1]
assert find('xmm6, dword ptr [rbp + 0x10]',coordinates[1]) < coordinates[2]
write = find('dword ptr [rdi + 0x6bb0], eax',coordinates[2],'mov')
assert coordinates[2] < write
assert body[coordinates[2]+2].operands[0].imm == exit_address
for text in ('dword ptr [rdi + 0x6bc8], xmm6','dword ptr [rdi + 0x6bcc], xmm9','dword ptr [rdi + 0x6bd0], xmm8','dword ptr [rdi + 0x6bd4], xmm7'):
    assert find(text,write) > write
same = find('dword ptr [rdi + 0x6bb4], eax',0,'cmp')
metadata = find('eax, dword ptr [rdi + 0x3b8]',write,'mov')
assert body[same+1].mnemonic == 'je' and body[same+1].operands[0].imm == body[metadata].address
assert find('dword ptr [rdi + 0x6bb0], eax',0,'cmp') < same < route
assert find('qword ptr [rdi + 0x30d8], r12',0,'cmp') < route
for forbidden in ('?TeleportTo@Player@@','?NearTeleportTo@','?CastSpell@Unit@@','?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@'):
    assert not site(forbidden), 'Entrance recorder gained movement/progression: '+forbidden
print('PASS selected Release RecordScenarioEntrance: admitted native route then actual Warrior byte/map1511+944/193335 or1539+1037/203920;native source null/map/zero/coordinate failures exit before foreign LFG or checkpoint writes;native/ordinary coordinate snapshots reach validated checkpoint storage;existing map+scenario checkpoint skips position writes. No teleport/cast/item/credit operation. Source64/native56 and actual client remain separate.')
