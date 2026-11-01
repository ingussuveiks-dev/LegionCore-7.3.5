"""Actual weak AI lifetime, original participants and late native blessing."""
import hashlib
import json
import re
import struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

def verify(p):
 n,ins,reachable=p['native'],p['instructions'],p['reachable'];actor=p['actor']
 def method(name):return '?'+name+'@'+actor
 def pairs(body):return [(op.mnemonic,op.op_str) for op in body]
 def sites(body,prefix):
  target=n['symbol'](prefix)
  return [i for i,op in enumerate(body) if op.mnemonic in ('call','jmp') and op.operands[0].type==CS_OP_IMM and n['resolve'](op.operands[0].imm)==target]
 initial=ins(p['opening']);outer=ins(p['outer']);inner=ins(p['callback'])
 ctor=ins('?GetAI@npc_vigfus_bladewind_last@@');dtor=ins('??_G'+actor)
 assert ('mov','ecx, 0x128') in pairs(ctor)
 assert {('mov','qword ptr [rsi + 0x118], rcx'),('mov','qword ptr [rsi + 0x120], rax')}<=set(pairs(ctor))
 remove,=sites(dtor,method('RemoveIntroConversation'))
 assert {('mov','qword ptr [rcx + 0x118], 0'),('mov','qword ptr [rcx + 0x120], 0'),('lock xadd','dword ptr [rbx + 8], eax')}<=set(pairs(dtor[:remove]))
 table=re.findall(r'\s+\w+:\w+\s+\?\?_7npc_vigfus_bladewind_lastAI@npc_vigfus_bladewind_last@@6B@\s+([0-9a-fA-F]{16})',(n['args'].release/'worldserver.map').read_text(errors='replace'))
 address,=map(lambda value:int(value,16),table)
 assert n['resolve'](struct.unpack_from('<Q',n['binary'],n['offset'](address))[0])==n['symbol']('??_G'+actor)
 assert address in {op.address+op.size+arg.mem.disp for op in ctor for arg in op.operands if arg.type==CS_OP_MEM and arg.mem.base==X86_REG_RIP}

 cast_prefix='?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@IW4TriggerCastFlags@@'
 cast,=sites(outer,cast_prefix);aura,=sites(outer,'?HasAura@Unit@@');timer,=sites(outer,'?AddDelayedEvent@FunctionProcessor@@')
 aura_remove,=sites(inner,'?RemoveAura@Unit@@QEAAXIVObjectGuid@@')
 targets=sites(outer,method('IsIntroTarget'));actors=sites(outer,method('IsIntroHelya'))
 assert len(targets)==len(actors)==2 and targets[0]<actors[0]<cast<targets[1]<actors[1]<aura<timer
 assert not sites(initial,cast_prefix) and not sites(initial,'?HasAura@Unit@@')
 assert len(sites(initial,method('IsIntroHelya')))==len(sites(initial,'?CreateConversation@Conversation@@'))==1
 for register,destination in (('r14','xmm7'),('rbx','xmm8')):
  assert any(initial[i-1].op_str=='rcx, '+register and initial[i+1].op_str==destination+', xmmword ptr [rax]' for i in sites(initial,'?GetGuidValue@Object@@'))
 assert {('movups','xmmword ptr [rax + 0x24], xmm7'),('movups','xmmword ptr [rax + 0x34], xmm8'),('mov','rbx, qword ptr [rdi + 0x120]'),('mov','r14, qword ptr [rdi + 0x118]')}<=set(pairs(initial))
 queued,=sites(initial,'?AddDelayedEvent@FunctionProcessor@@')
 assert ('lea','rcx, [r13 + 0x36a0]') in pairs(initial[queued-5:queued]) and ('mov','edx, 0x5dc0') in pairs(initial[queued-5:queued])
 assert ('lea','rcx, [r12 + 0x36a0]') in pairs(outer[timer-5:timer]) and ('mov','edx, 0x1b58') in pairs(outer[timer-5:timer])

 def weak_guard(body,start,ctx,control,zero):
  window=body[start:start+45]
  load=next(i for i,op in enumerate(window) if op.mnemonic=='mov' and op.op_str==f'{control}, qword ptr [{ctx} + 0x10]')
  tested=load+1
  if (window[tested].mnemonic,window[tested].op_str)==('mov','rdi, rcx'):
   assert ctx=='rcx' and control=='rax'
   tested+=1
  assert (window[tested].mnemonic,window[tested].op_str)==('test',f'{control}, {control}')
  assert window[tested+1].mnemonic=='je'
  assert (window[tested+2].mnemonic,window[tested+2].op_str)==('cmp',f'dword ptr [{control} + 8], {zero}') and window[tested+3].mnemonic=='je'
  owner=next(i for i,op in enumerate(window) if op.mnemonic=='mov' and op.op_str==f'rcx, qword ptr [{ctx}]')
  assert tested+3<owner,'Expired weak control must precede the captured raw AI'
  epoch=next(i for i in range(owner+1,owner+8) if window[i].mnemonic=='mov' and window[i].op_str in ('eax, dword ptr [rcx + 0xc8]','edx, dword ptr [rcx + 0xc8]'))
  epoch_register=window[epoch].op_str.split(',')[0]
  capture='r14' if body is outer else 'rdi + 0x18'
  assert (window[epoch+1].mnemonic,window[epoch+1].op_str)==('cmp',f'dword ptr [{capture}], {epoch_register}') and window[epoch+2].mnemonic=='jne'
  pending=next(i for i in range(epoch+3,epoch+6) if window[i].mnemonic=='cmp' and window[i].op_str in ('byte ptr [rcx + 0xc7], 0','byte ptr [rcx + 0xc7], sil'))
  assert window[pending+1].mnemonic=='je'
  forbidden={body[i].address for name in (method('IsIntroTarget'),method('IsIntroHelya'),'?GetPlayer@ObjectAccessor@@','?GetCreature@ObjectAccessor@@',cast_prefix,'?RemoveAura@Unit@@QEAAXIVObjectGuid@@','?AddDelayedEvent@FunctionProcessor@@') for i in sites(body,name)}
  forbidden|={op.address for op in body if op.mnemonic in ('call','jmp') and op.operands[0].type==CS_OP_MEM and op.operands[0].mem.disp==0x10}
  for branch in (window[tested+1],window[tested+3],window[epoch+2],window[pending+1]):assert not (forbidden & reachable(body,branch.operands[0].imm))
 weak_guard(outer,0,'rcx','rax','0');weak_guard(inner,0,'rcx','rax','0')
 assert ('xor','esi, esi') in pairs(outer[:cast])
 weak_guard(outer,cast+1,'r15','rcx','esi');weak_guard(inner,aura_remove+1,'rdi','rax','0')

 players=sites(outer,'?GetPlayer@ObjectAccessor@@');helyas=sites(outer,'?GetCreature@ObjectAccessor@@')
 assert len(players)==len(helyas)==2 and players[0]<helyas[0]<cast<players[1]<helyas[1]<targets[1]
 assert {('lea','rax, [r15 + 0x1c]'),('lea','r13, [r15 + 0x2c]')}<=set(pairs(outer[:cast]))
 assert outer[helyas[0]+1].op_str=='r12, rax'
 assert [op.op_str for op in outer[cast-4:cast]]==['r9d, r9d','r8d, 0x2f9f6','rdx, qword ptr [rdx + 0x20]','rcx, r12']
 assert outer[cast+1].op_str=='dword ptr [rsp + 0x40], eax'
 assert {('movups','xmm0, xmmword ptr [r13]'),('lea','r8, [rsp + 0xd0]'),('mov','edx, 0x2f9f6'),('mov','rcx, qword ptr [rcx + 0x20]')}<=set(pairs(outer[aura-12:aura]))
 result=next(i for i,op in enumerate(outer[cast:aura],cast) if op.mnemonic=='cmp' and op.op_str=='dword ptr [rsp + 0x40], esi')
 failures=[outer[result+1],outer[targets[1]+2],outer[actors[1]+2],outer[aura+2]]
 assert [op.mnemonic for op in failures]==['jne','je','je','je'] and len({op.operands[0].imm for op in failures})==1
 failed=reachable(outer,failures[0].operands[0].imm)
 assert outer[timer].address not in failed and any(outer[i].address in failed for i in sites(outer,method('RemoveIntroConversation')))
 for checked in (targets[0],actors[0]):
  branch=outer[checked+2]
  assert branch.mnemonic=='je' and outer[timer].address not in reachable(outer,branch.operands[0].imm)
 assert {('movups','xmmword ptr [rax + 0x24], xmm7'),('movups','xmmword ptr [rax + 0x34], xmm6')}<=set(pairs(outer[aura:timer]))
 assert not sites(outer,'?FindNearestPlayer@WorldObject@@') and not sites(inner,'?FindNearestPlayer@WorldObject@@')

 for field in (0x1c,0x2c):assert pairs(inner).count(('movups',f'xmm0, xmmword ptr [rdi + {field:#x}]'))==2
 assert len(sites(inner,'?GetPlayer@ObjectAccessor@@'))==len(sites(inner,'?GetCreature@ObjectAccessor@@'))==2
 checks=sites(inner,method('IsIntroHelya'));assert len(checks)==2 and checks[0]<aura_remove<checks[1]
 consumed,=[i for i,op in enumerate(inner) if op.mnemonic=='mov' and op.op_str=='byte ptr [rax + 0xc7], 0']
 engagement,=[i for i,op in enumerate(inner) if op.mnemonic=='call' and op.op_str=='qword ptr [rax + 0x10]']
 assert checks[1]<consumed<engagement
 assert ('mov','dword ptr [rcx + 0x100], 0x3e8') in pairs(outer) and ('mov','word ptr [rcx + 0xc6], 0') in pairs(outer)
 helya=ins(method('IsIntroHelya'))
 for dependency in ('?GetUInt32Value@Object@@','?IsInWorld@Object@@','?IsPreDelete@Object@@','?InSamePhase@WorldObject@@','?GetExactDist2d@Position@@QEBAMPEBU1@@Z'):
  assert sites(helya,dependency)
 for condition in ('eax, 0x182e3','dword ptr [rbx + 0xa18], 0','qword ptr [rbx + 8], rax'):
  assert any(op.mnemonic=='cmp' and op.op_str==condition and helya[i+1].mnemonic=='jne' for i,op in enumerate(helya[:-1]))
 radius=next(op for op in helya if op.mnemonic=='movss' and op.op_str.startswith('xmm1, dword ptr [rip'))
 assert struct.unpack_from('<f',n['binary'],n['offset'](radius.address+radius.size+radius.operands[1].mem.disp))[0]==100.0
 assert any(op.mnemonic=='comiss' and op.op_str=='xmm1, xmm0' and helya[i+1].mnemonic=='jb' for i,op in enumerate(helya[:-1]))
 assert ('mov','dword ptr [rdi + 0x100], 0x3e8') in pairs(initial)
 assert any(op.mnemonic=='cmp' and op.op_str=='dword ptr [rcx + 0x100], 0' and initial[i+1].mnemonic=='jne' for i,op in enumerate(initial[:-1]))
 update=ins(method('UpdateAI'))
 countdown=next(i for i,op in enumerate(update) if op.mnemonic=='mov' and op.op_str=='dword ptr [rcx + 0x100], eax')
 assert any(op.mnemonic=='cmovbe' for op in update[:countdown]) and countdown<sites(update,method('UpdateIntro'))[0]
 for text in (b'Warswords intro waiting for live same-phase Helya99043',b'Warswords intro waiting for native Helya195062 aura',b'Warswords intro recovered native Helya195062 aura',b'Warswords intro lost its player or Helya before the late blessing'):
  assert text in n['binary']
 for body in (initial,outer,inner,dtor):
  for forbidden in ('?CompleteQuest@Player@@','?KilledMonsterCredit@Player@@','?AddItem@Player@@','?TeleportTo@Player@@'):
   targets={n['resolve'](address) for name,address in n['symbols'].items() if name.startswith(forbidden)}
   assert not any(op.mnemonic in ('call','jmp') and op.operands[0].type==CS_OP_IMM and n['resolve'](op.operands[0].imm) in targets for op in body)
 print('PASS selected Vigfus constructor128/lifetime118-120 and deleting vtable retire strong ownership before843 cleanup; both weak closures reject expired AI before entry/post-cast/post-aura dereferences. Original full player/Helya GUIDs, normal195062 actual recipient/caster-aura receipt, Unit-owned24+7s timers, failed-cast cancellation/1s retry and final fresh-context-before-pending/AttackStart verified; no synthetic progression. Client/full944 remains separate.')
 print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'lifetime_guards':4,'native_unit_timers':2}))
 return {'lifetime_verified':True,'body':outer,'sites':sites}
