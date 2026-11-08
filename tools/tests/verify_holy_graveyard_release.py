"""Bind Holy1092 bidirectional cemetery selection to the real registered instance."""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p=runpy.run_path(str(Path(__file__).with_name('verify_shadow_graveyard_release.py')))
q=p['p'];n,body,sites=(p[k] for k in ('n','body','sites'));scope=q['holy_scope'];fallback=q['fallback']
assert scope is not None,'Release lacks Holy cemetery recovery'
step,=[i for i in sites('?GetCurrentStep@Scenario@@') if i>scope]
assert [(o.mnemonic,o.op_str) for o in body[step+1:step+11]]==[
 ('xor','ebx, ebx'),('cmp','eax, 2'),('jae',hex(body[step+6].address)),
 ('mov','edx, 0x1570'),('jmp',hex(body[step+11].address)),
 ('add','eax, -5'),('mov','edx, ebx'),('cmp','eax, 1'),('setbe','dl'),('add','edx, 0x163d')]
lookup=next(i for i,o in enumerate(body) if i>step and o.mnemonic=='mov' and o.op_str=='rdi, qword ptr [rax + rcx*8]')
identity=lookup+3;failure=body[identity+1].operands[0].imm
assert body[lookup-3].mnemonic=='jae' and body[lookup-3].operands[0].imm==failure
assert body[lookup-2].op_str=='ecx, edx' and body[lookup+1].op_str=='rdi, rdi'
assert body[lookup+2].mnemonic=='je' and body[lookup+2].operands[0].imm==failure
assert body[identity].mnemonic=='cmp' and body[identity].op_str=='dword ptr [rdi], edx' and body[identity+1].mnemonic=='jne'
mapcheck=identity+3;assert body[mapcheck-1].op_str=='esi, 0x603' and body[mapcheck].op_str=='word ptr [rdi + 0x1c], si' and body[mapcheck+1].operands[0].imm==failure
coord=q['coordinates'][2];assert scope<step<lookup<identity<mapcheck<coord
assert any(o.mnemonic=='je' and o.operands[0].imm==failure for o in body[mapcheck+2:coord])
for register,expected in [('xmm6',3.141592653589793),('xmm7',180.0)]:
 o=next(o for o in body[mapcheck:coord] if o.mnemonic=='movss' and o.op_str.startswith(register+',') and 'rip' in o.op_str);a=next(a for a in o.operands if a.type==CS_OP_MEM and a.mem.base==X86_REG_RIP);value,=struct.unpack_from('<f',n['binary'],n['offset'](o.address+o.size+a.mem.disp));assert abs(value-expected)<1e-5
assert body[coord+2].mnemonic=='jne' and body[coord+2].operands[0].imm==body[q['coord']+3].address
assert body[coord+3].address==failure
end=next(i for i,o in enumerate(body) if o.address==fallback)
assert any(o.mnemonic=='jmp' and o.operands[0].imm==body[q['refused']].address for o in body[coord+3:end])
assert b'Silver Hand graveyard: invalid native WorldSafeLocs %u for instance %u' in n['binary']
print('PASS linked Holy cemetery:real registered1092 dispatch,5488 before2,5694 only5..6,5693 at2..4/from7;native indexed identity/map/zero/finite/radian guards,checked shared XYZ/facing stores,contextual null failure;retained Arms/Shadow/general and Slaghammer lifecycle/progression exclusions. Client traversal separate.')
