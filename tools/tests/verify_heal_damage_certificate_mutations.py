from five_ten_20261111_pe import ins,n,decoder
from damage_scaling_release_check import check as damage
from heal_application_release_check import check as healing
import struct

d=ins('?DealDamage@Unit@@');h=ins('?DealHeal@Unit@@')
damage(n,ins);healing(n,ins)
def reject(label,body,fn,subject,native=n):
 def selected(prefix):return body if prefix==subject else ins(prefix)
 try:fn(native,selected)
 except (AssertionError,ValueError,StopIteration) as error:print('PASS expected rejected in-memory counterfactual '+label+' '+type(error).__name__);return
 raise AssertionError('Broken semantic mutation was accepted: '+label)
def without(body,index):
 x=body[index];replacement=list(decoder.disasm(bytes([0x90])*x.size,x.address));return body[:index]+replacement+body[index+1:]
conversion=next(i for i,x in enumerate(d)if x.mnemonic=='cvttsd2si'and x.op_str=='rax, xmm2')
reject('missing actual damage integer store',without(d,conversion+1),damage,'?DealDamage@Unit@@')
slot=next(i for i,x in enumerate(d)if x.mnemonic=='cmp'and x.op_str=='rcx, qword ptr [rbx + 0x9c0]')
reject('missing current-vs-suspended AI identity comparison',without(d,slot),damage,'?DealDamage@Unit@@')
limit=d[conversion-2];address=limit.address+limit.size+limit.operands[1].mem.disp;raw=bytearray(n['binary']);struct.pack_into('<d',raw,n['offset'](address),0.0);modified=dict(n,binary=bytes(raw))
reject('wrong numeric saturation limit',d,damage,'?DealDamage@Unit@@',modified)
bound=next(i for i,x in enumerate(h)if x.mnemonic=='cmovb'and x.op_str=='r14, rcx')
reject('missing actual missing-health bound',without(h,bound),healing,'?DealHeal@Unit@@')
amount=next(i for i,x in enumerate(h)if x.mnemonic=='mov'and x.op_str=='r8, qword ptr [rbp - 0x59]');x=h[amount];raw=bytearray(x.bytes);assert raw[0]&0xf0==0x40 and raw[0]&8;raw[0]&=~8;replacement=list(decoder.disasm(bytes(raw),x.address));assert replacement[0].op_str=='r8d, dword ptr [rbp - 0x59]'
reject('narrowed final native healing amount',h[:amount]+replacement+h[amount+1:],healing,'?DealHeal@Unit@@')
print('PASS two actual unchanged linked positive paths and five rejected semantic mutations; nothing written to executable or map')
