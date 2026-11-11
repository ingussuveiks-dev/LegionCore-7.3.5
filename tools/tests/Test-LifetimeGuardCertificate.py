"""Independent x64 fixtures for the retained binary safety certificate."""
from capstone import Cs,CS_ARCH_X86,CS_MODE_64
from verify_ai_lifetime_guard import has_lifetime_gate
md=Cs(CS_ARCH_X86,CS_MODE_64);md.detail=True
p={'n':{'resolve':lambda x:x,'_ai_guard_runtime_helpers':set()}}
def fixture(offset,unsafe=False,bad_return=False):
 # closure: source pointer at8(explicit)or24(default capture),weak ctrl at40.
 code=bytearray();jumps=[]
 if unsafe:code+=bytes([0x4c,0x8b,0x41,offset])+bytes.fromhex('49 8b 50 20')
 code+=bytes.fromhex('48 8b 41 28 48 85 c0');jumps.append(len(code));code+=b'\x74\x00'
 code+=bytes.fromhex('83 78 08 00');jumps.append(len(code));code+=b'\x74\x00'
 code+=bytes([0x48,0x8b,0x41,offset])+bytes.fromhex('48 8b 48 20 c3');end=len(code)
 code+=bytes.fromhex('48 8b 49 20 c3')if bad_return else b'\xc3'
 for at in jumps:code[at+1]=end-at-2
 return list(md.disasm(bytes(code),0x1000))
for offset in [8,24]:
 assert has_lifetime_gate(p,fixture(offset))
 for bad in [fixture(offset,unsafe=True),fixture(offset,bad_return=True)]:
  try:accepted=has_lifetime_gate(p,bad)
  except AssertionError:accepted=False
  assert not accepted,'unsafe capture access/expired-branch payload was certified'
print('PASS binary lifetime certificate:both explicit/default capture layouts admitted;four pre-gate/expired-branch unsafe counterfactuals refused')
