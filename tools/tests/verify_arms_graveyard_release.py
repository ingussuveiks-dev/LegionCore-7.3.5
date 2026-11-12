from linked_player_abi import inherited_operand
"""Bind selected Arms cemetery checkpoints to the registered shared1539 instance."""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p=runpy.run_path(str(Path(__file__).with_name('verify_slaghammer_handoff_release.py')))
n,ins=p['native'],p['ins'];prefix='?GetClosestGraveYard@instance_silver_hand_InstanceMapScript@instance_silver_hand@@';body=ins(prefix)
def sites(name):
 targets={n['resolve'](v) for k,v in n['symbols'].items() if k.startswith(name)}
 return [i for i,o in enumerate(body) if o.mnemonic in ('call','jmp') and o.operands[0].type==CS_OP_IMM and n['resolve'](o.operands[0].imm) in targets]
def at(text,mnemonic=None):return next(i for i,o in enumerate(body) if o.op_str==text and (mnemonic is None or o.mnemonic==mnemonic))
assert sites('?GetScenario@ScenarioMgr@@'),'Release lacks Arms checkpoint cemetery recovery'
factory=ins('?GetInstanceScript@instance_silver_hand@@');tables=[]
for i,o in enumerate(factory[:-1]):
 if o.mnemonic=='lea' and len(o.operands)==2 and o.operands[1].type==CS_OP_MEM and o.operands[1].mem.base==X86_REG_RIP and factory[i+1].op_str=='qword ptr [rsi], rax':
  table=struct.unpack_from('<96Q',n['binary'],n['offset'](o.address+o.size+o.operands[1].mem.disp))
  if n['symbol'](prefix) in table:tables.append(table)
table,=tables;assert table.count(n['symbol'](prefix))==1
slot=table.index(n['symbol'](prefix))*8
assert any(o.mnemonic=='call' and o.operands[0].type==CS_OP_MEM and o.operands[0].mem.disp==slot for o in ins('?GetClosestGraveYard@InstanceMap@@'))
scope=at('eax, 0x40d','cmp');fallback=body[scope+1].operands[0].imm;assert body[scope+1].mnemonic=='jne'
shadow_scope=next((i for i,o in enumerate(body) if o.mnemonic=='cmp' and o.op_str=='eax, 0x3df'),None)
if shadow_scope is not None:
 assert any(o.address==fallback for o in body[shadow_scope-4:shadow_scope]) and any(i in sites('?GetScenarioId@Scenario@@') for i in range(shadow_scope-4,shadow_scope))
 assert body[shadow_scope+1].mnemonic=='jne'
 fallback=body[shadow_scope+1].operands[0].imm
shadow_fallback=fallback
holy_scope=next((i for i,o in enumerate(body) if o.mnemonic=='cmp' and o.op_str=='eax, 0x444'),None)
if holy_scope is not None:
 assert any(o.address==fallback for o in body[holy_scope-4:holy_scope]) and any(i in sites('?GetScenarioId@Scenario@@') for i in range(holy_scope-4,holy_scope))
 assert body[holy_scope+1].mnemonic=='jne'
 fallback=body[holy_scope+1].operands[0].imm
