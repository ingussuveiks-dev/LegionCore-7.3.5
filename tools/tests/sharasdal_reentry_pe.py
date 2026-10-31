"""Prove unchanged re-entry zero stores across MSVC register allocations."""
from capstone.x86_const import X86_REG_R14,X86_REG_R14D,X86_REG_R14W,X86_REG_R14B
from capstone.x86_const import X86_REG_RBP,X86_REG_EBP,X86_REG_BP,X86_REG_BPL

def verify_zero_rearm(body,flag,counters):
 candidates=[]
 for byte,whole,zero,aliases in [
  ('r14b','r14','r14d, r14d',{X86_REG_R14,X86_REG_R14D,X86_REG_R14W,X86_REG_R14B}),
  ('bpl','rbp','ebp, ebp',{X86_REG_RBP,X86_REG_EBP,X86_REG_BP,X86_REG_BPL})]:
  for i,x in enumerate(body):
   if x.mnemonic!='mov' or x.op_str!=f'byte ptr [rdi + {flag:#x}], {byte}':continue
   zeros=[j for j,y in enumerate(body[:i]) if y.mnemonic=='xor' and y.op_str==zero]
   assert zeros and not any(set(y.regs_access()[1])&aliases for y in body[zeros[-1]+1:i])
   stores=[j for j,y in enumerate(body[i+1:],i+1) if y.mnemonic=='mov' and y.op_str==f'qword ptr [rdi + {counters:#x}], {whole}']
   at,=stores
   assert not any(set(y.regs_access()[1])&aliases for y in body[i+1:at])
   candidates.append(i)
 assert len(candidates)==1
