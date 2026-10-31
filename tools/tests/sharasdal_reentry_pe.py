"""Prove unchanged re-entry zero stores across MSVC register allocations."""
from capstone import CS_OP_MEM, CS_OP_REG

def verify_zero_rearm(body,flag,counters):
 candidates=[]
 for whole,dword,word,byte in [('r14','r14d','r14w','r14b'),('rbp','ebp','bp','bpl'),('r12','r12d','r12w','r12b')]:
  aliases={whole,dword,word,byte}
  def writes(part):return any({y.reg_name(r) for r in y.regs_access()[1]}&aliases for y in part)
  for i,x in enumerate(body):
   if x.mnemonic!='mov' or x.operands[0].type!=CS_OP_MEM or x.operands[0].mem.disp!=flag or x.operands[1].type!=CS_OP_REG or x.reg_name(x.operands[1].reg)!=byte:continue
   receiver=x.reg_name(x.operands[0].mem.base)
   zeros=[j for j,y in enumerate(body[:i]) if y.mnemonic=='xor' and y.op_str==f'{dword}, {dword}']
   assert zeros and not writes(body[zeros[-1]+1:i])
   stores=[j for j,y in enumerate(body[i+1:],i+1) if y.mnemonic=='mov' and y.op_str==f'qword ptr [{receiver} + {counters:#x}], {whole}']
   at,=stores
   assert not writes(body[i+1:at])
   candidates.append(i)
 assert len(candidates)==1
