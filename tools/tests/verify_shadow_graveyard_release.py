"""Verify actual991 interior-only recovery and retained registered Arms/general paths."""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p=runpy.run_path(str(Path(__file__).with_name('verify_arms_graveyard_release.py')))
n,body,sites,at=(p[k] for k in ('n','body','sites','at'));scope=p['shadow_scope'];fallback=p['shadow_fallback']
assert scope is not None,'Release lacks Shadow interior cemetery recovery'
first,second=[i for i in sites('?GetCurrentStep@Scenario@@') if scope<i<(p['holy_scope'] or len(body))]
assert [(o.mnemonic,o.op_str) for o in body[first+1:first+3]]==[('cmp','eax, 2'),('jb',hex(fallback))],'Shadow stages0/1 must retain the general fallback'
assert [(o.mnemonic,o.op_str) for o in body[second+1:second+5]]==[('mov','edx, 0x14e7'),('mov','ecx, 0x14d7'),('cmp','eax, 7'),('cmovb','edx, ecx')]
lookup=next(i for i,o in enumerate(body) if i>second and o.mnemonic=='mov' and o.op_str=='rdi, qword ptr [rax + rdx*8]')
identity=next(i for i,o in enumerate(body) if i>lookup and o.mnemonic=='cmp' and o.op_str=='dword ptr [rdi], edx');failure=body[identity+1].operands[0].imm
assert body[identity+1].mnemonic=='jne' and body[lookup+2].mnemonic=='je' and body[lookup+2].operands[0].imm==failure and body[lookup-2].mnemonic=='jae' and body[lookup-2].operands[0].imm==failure
mapcheck=next(i for i,o in enumerate(body) if i>identity and o.mnemonic=='cmp' and o.op_str=='word ptr [rdi + 0x1c], si')
assert body[mapcheck-1].op_str=='esi, 0x603' and body[mapcheck+1].operands[0].imm==failure
coord=p['coordinates'][1];assert scope<first<second<lookup<identity<mapcheck<coord
assert any(o.mnemonic=='je' and o.operands[0].imm==failure for o in body[mapcheck+2:coord])
for register,expected in [('xmm6',3.141592653589793),('xmm7',180.0)]:
 o=next(o for o in body[mapcheck:coord] if o.mnemonic=='movss' and o.op_str.startswith(register+',') and 'rip' in o.op_str);a=next(a for a in o.operands if a.type==CS_OP_MEM and a.mem.base==X86_REG_RIP);value,=struct.unpack_from('<f',n['binary'],n['offset'](o.address+o.size+a.mem.disp));assert abs(value-expected)<1e-5
assert body[coord+2].mnemonic=='jne' and body[coord+2].operands[0].imm==body[p['coord']+3].address,'Validated Shadow source must reach proven native XYZ/map/facing stores'
assert body[coord+3].address==failure
end=next(i for i,o in enumerate(body) if o.address==fallback);refused=body[p['refused']].address
assert any(o.mnemonic in ('je','jmp') and o.operands[0].imm==refused for o in body[coord+3:end])
assert b'Shadow graveyard: invalid native WorldSafeLocs %u for instance %u' in n['binary']
print('PASS linked Shadow cemetery:registered991 branch,stages0/1 retain legacy5281,2..6 choose5335/from7 choose5351,indexed source/ID/map/zero/finite guards,native radians and checked shared XYZ/facing stores,contextual diagnostic/null failure;Arms,other/general and owner/receipt/lifecycle/progression exclusions retained. Full client separate.')
