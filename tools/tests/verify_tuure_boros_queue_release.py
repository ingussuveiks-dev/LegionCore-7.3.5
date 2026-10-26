"""Follow Boros's actual AI-owned queue and all four reset-generation guards."""
from pathlib import Path
import runpy
from capstone import CS_OP_MEM,CS_OP_REG,CS_OP_IMM
from capstone.x86_const import X86_REG_RCX

p=runpy.run_path(str(Path(__file__).with_name('verify_tuure_intro_heal_release.py')))
n,instructions,values=p['n'],p['instructions'],p['values']
scope='npc_vindicator_borosAI@npc_vindicator_boros@@'
reset,life=('?'+method+'@'+scope for method in ('Reset','UpdateLifecycle'))
increments=[x for x in instructions(reset) if x.mnemonic=='inc' and x.operands[0].type==CS_OP_MEM and x.operands[0].size==8]
assert len(increments)==1
generation=increments[0].operands[0].mem.disp
body=instructions(life)
assert n['symbol']('?Update@FunctionProcessor@@') in n['calls'](life)
queues=[x.operands[1].mem.disp for x in body if x.mnemonic=='lea' and x.operands[0].type==CS_OP_REG and x.operands[0].reg==X86_REG_RCX and x.operands[1].type==CS_OP_MEM]
assert len(queues)==1 and queues[0]>0x20 # separate AI field, not me/Unit queue
queue=queues[0]
callbacks=[name for name in n['symbols'] if name.startswith('?_Do_call@') and scope in name and 'HealReceived@' not in name]
guarded=[]
for name in callbacks:
    b=instructions(name)
    if any(op.type==CS_OP_MEM and op.mem.disp==generation for x in b for op in x.operands) and any(x.mnemonic=='cmp' and i+1<len(b) and b[i+1].mnemonic=='jne' and any(op.type==CS_OP_MEM and op.mem.disp==0x10 for op in x.operands) for i,x in enumerate(b)):
        guarded.append(name)
assert len(guarded)==3 and sum('MovementInform@' in name for name in guarded)==2
assert sum('MoveInLineOfSight@' in name for name in guarded)==1
outer=[name for name in callbacks if 'EnterEvadeMode@' in name and name not in guarded]
assert not outer
scheduled=['?MovementInform@'+scope,'?MoveInLineOfSight@'+scope]
references=sum(len(x.operands)>1 and x.operands[0].type==CS_OP_REG and x.operands[0].reg==X86_REG_RCX and ((x.mnemonic=='lea' and x.operands[1].type==CS_OP_MEM and x.operands[1].mem.disp==queue) or (x.mnemonic=='add' and x.operands[1].type==CS_OP_IMM and x.operands[1].imm==queue)) for method in scheduled for x in instructions(method))
assert references==3, 'Not all three local operations bind the selected AI queue'
assert {2000,3000}<=values(scheduled[0]) and 4000 in values(scheduled[1])
print('PASS actual selected Boros lifecycle updates separate AI-owned FunctionProcessor; Reset generation protects all compiled2s/3s/4s callbacks on that queue. Old evade16s lambda absent; actual-death1s/16s belongs to the separately regressed instance. Native host owns lifetime tests; client pending.')