assert any(o.mnemonic=='je' and o.operands[0].imm==fallback for o in body[sites('?GetScenario@ScenarioMgr@@')[0]+1:scope])
step=next(i for i in sites('?GetCurrentStep@Scenario@@') if scope<i<=scope+3);assert scope<step
assert [(o.mnemonic,o.op_str) for o in body[step+1:step+3]]==[('cmp','eax, 4'),('jae',hex(body[step+5].address))]
assert body[step+3].op_str=='edx, 0x14db' and body[step+4].mnemonic=='jmp' and body[step+4].operands[0].imm==body[step+9].address
assert [(o.mnemonic,o.op_str) for o in body[step+5:step+9]]==[('mov','edx, 0x1649'),('mov','ecx, 0x14d7'),('cmp','eax, 7'),('cmovb','edx, ecx')]
lookup=at('rdi, qword ptr [rax + rcx*8]','mov');identity=at('dword ptr [rdi], edx','cmp');failure=body[identity+1].operands[0].imm
assert body[identity+1].mnemonic=='jne' and body[lookup+2].mnemonic=='je' and body[lookup+2].operands[0].imm==failure
assert body[lookup-3].mnemonic=='jae' and body[lookup-3].operands[0].imm==failure and body[lookup-2].op_str=='ecx, edx'
mapcheck=at('word ptr [rdi + 0x1c], si','cmp');assert body[mapcheck-1].op_str=='esi, 0x603' and body[mapcheck+1].operands[0].imm==failure
coordinates=sites('?IsValidMapCoord@MapManager@@SA_NIMMMM@Z');assert len(coordinates)==1+int(shadow_scope is not None)+int(holy_scope is not None);coord=coordinates[0];assert step<lookup<identity<mapcheck<coord
assert any(o.mnemonic=='je' and o.operands[0].imm==failure for o in body[mapcheck+2:coord]) and body[coord+2].operands[0].imm==failure
for register,expected in [('xmm6',3.141592653589793),('xmm7',180.0)]:
 o=next(o for o in body[mapcheck:coord] if o.mnemonic=='movss' and o.op_str.startswith(register+',') and 'rip' in o.op_str);a=next(a for a in o.operands if a.type==CS_OP_MEM and a.mem.base==X86_REG_RIP)
 value,=struct.unpack_from('<f',n['binary'],n['offset'](o.address+o.size+a.mem.disp));assert abs(value-expected)<1e-5
expected=[('lea','rbx, [r14 + 0x328]'),('movss','xmm0, dword ptr [rdi + 0x18]'),('mulss','xmm0, xmm6'),('divss','xmm0, xmm7'),('movss','xmm3, dword ptr [rdi + 0x14]'),('movss','xmm2, dword ptr [rdi + 0x10]'),('movss','xmm1, dword ptr [rdi + 0xc]'),('mov','dword ptr [rbx + 0x10], esi')]
assert [(o.mnemonic,o.op_str) for o in body[coord+3:coord+11]]==expected
jump=body[coord+11];assert jump.mnemonic=='jmp';stores=next(i for i,o in enumerate(body) if o.address==jump.operands[0].imm)
assert [(o.mnemonic,o.op_str) for o in body[stores:stores+3]]==[('movss','dword ptr [rbx], xmm1'),('movss','dword ptr [rbx + 4], xmm2'),('movss','dword ptr [rbx + 8], xmm3')]
assert body[stores+3].mnemonic=='call' and n['resolve'](body[stores+3].operands[0].imm)==n['symbol']('?NormalizeOrientation@Position@@') and body[stores+4].op_str=='dword ptr [rbx + 0xc], xmm0' and body[stores+5].op_str=='rax, rbx'
assert stores>0 and body[stores-1].op_str=='xmm0, xmm0' # General5281 retains zero facing; Arms jumps past it.
fall=next(i for i,o in enumerate(body) if o.address==fallback);assert body[fall].op_str=='rbx, [r14 + 0x328]' and body[fall+1].op_str=='dword ptr [rbx + 0x10], 0x603'
assert any(o.mnemonic=='mov' and o.op_str==inherited_operand('rcx, qword ptr [rax + 0xa508]') for o in body[fall:stores]) # Native5281 remains separate.
assert b'Stromkar graveyard: invalid native WorldSafeLocs %u for instance %u' in n['binary']
refused=at('eax, eax','xor');assert body[refused+1].mnemonic=='jmp' and body[refused+1].operands[0].imm>body[stores+5].address
for forbidden in ('?TeleportTo@Player@@','?ResurrectPlayer@Player@@','?SetHealth@Unit@@','?AddItem@Player@@','?CompleteQuest@Player@@','?KilledMonsterCredit@Player@@','?UpdateAchievementCriteria@Player@@','?SetCurrentStep@Scenario@@'):
 assert not sites(forbidden),forbidden
print('PASS linked Arms cemetery:actual registered primary InstanceScript/virtual dispatch,1037-only5339 before4/5335 at4..6/5705 from7,native indexed identity/map/zero/finite guards,radian XYZ/map/facing stores and null diagnostics,retained separate5281 fallback,Slaghammer owner/receipt/lifecycle and progression exclusions. Client traversal separate.')
