"""Verify registered1600 cemetery plus the complete retained lower-ascent chain."""
from pathlib import Path
import runpy,struct,re
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_lower_ascent_release.py')))
n=p['n'];ins=n['instructions'];prefix='?GetClosestGraveYard@instance_sharasdal_InstanceMapScript@instance_sharasdal@@';body=ins(prefix)
def sites(name):
 targets={n['resolve'](v) for k,v in n['symbols'].items() if k.startswith(name)}
 return [i for i,o in enumerate(body) if o.mnemonic in ('call','jmp') and o.operands[0].type==CS_OP_IMM and n['resolve'](o.operands[0].imm) in targets]
def at(text,mnemonic=None):return next(i for i,o in enumerate(body) if o.op_str==text and (mnemonic is None or o.mnemonic==mnemonic))
assert sites('?GetScenario@ScenarioMgr@@'),'Release lacks Sharasdal cemetery recovery'
pattern=re.compile(r'\s+\w+:\w+\s+\?\?_7instance_sharasdal_InstanceMapScript@instance_sharasdal@@6B@\s+([0-9a-fA-F]{16})')
address,=[int(m[1],16) for line in (n['args'].release/'worldserver.map').read_text(errors='replace').splitlines() if (m:=pattern.match(line))]
factory=ins('?GetInstanceScript@instance_sharasdal@@');ctor='??0instance_sharasdal_InstanceMapScript@instance_sharasdal@@'
call,=[i for i,o in enumerate(factory) if o.mnemonic=='call' and o.operands[0].type==CS_OP_IMM and n['resolve'](o.operands[0].imm)==n['symbol'](ctor)]
assert factory[call-1].op_str=='rcx, rax' and factory[call-2].op_str=='rdx, rbx'
constructor=ins(ctor)
install,=[i for i,o in enumerate(constructor) if o.mnemonic=='lea' and any(a.type==CS_OP_MEM and a.mem.base==X86_REG_RIP and o.address+o.size+a.mem.disp==address for a in o.operands)]
assert constructor[install+1].op_str=='qword ptr [rbx], rax'
table=struct.unpack_from('<96Q',n['binary'],n['offset'](address));assert table.count(n['symbol'](prefix))==1;slot=table.index(n['symbol'](prefix))*8
assert any(o.mnemonic=='call' and o.operands[0].type==CS_OP_MEM and o.operands[0].mem.disp==slot for o in ins('?GetClosestGraveYard@InstanceMap@@'))
scope=at('eax, 0x42a','cmp');refused=body[scope+1].operands[0].imm;assert body[scope+1].mnemonic=='jne'
assert any(o.mnemonic=='je' and o.operands[0].imm==refused for o in body[sites('?GetScenario@ScenarioMgr@@')[0]+1:scope])
step,=sites('?GetCurrentStep@Scenario@@');assert scope<step
selection=at('edx, 0x1543','mov')
assert [(o.mnemonic,o.op_str) for o in body[step+1:step+6]]==[('xor','ecx, ecx'),('cmp','eax, 4'),('setae','cl'),('cmp','eax, 3'),('jne',hex(body[selection].address))]
assert [(o.mnemonic,o.op_str) for o in body[selection:selection+4]]==[('mov','edx, 0x1543'),('mov','eax, 0x1561'),('test','ecx, ecx'),('cmovne','edx, eax')]
mgr,=sites('?GetAchievementMgr@Scenario@@');progress,=sites('?GetCriteriaProgressMap@');assert step<mgr<progress<selection
hash_value=14695981039346656037
for byte in struct.pack('<I',47131):hash_value=((hash_value^byte)*1099511628211)&((1<<64)-1)
assert body[progress+2].op_str==f'rcx, {hex(hash_value)}'
key=at('dword ptr [rcx + 0x10], 0xb81b','cmp');counter=at('qword ptr [rcx + 0x28], 0','cmp');assert progress<key<counter<selection
assert sum(o.mnemonic=='cmp' and o.op_str=='dword ptr [rcx + 0x10], 0xb81b' for o in body[progress:counter])==2
assert [(o.mnemonic,o.op_str) for o in body[counter-4:counter]]==[('test','rcx, rcx'),('je',hex(body[counter+4].address)),('cmp','rcx, r9'),('je',hex(body[counter+4].address))]
assert body[counter+1].mnemonic=='je' and body[counter+1].operands[0].imm==body[counter+4].address
assert [(o.mnemonic,o.op_str) for o in body[counter+2:counter+5]]==[('mov','ecx, 1'),('jmp',hex(body[selection].address)),('xor','ecx, ecx')]
lookup=at('rdi, qword ptr [rax + rdx*8]','mov');failure=body[lookup+4].operands[0].imm
assert body[lookup-2].mnemonic=='jae' and body[lookup-2].operands[0].imm==failure
assert body[lookup+1].op_str=='rdi, rdi' and body[lookup+2].mnemonic=='je' and body[lookup+2].operands[0].imm==failure
assert body[lookup+3].op_str=='dword ptr [rdi], edx' and body[lookup+4].mnemonic=='jne'
mapcheck=at('word ptr [rdi + 0x1c], r15w','cmp');assert body[mapcheck-1].op_str=='r15d, 0x640' and body[mapcheck+1].operands[0].imm==failure
coord,=sites('?IsValidMapCoord@MapManager@@SA_NIMMMM@Z');assert lookup<mapcheck<coord
assert any(o.mnemonic=='je' and o.operands[0].imm==failure for o in body[mapcheck+2:coord]) and body[coord+2].operands[0].imm==failure
for operation,value in [('mulss',3.141592653589793),('divss',180.0)]:
 for o in [o for o in body[mapcheck:coord+18] if o.mnemonic==operation and 'rip' in o.op_str]:
  a=next(a for a in o.operands if a.type==CS_OP_MEM and a.mem.base==X86_REG_RIP);actual,=struct.unpack_from('<f',n['binary'],n['offset'](o.address+o.size+a.mem.disp));assert abs(actual-value)<1e-5
