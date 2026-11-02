"""Actual registered Warrior intro timers and original-target AI recovery."""
from pathlib import Path
import hashlib,json,runpy
from capstone import CS_OP_IMM,CS_OP_MEM
from sharasdal_erunak_entry_pe import sites,values,fields
from monk_intro_clock_pe import verify_clock

p=runpy.run_path(str(Path(__file__).with_name('verify_monk_nine_czi_timers_release.py')))
n,refs,table,resolved=(p[k] for k in ('n','refs','table','resolved'))
ins=n['instructions']
def pairs(body):return [(op.mnemonic,op.op_str) for op in body]
def reachable(body,start):
 positions={op.address:i for i,op in enumerate(body)};pending=[start];seen=set()
 while pending:
  address=pending.pop()
  if address in seen or address not in positions:continue
  seen.add(address);op=body[positions[address]]
  if op.mnemonic in ('ret','int3','ud2'):continue
  if op.mnemonic.startswith('j') and op.operands[0].type==CS_OP_IMM:pending.append(op.operands[0].imm)
  if op.mnemonic!='jmp':pending.append(op.address+op.size)
 return seen
new_opening=any(name.startswith('?GetData@npc_ragnvald_dragonbornAI@') for name in n['symbols'])
actors=('npc_ragnvald_dragonborn','npc_danica_the_reclaimer')
parents=[];callbacks=[]
for actor,member,size,event,delay in zip(actors,(0x60,0x40),(0xe0,0xc0),('MoveInLineOfSight','IsSummonedBy'),(2000,3000)):
 if new_opening and actor=='npc_ragnvald_dragonborn':event='DoAction'
 suffix=actor+'AI@'+actor+'@@';method=lambda name:'?'+name+'@'+suffix
 address,raw=table('??_7'+suffix+'6B@',82);slots=resolved(raw)
 script,sraw=table('??_7'+actor+'@@6BScriptObject@@@',16)
 factory=ins('?GetAI@'+actor+'@@');destroy=ins('??_G'+suffix)
 assert address in refs(factory) and script in refs(ins('?AddSC_wars_intro_scenario@@'))
 assert n['symbol']('?GetAI@'+actor+'@@') in resolved(sraw)
 assert slots[0]==n['symbol']('??_G'+suffix) and slots[0x110//8]==n['symbol'](method('UpdateLifecycle'))
 assert n['symbol'](method(event)) in slots and size in values(factory)
 ctor,=sites(n,factory,'??0FunctionProcessor@@');dtor,=sites(n,destroy,'??1FunctionProcessor@@')
 assert factory[ctor-1].mnemonic=='lea' and f'+ {member:#x}]' in factory[ctor-1].op_str
 assert pairs(destroy)[dtor-1]==('add',f'rcx, {member:#x}')
 verify_clock(n,method('UpdateLifecycle'),member)
 parent=ins(method(event));parents.append(parent)
 queued,=sites(n,parent,'?AddDelayedEvent@FunctionProcessor@@')
 assert any(op.mnemonic=='lea' and op.op_str.endswith(f'+ {member:#x}]') and op.op_str.startswith('rcx,') for op in parent[queued-12:queued])
 assert ('mov',f'edx, {delay:#x}') in pairs(parent[queued-3:queued])
 callback,=[name for name in n['symbols'] if name.startswith('?_Do_call@') and event+'@'+suffix in name]
 callbacks.append(callback)
 name='??_7'+callback[len('?_Do_call@'):].removesuffix('EEAAXXZ')+'6B@'
 vt,raw=table(name,7);assert vt in refs(parent) and n['symbol'](callback) in resolved(raw)

dispatch=ins('?Update@Creature@@')
assert sum(op.mnemonic=='call' and op.operands[0].type==CS_OP_MEM and op.operands[0].mem.disp==0x110 for op in dispatch)>=2
danica=ins(callbacks[1]);helper='?AttackBattleTarget@npc_ragnvald_dragonbornAI@'
opening=None
if new_opening:
 from warrior_opening_pe import verify_actor
 opening=verify_actor(n,ins,table,resolved,reachable,parents[0],callbacks[0])
 battle=opening['body']
else:
 bridge=ins(callbacks[0]);assert pairs(bridge)[:1]==[('add','rcx, 8')] and bridge[1].mnemonic=='jmp'
 outlined,=[name for name in n['symbols'] if name.startswith('??R<lambda_') and 'MoveInLineOfSight@npc_ragnvald_dragonbornAI@' in name and n['symbol'](name)==n['resolve'](bridge[1].operands[0].imm)]
 battle=ins(outlined);helper='?AttackBattleTarget@npc_ragnvald_dragonbornAI@'
 assert len(sites(n,battle,helper))==len(sites(n,battle,'?GetGuidValue@Object@@'))==3
 assert {102990,102995,97166,97167,97176,102970,102964,102965,105419}<=values(battle)
 assert len(sites(n,battle,'?SetHomePosition@Creature@@QEAAXMMMM'))==2
 own_attack,=[i for i,op in enumerate(battle) if op.mnemonic=='call' and op.op_str=='r8']
 assert pairs(battle[own_attack-4:own_attack])==[('mov','rcx, qword ptr [rsi]'),('mov','rdx, qword ptr [rcx]'),('mov','r8, qword ptr [rdx + 0x10]'),('mov','rdx, rax')]
 assert ('cmove','rdi, rdx') in pairs(battle) and ('cmp','qword ptr [rbx + 0x10], rcx') in pairs(battle)
 for at in sites(n,battle,helper):
  assert ('mov','r9b, 0xa') in pairs(battle[at-6:at])
  assert any(op.mnemonic=='movups' and op.op_str=='xmm0, xmmword ptr [rax]' for op in battle[at-12:at])
assert {1405,205287,49203,49262,92}<=values(danica)
assert sites(n,danica,'?CreateConversation@Conversation@@') and sites(n,danica,'?DoCast@UnitAI@@QEAAXI')
assert len(sites(n,danica,'?UpdateAchievementCriteria@Player@@'))==2

body=ins(helper);lookup,=sites(n,body,'?GetCreature@ObjectAccessor@@')
queued,=sites(n,body,'?AddDelayedEvent@FunctionProcessor@@')
attack,=[i for i,op in enumerate(body) if op.mnemonic=='call' and op.op_str=='qword ptr [rax + 0x10]']
assert ('movups','xmm0, xmmword ptr [rdx]') in pairs(body[:lookup])
assert ('mov','r14, rdx') in pairs(body[:lookup]) and ('mov','rbx, rcx') in pairs(body[:lookup])
for condition,jump in (('dword ptr [rbx + 0xa18], 0','jne'),('rax, rax','je'),('dword ptr [rax + 0xa18], 0','jne')):
 at=next(i for i,op in enumerate(body) if op.mnemonic in ('cmp','test') and op.op_str==condition)
 assert body[at+1].mnemonic==jump
 rejected=reachable(body,body[at+1].operands[0].imm)
 assert body[queued].address not in rejected and body[attack].address not in rejected
current=next(i for i,op in enumerate(body) if op.mnemonic=='mov' and op.op_str=='rcx, qword ptr [rbx + 0x9c0]')
assert pairs(body[current+1:current+7])==[('test','rcx, rcx'),('jne',body[current+2].op_str),('mov','rcx, rdi'),('test','rdi, rdi'),('jne',body[current+5].op_str),('test','sil, sil')]
assert body[current+2].operands[0].imm==body[current+5].operands[0].imm==body[attack-1].address
assert body[current+7].mnemonic=='jne'
assert body[queued].address in reachable(body,body[current+7].operands[0].imm)
exhausted=reachable(body,body[current+8].address)
assert body[queued].address not in exhausted and body[attack].address not in exhausted
assert sites(n,body,'?ShouldLog@Log@@') and sites(n,body,'?outMessage@Log@@')
assert b'Warrior intro: ally %s has no AI for its delayed attack after 10 retries' in n['binary']
assert {('lea','rcx, [rbx + 0x36a0]'),('mov','edx, 0x1f4'),('mov','qword ptr [rsp + 0xc8], rbx'),('movups','xmm0, xmmword ptr [r14]'),('movaps','xmmword ptr [rsp + 0xd0], xmm0')}<=set(pairs(body))
retry,=[name for name in n['symbols'] if name.startswith('?_Do_call@') and 'AttackBattleTarget@npc_ragnvald_dragonbornAI@' in name]
vt,raw=table('??_7'+retry[len('?_Do_call@'):].removesuffix('EEAAXXZ')+'6B@',7)
assert vt in refs(body) and n['symbol'](retry) in resolved(raw)
assert {('movups','xmm0, xmmword ptr [rcx + 0x10]'),('movzx','r9d, byte ptr [rcx + 0x20]'),('mov','rcx, qword ptr [rcx + 8]'),('dec','r9b'),('xor','r8d, r8d')}<=set(pairs(ins(retry)))
assert sites(n,ins(retry),helper)
for decoded in (battle,body,ins(retry)):
 for forbidden in ('?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@'):
  assert not sites(n,decoded,forbidden)
print('PASS actual registered Ragnvald/Danica factories/vtables/lifecycle110/constructor-destructor members60/40, two owned2/3s callback vtables and native ALIVE-CORPSE clocks. Three original-target native attack calls retain current-AI/self-only fallback; ally-owned full128-bit GUID500ms/10 retry drops AI fallback capture, rejects missing/dead actors and diagnoses exhaustion. Native original combat/home/1405/205287/49203/49262 retained; no helper progression grants. Full933/client and other admission/failure branches remain separate.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'owned_callbacks':2,'ally_retry_callbacks':1}))
