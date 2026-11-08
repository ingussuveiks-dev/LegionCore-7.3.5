"""Bind native four-checkpoint recovery to the registered1545 instance."""
from pathlib import Path
import runpy,struct,re
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p=runpy.run_path(str(Path(__file__).with_name('verify_dreadblades_pickup_release.py')))
n,ins=p['n'],p['instructions'];prefix='?GetClosestGraveYard@instance_dreadblades@@';body=ins(prefix)
def sites(name):
 targets={n['resolve'](v) for k,v in n['symbols'].items() if k.startswith(name)}
 return [i for i,o in enumerate(body) if o.mnemonic in ('call','jmp') and o.operands[0].type==CS_OP_IMM and n['resolve'](o.operands[0].imm) in targets]
def rip(o):
 a=next(a for a in o.operands if a.type==CS_OP_MEM and a.mem.base==X86_REG_RIP)
 return o.address+o.size+a.mem.disp
def find(text,mnemonic=None):return next(i for i,o in enumerate(body) if o.op_str==text and (mnemonic is None or o.mnemonic==mnemonic))
assert sites('?GetScenario@ScenarioMgr@@'),'Release lacks Dreadblades four-checkpoint recovery'
factoryName='?GetInstanceScript@?$GenericInstanceMapScript@Uinstance_dreadblades@@@@';factory=ins(factoryName);tables=[]
for i,o in enumerate(factory[:-1]):
 if o.mnemonic=='lea' and o.op_str.startswith('rax, [rip') and factory[i+1].op_str=='qword ptr [rdi], rax':
  table=struct.unpack_from('<96Q',n['binary'],n['offset'](rip(o)))
  if n['symbol'](prefix) in table:tables.append(table)
table,=tables;assert table.count(n['symbol'](prefix))==1
slot=table.index(n['symbol'](prefix))*8
assert any(o.mnemonic=='call' and o.operands[0].type==CS_OP_MEM and o.operands[0].mem.disp==slot for o in ins('?GetClosestGraveYard@InstanceMap@@'))
descriptorName='??_7?$GenericInstanceMapScript@Uinstance_dreadblades@@@@6BScriptObject@@@'
matches=re.findall(r'^\s+\w+:\w+\s+'+re.escape(descriptorName)+r'\s+([0-9a-fA-F]{16})\s', (n['args'].release/'worldserver.map').read_text(errors='replace'),re.M)
assert len(matches)==1;descriptor=int(matches[0],16)
assert n['symbol'](factoryName) in struct.unpack_from('<8Q',n['binary'],n['offset'](descriptor))
registration=ins('?AddSC_instance_class_art_scenarios@@')
assert descriptor in [rip(o) for o in registration if o.mnemonic=='lea' and any(a.type==CS_OP_MEM and a.mem.base==X86_REG_RIP for a in o.operands)]
assert 1545 in {a.imm for o in registration for a in o.operands if a.type==CS_OP_IMM}
scope=find('eax, 0x3f4','cmp');null=body[scope+1].operands[0].imm
get,=sites('?GetScenario@ScenarioMgr@@');assert body[get+2].op_str=='rax, rax' and body[get+3].mnemonic=='je' and body[get+3].operands[0].imm==null
assert body[scope+1].mnemonic=='jne' and any(o.address==null and o.op_str=='eax, eax' and o.mnemonic=='xor' for o in body)
step,=sites('?GetCurrentStep@Scenario@@');select=body[step+1:step+13]
assert [(o.mnemonic,o.op_str) for o in select]==[
 ('cmp','eax, 4'),('jb',hex(body[step+5].address)),('mov','edx, 0x14f1'),('jmp',hex(body[step+13].address)),
 ('cmp','eax, 2'),('jb',hex(body[step+9].address)),('mov','edx, 0x14f0'),('jmp',hex(body[step+13].address)),
 ('mov','edx, 0x14b3'),('mov','ecx, 0x14ef'),('cmp','eax, 1'),('cmove','edx, ecx')]
lookup=find('rdi, qword ptr [rax + rcx*8]','mov');identity=lookup+3;failure=body[identity+1].operands[0].imm
assert body[lookup-3].mnemonic=='jae' and body[lookup-3].operands[0].imm==failure
assert body[lookup-2].op_str=='ecx, edx' and body[lookup+1].op_str=='rdi, rdi'
assert body[lookup+2].mnemonic=='je' and body[lookup+2].operands[0].imm==failure
assert body[identity].op_str=='dword ptr [rdi], edx' and body[identity].mnemonic=='cmp' and body[identity+1].mnemonic=='jne'
mapcheck=identity+3;assert body[mapcheck-1].op_str=='r14d, 0x609' and body[mapcheck].op_str=='word ptr [rdi + 0x1c], r14w' and body[mapcheck+1].operands[0].imm==failure
coord,=sites('?IsValidMapCoord@MapManager@@SA_NIMMMM');assert mapcheck<coord
assert body[coord+1].op_str=='al, al' and body[coord+2].mnemonic=='je' and body[coord+2].operands[0].imm==failure
assert any(o.mnemonic=='je' and o.operands[0].imm==failure for o in body[mapcheck+2:coord])
for mnemonic,expected in [('mulss',3.141592653589793),('divss',180.)]:
 ops=[o for o in body[mapcheck:] if o.mnemonic==mnemonic and 'rip' in o.op_str];assert len(ops)==2
 for o in ops:assert abs(struct.unpack_from('<f',n['binary'],n['offset'](rip(o)))[0]-expected)<1e-5
assert body[coord+3].op_str=='rbx, [rsi + 0x2e0]'
stores=['dword ptr [rbx + 0x10], r14d','dword ptr [rbx], xmm1','dword ptr [rbx + 4], xmm2','dword ptr [rbx + 8], xmm3','dword ptr [rbx + 0xc], xmm0']
indices=[find(s) for s in stores];assert indices==sorted(indices) and all(i>coord for i in indices)
normalize,=sites('?NormalizeOrientation@Position@@');assert indices[-2]<normalize<indices[-1]
assert body[indices[-1]+1].op_str=='rax, rbx' and body[indices[-1]+2].mnemonic=='jmp'
assert b'Dreadblades graveyard: invalid native WorldSafeLocs %u for instance %u' in n['binary']
assert any(o.mnemonic=='je' and o.operands[0].imm==null for o in body if o.address>=failure)
calls=n['calls'](prefix)
for forbidden in ('?TeleportTo@Player@@','?ResurrectPlayer@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?KilledMonsterCredit@Player@@','?UpdateAchievementCriteria@Player@@','?SetCurrentStep@Scenario@@'):
 assert not any(k.startswith(forbidden) and n['resolve'](v) in calls for k,v in n['symbols'].items()),forbidden
print('PASS linked Dreadblades cemetery:registered1545 descriptor/factory/primary vtable/native virtual dispatch;actual1012 guard,5299 at0/5359 at1/5360 at2..3/5361 from4 including completed5,indexed identity/map/zero/finite/radians,stable XYZ/map/facing return/contextual null refusal. Retained Eliza/native loot pickup proof;client sequence separate.')
