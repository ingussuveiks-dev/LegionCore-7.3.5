"""Follow opted-in threshold retry from the actual action11 switch and loader."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG

p=runpy.run_path(str(Path(__file__).with_name('verify_lights_wrath_takeover_release.py')))
n,ins=p['n'],p['ins']
action=ins('?ProcessAction@SmartScript@@')
maximum=next(i for i,x in enumerate(action) if x.mnemonic=='cmp' and any(o.type==CS_OP_IMM and o.imm==223 for o in x.operands))
assert action[maximum-1].mnemonic=='dec'
tables=[o.mem for x in action[maximum+1:maximum+10] for o in x.operands if o.type==CS_OP_MEM and o.mem.index and o.mem.disp>0]
assert len(tables)==2 and tables[0].scale==1 and tables[1].scale==4
def case(number):
 index=n['binary'][n['offset'](n['base']+tables[0].disp+number-1)]
 return n['base']+struct.unpack_from('<I',n['binary'],n['offset'](n['base']+tables[1].disp+index*4))[0]
first=case(11);end=min(a for a in {case(i) for i in range(1,225)} if a>first)
b=[x for x in action if first<=x.address<end]
# Compiler lowers flags256 into loaded field5c ->shift8 ->mask1, keeping
# the flag separately from each native cast result.
flags=[i for i,x in enumerate(b[:-2]) if x.mnemonic=='mov' and len(x.operands)==2 and x.operands[1].type==CS_OP_MEM and x.operands[1].mem.disp==0x5c and
       b[i+1].mnemonic=='shr' and b[i+1].operands[1].type==CS_OP_IMM and b[i+1].operands[1].imm==8 and b[i+2].mnemonic=='and' and b[i+2].operands[1].imm==1]
assert len(flags)==1
assert any(x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and
           n['resolve'](x.operands[0].imm)==n['symbol']('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N') for x in b)
resets=[i for i,x in enumerate(b) if x.mnemonic=='mov' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0x99 and x.operands[0].size==1]
assert len(resets)==2, 'Missing-target and rejected/no accepted cast must both retain the threshold'
for i in resets:
 window=b[i:i+12]
 assert any(x.mnemonic=='mov' and x.operands[1].type==CS_OP_IMM and x.operands[1].imm==500 for x in window)
 assert any(x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==n['symbol']('?RecalcTimer@SmartScript@@') for x in window)
 assert any(x.mnemonic=='jmp' for x in window), 'Failure must return before LINK'
assert any(x.mnemonic=='cmp' and x.operands[1].type==CS_OP_IMM and x.operands[1].imm==123 for x in b), 'Legacy SPELL_IN_PROGRESS policy absent'
# Native success records the accepted-cast boolean only from result==OK;
# the separately opted-in failure path excludes the old in-progress exception.
success=[i for i,x in enumerate(b[:-2]) if x.mnemonic=='test' and x.op_str=='eax, eax' and b[i+1].mnemonic=='je']
assert success
loader=ins('?IsEventValid@SmartAIMgr@@')
sites=[i for i,x in enumerate(loader) if x.mnemonic=='bt' and x.operands[1].type==CS_OP_IMM and x.operands[1].imm==8 and i>0 and
       loader[i-1].mnemonic=='mov' and loader[i-1].operands[1].type==CS_OP_MEM and loader[i-1].operands[1].mem.disp==0x5c]
assert len(sites)==1
i=sites[0];window=loader[i:i+18]
for displacement,value in ((0x30,2),(8,0),(0x70,2),(0x10,0)):
 assert any(x.mnemonic=='cmp' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==displacement and
            x.operands[1].type==CS_OP_IMM and x.operands[1].imm==value for x in window)
assert any(x.mnemonic=='test' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0x3c and x.operands[1].imm==1 for x in window)
assert {11,256}<={o.imm for x in window for o in x.operands if o.type==CS_OP_IMM}
assert any(x.mnemonic=='cmp' and x.operands[1].type==CS_OP_IMM and x.operands[1].imm==75 for x in loader[i-4:i]), 'Ordinary action75 aura argument must bypass cast-flag validation'
for caller,callee in (('?UpdateTimer@SmartScript@@','?ProcessEvent@SmartScript@@'),('?OnReset@SmartScript@@','?InitTimer@SmartScript@@')):
 assert n['symbol'](callee) in n['calls'](caller)
print('PASS actual action11 switch scopes flags256, ordinary cast result and both500ms/runOnce retry branches; native loader health/victim/one-shot/source/no-LINK/exclusive flag admission, action75 aura isolation and timer/Reset dispatch. Native host supplies actual tick boundaries and counterfactual; real combat/client pending.')
