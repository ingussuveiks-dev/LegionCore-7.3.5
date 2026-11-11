"""Verify the MSVC weak-use-count admission before callback payload access."""
import re

def has_lifetime_gate(p,rows):
 n=p['n']
 if '_ai_guard_runtime_helpers'not in n:
  text=n.get('maptext')
  if text is None:text=(n['args'].release/'worldserver.map').read_text(errors='replace')
  n['_ai_guard_runtime_helpers']={n['resolve'](int(address,16))for name,address in re.findall(r'^\s+\w+:\w+\s+(__security_check_cookie|__chkstk|__alloca_probe)\s+([0-9a-fA-F]{16})\s+f',text,re.M)}
 runtime_helpers=n['_ai_guard_runtime_helpers']
 def target(x):return int(x.op_str,16)if x.op_str.startswith('0x')else None
 def epilogue(address,stack=None,visited=None):
  stack=set(stack or {'rsp'});visited=set(visited or ())
  if address in visited:return False
  visited.add(address);index=next((i for i,x in enumerate(rows)if x.address==address),None)
  if index is None:return False
  for x in rows[index:index+64]:
   if x.mnemonic.startswith('ret'):return True
   if x.mnemonic=='jmp':return epilogue(target(x),stack,visited)
   if x.mnemonic=='call'and target(x)is not None and n['resolve'](target(x))in runtime_helpers:continue
   if x.mnemonic not in ['add','pop','mov','lea','nop','xor','movaps','movdqa','movups','movdqu']:return False
   if x.mnemonic!='lea'and any(base not in stack for base in re.findall(r'\[(r\w+)',x.op_str)):return False
   if x.mnemonic in ['lea','mov']:
    m=re.fullmatch(r'(r\w+), (?:\[(r\w+)[^]]*\]|(r\w+))',x.op_str)
    if m:
     if (m[2]or m[3])in stack:stack.add(m[1])
     else:stack.discard(m[1])
  return False
 for i,x in enumerate(rows):
  m=re.fullmatch(r'dword ptr \[(r\w+) \+ 8\], 0',x.op_str)if x.mnemonic=='cmp'else None
  if not m or i+1>=len(rows)or rows[i+1].mnemonic not in ['je','jz']:continue
  start=i
  while start and rows[start].address==rows[start-1].address+rows[start-1].size:start-=1
  if i-start>128:continue
  reg=m[1];nulls=[j for j in range(max(start,i-8),i-1)if rows[j].mnemonic=='test'and rows[j].op_str==reg+', '+reg and rows[j+1].mnemonic in ['je','jz']]
  if not nulls:continue
  stack={'rsp'}
  aliases={'eax':'rax','ebx':'rbx','ecx':'rcx','edx':'rdx','edi':'rdi','esi':'rsi','ebp':'rbp','esp':'rsp'}
  for y in rows[start:i]:
   before=set(stack)
   for regId in y.regs_access()[1]:
    reg=aliases.get(y.reg_name(regId),re.sub(r'(r\d+)[dwb]$',r'\1',y.reg_name(regId)))
    if reg!='rsp':stack.discard(reg)
   if y.mnemonic in ['lea','mov']:
    m=re.fullmatch(r'(r\w+), (?:\[(r\w+)[^]]*\]|(r\w+))',y.op_str)
    if m and(m[2]or m[3])in before:stack.add(m[1])
  if not epilogue(target(rows[i+1]),stack)or not any(epilogue(target(rows[j+1]),stack)for j in nulls):continue
  # No payload call may precede the admission in this function body.
  if any(y.mnemonic=='call'and(target(y)is None or n['resolve'](target(y))not in runtime_helpers)for y in rows[start:i]):continue
  # Loading a dangling pointer value is harmless;dereferencing it is not.
  # These explicit-this captures put this first;delegated lambda operators
  # receive the closure payload directly instead of std::function's +8 slot.
  # Explicit/default captures have different MSVC field order. Treat every
  # pointer-sized captured field as potentially dangling;only the captured
  # weak control block's final use-count read is admissible before the gate.
  control_reg=re.fullmatch(r'dword ptr \[(r\w+) \+ 8\], 0',rows[i].op_str)[1]
  state={'rcx':'closure'}
  def canon(reg):return {'eax':'rax','ebx':'rbx','ecx':'rcx','edx':'rdx','edi':'rdi','esi':'rsi','ebp':'rbp','esp':'rsp'}.get(reg,re.sub(r'(r\d+)[dwb]$',r'\1',reg))
  for y in rows[start:i+1]:
   before=dict(state)
   for base,offset in re.findall(r'\[(r\w+)(?: \+ (0x[0-9a-f]+|\d+))?\]',y.op_str):
    if y.mnemonic!='lea'and before.get(base)=='capture'and not(y.address==rows[i].address and base==control_reg and int(offset or'0',0)==8):raise AssertionError('captured pointer accessed before lifetime admission')
   for regId in y.regs_access()[1]:state.pop(canon(y.reg_name(regId)),None)
   if y.mnemonic=='mov':
    m=re.fullmatch(r'(r\w+), qword ptr \[(r\w+)(?: \+ (0x[0-9a-f]+|\d+))?\]',y.op_str)
    if m and before.get(m[2])=='closure':state[m[1]]='capture'
    else:
     m=re.fullmatch(r'(r\w+), (r\w+)',y.op_str)
     if m and m[2]in before:state[m[1]]=before[m[2]]
  return True
 return False

def verify_plan(p,plan):
 seen=[]
 for e in plan:
  found=[]
  for cls,method in sorted({(c['cls'],c['method'])for c in e['callbacks']}):
   for name in p['callbacks'](cls,method):
    if not name.endswith('EEAAXXZ')or cls+'@'not in name or method+'@'not in name:continue
    if method==cls and '???0'+cls+'@'not in name:continue
    if has_lifetime_gate(p,p['expanded'](name)):found.append(name)
  expected=sum(c.get('compiled_count',1)for c in e['callbacks']);assert len(found)==expected,(e['key'],'actual guarded closures',len(found),'expected',expected)
  seen+=found
 return seen