expected=[('lea','rbx, [rsi + 0x2e0]'),('movss','xmm0, dword ptr [rdi + 0x18]')]
assert [(o.mnemonic,o.op_str) for o in body[coord+3:coord+5]]==expected
assert [(o.mnemonic,o.op_str) for o in body[coord+7:coord+14]]==[('movss','xmm3, dword ptr [rdi + 0x14]'),('movss','xmm2, dword ptr [rdi + 0x10]'),('movss','xmm1, dword ptr [rdi + 0xc]'),('mov','dword ptr [rbx + 0x10], r15d'),('movss','dword ptr [rbx], xmm1'),('movss','dword ptr [rbx + 4], xmm2'),('movss','dword ptr [rbx + 8], xmm3')]
assert coord+14 in sites('?NormalizeOrientation@Position@@') and body[coord+15].op_str=='dword ptr [rbx + 0xc], xmm0' and body[coord+16].op_str=='rax, rbx'
assert next(o for o in body if o.address==refused).op_str=='eax, eax'
assert b'Sharasdal graveyard: invalid native WorldSafeLocs %u for instance %u' in n['binary']
for forbidden in ('?TeleportTo@Player@@','?ResurrectPlayer@Player@@','?SetHealth@Unit@@','?AddItem@Player@@','?CompleteQuest@Player@@','?UpdateAchievementCriteria@Player@@','?SetCurrentStep@Scenario@@'):
 assert not sites(forbidden),forbidden
print('PASS linked Sharasdal cemetery:registered1600/1066 dispatch,5443 before actual3/47131 Counter,5473 only after that receipt/from4,indexed source/ID/map/zero/finite guards,native radians and stable2e0 XYZ/map/facing stores/null diagnostic,no grants;complete lower-ascent/shared chain retained. Client traversal separate.')
