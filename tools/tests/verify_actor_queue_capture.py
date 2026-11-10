"""Track the closure's captured pointer separately from freshly read actor members."""
import re
def check_actor_capture(p,rows):
 state={'rcx':'closure'};loaded=False
 helper={p['n']['resolve'](a)for name,a in p['n']['symbols'].items()if name.startswith(('?Talk@CreatureAI@@','?DoCast@UnitAI@@'))}
 for x in rows:
  before=dict(state)
  memory=re.findall(r'\[(r\w+)(?: \+ (0x[0-9a-f]+|\d+))?\]',x.op_str)
  for reg,offset in memory:
   if x.mnemonic!='lea'and state.get(reg)=='captured'and offset and int(offset,0)==0x20:
    raise AssertionError('captured old AI dereferenced for me')
  if x.mnemonic in ('call','jmp')and x.op_str.startswith('0x'):
   target=p['n']['resolve'](int(x.op_str,16))
   if target in helper and state.get('rcx')=='captured':raise AssertionError('old captured AI passed directly to AI helper')
  def canonical(reg):
   aliases={'eax':'rax','ax':'rax','al':'rax','ah':'rax','ebx':'rbx','bx':'rbx','bl':'rbx','bh':'rbx','ecx':'rcx','cx':'rcx','cl':'rcx','ch':'rcx','edx':'rdx','dx':'rdx','dl':'rdx','dh':'rdx','edi':'rdi','di':'rdi','dil':'rdi','esi':'rsi','si':'rsi','sil':'rsi','ebp':'rbp','bpl':'rbp','esp':'rsp','spl':'rsp'}
   return aliases.get(reg,re.sub(r'(r\d+)[dwb]$',r'\1',reg))
  for reg in x.regs_access()[1]:state.pop(canonical(x.reg_name(reg)),None)
  if x.mnemonic=='mov':
   m=re.fullmatch(r'(r\w+), qword ptr \[(r\w+)(?: \+ (0x[0-9a-f]+|\d+))?\]',x.op_str)
   if m:
    dest,base,offset=m.groups();offset=int(offset,0)if offset else 0
    if (before.get(base)=='closure'and offset==8)or(before.get(base)=='lambda'and offset==0):state[dest]='captured';loaded=True
   else:
    m=re.fullmatch(r'(r\w+), (r\w+)',x.op_str)
    if m and m[2]in before:state[m[1]]=before[m[2]]
  if x.mnemonic=='add':
   m=re.fullmatch(r'(r\w+), 8',x.op_str)
   if m and before.get(m[1])=='closure':state[m[1]]='lambda'
  if x.mnemonic=='lea':
   m=re.fullmatch(r'(r\w+), \[(r\w+) \+ 8\]',x.op_str)
   if m and before.get(m[2])=='closure':state[m[1]]='lambda'
  if x.mnemonic=='call':
   for reg in ['rax','rcx','rdx','r8','r9','r10','r11']:state.pop(reg,None)
 assert loaded,'closure must directly load its captured actor'
