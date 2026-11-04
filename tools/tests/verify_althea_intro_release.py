"""Selected actual Althea LOS/receipt/voice/guard release implementation in Release."""
from pathlib import Path
import runpy,re,struct
from capstone import CS_OP_MEM,CS_OP_IMM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites,values,floats
p=runpy.run_path(str(Path(__file__).with_name('verify_kingslayers_door_release.py')))
n=p['n'];ins=n['instructions'];method=lambda name:'?'+name+'@npc_sister_althea_ebonlockeAI@npc_sister_althea_ebonlocke@@'
pairs=lambda body:[(x.mnemonic,x.op_str) for x in body]
def reachable(body,start):
 indexed={op.address:i for i,op in enumerate(body)};pending=[start];seen=set()
 while pending:
  at=pending.pop()
  if at in seen or at not in indexed:continue
  seen.add(at);i=indexed[at];op=body[i]
  if op.mnemonic.startswith('ret'):continue
  if op.mnemonic=='jmp' and op.operands[0].type==CS_OP_IMM:pending.append(op.operands[0].imm);continue
  if op.mnemonic.startswith('j') and op.operands[0].type==CS_OP_IMM:pending.append(op.operands[0].imm)
  if i+1<len(body):pending.append(body[i+1].address)
 return seen
eligible=ins(method('IntroPlayer'));los=ins(method('MoveInLineOfSight'));voice=ins(method('IntroVoice'));tick=ins(method('UpdateIntro'));opponent=ins(method('IntroOpponent'))
assert {108218,1620,1123,42504,42627,48577,48585}<=values(eligible)
for target,count in [('?IsInWorld@Object@@',2),('?IsPreDelete@Object@@',2),('?IsQuestObjectExpired@WorldObject@@',2),('?CanContact@Player@@',1),('?InSamePhase@WorldObject@@',1),('?CanTargetQuestObject@WorldObject@@',2),('?GetQuestRewardStatus@Player@@',1),('?GetQuestStatus@Player@@',2)]:assert len(sites(n,eligible,target))==count,(target,count)
assert {('cmp','dword ptr [rcx + 0xa18], 0'),('cmp','dword ptr [rbx + 0xa18], 0'),('test','byte ptr [rbx + 0x4ff0], 1'),('cmp','al, 4')}<=set(pairs(eligible))
for offset in (0x6848,0x6849,0x6851):assert ('cmp',f'byte ptr [rbx + {hex(offset)}], 0') in pairs(eligible)
assert ('cmp','qword ptr [rbx + 8], rax') in pairs(eligible)
award,=sites(n,los,'?UpdateAchievementCriteria@Player@@');checks=sites(n,los,method('IntroPlayer'));leaves=sites(n,los,method('IntroLeaf'));voices=sites(n,los,method('IntroVoice'))
assert len(checks)>=3 and len(leaves)==2 and len(voices)==2 and checks[0]<leaves[0]<award<checks[1]<leaves[1]<voices[0]<checks[2]<voices[1]
assert {51549,92,48577,1000,108214}<=values(los) and {55.0,35.0,60.0}<=floats(n,los)
checkpoint,=[x for x in los if x.mnemonic=='mov' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0x38 and x.operands[1].type==CS_OP_IMM and x.operands[1].imm==1]
last=leaves[1];assert pairs(los[last+1:last+3])==[('test','al, al'),('jne',los[last+2].op_str)] and los[last+2].operands[0].imm==checkpoint.address
assert checkpoint.address not in reachable(los,los[last+3].address),'Rejected native receipt must not consume introDone'
garona,=sites(n,los,'?FindNearestCreature@WorldObject@@');assert checks[2]<garona<voices[1]
assert len(sites(n,los,'?CanTargetQuestObject@WorldObject@@'))>=4
create,=sites(n,voice,'?CreateConversation@Conversation@@');assert {2422,2424,2425,1000}<=values(voice)
assert voice[create+1].mnemonic=='test' and voice[create+2].mnemonic=='jne'
accepted=voice[create+2].operands[0].imm
assert accepted in reachable(voice,accepted) and any(x.address==accepted and x.mnemonic=='mov' and x.op_str=='byte ptr [rdi + rsi + 0x62], 1' for x in voice)
assert accepted not in reachable(voice,voice[create+3].address),'Failed native factory must retain presentation retry'
assert ('call','qword ptr [rax]') in pairs(voice[create+3:]),'False factory must release caller-owned allocation'
assert ('mov','dword ptr [rcx + rdi*4 + 0x68], 0x3e8') in pairs(voice[:create])
assert len(sites(n,opponent,method('IntroPlayer')))==1 and sites(n,opponent,'?GetPlayerListInGrid@WorldObject@@') and 50.0 in floats(n,opponent)
assert not sites(n,opponent,'?FindNearestPlayer@WorldObject@@')
pick=sites(n,tick,method('IntroOpponent'));jump,=sites(n,tick,'?MoveJump@MotionMaster@@');remove,=sites(n,tick,'?RemoveAura@Unit@@');outro,=sites(n,tick,method('IntroVoice'))
assert len(pick)==2 and pick[0]<jump<remove<pick[1]<outro
assert {214873,1000}<=values(tick) and {-8614.58,403.17,102.92,5.0}<=floats(n,tick)
assert tick[pick[0]+2].mnemonic=='test' and tick[pick[0]+3].mnemonic=='je'
assert tick[jump].address not in reachable(tick,tick[pick[0]+3].operands[0].imm),'No eligible opponent must retain guard poll without releasing movement/protection'
assert ('call','qword ptr [rax + 0x10]') in pairs(tick[remove:outro]),'Actual AI AttackStart handoff absent'
update=ins(method('UpdateAI'));poll,=sites(n,update,method('UpdateIntro'));victim,=sites(n,update,'?UpdateVictim@CreatureAI@@');assert poll<victim
assert not sites(n,tick,'?UpdateAchievementCriteria@Player@@') and not sites(n,voice,'?UpdateAchievementCriteria@Player@@')
for part in (eligible,los,voice,tick,opponent):
 for forbidden in ('?AddItem@Player@@','?CompleteQuest@Player@@','?KilledMonsterCredit@Player@@'):assert not sites(n,part,forbidden)
assert not {51613,51654}.intersection(values(los+voice+tick)), 'Intro must not fabricate independent victory/door credit'
for message in (b'Althea intro: conversation %u unavailable;',b'Althea intro: native51549 not acknowledged;'):assert message in n['binary']
maptext=(n['args'].release/'worldserver.map').read_text(errors='replace')
table,=re.findall(r'\s+\w+:\w+\s+\?\?_7npc_sister_althea_ebonlockeAI@npc_sister_althea_ebonlocke@@6B@\s+([0-9a-fA-F]{16})',maptext);address=int(table,16)
factory=ins('?GetAI@npc_sister_althea_ebonlocke@@');refs={x.address+x.size+o.mem.disp for x in factory for o in x.operands if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP};assert address in refs
slots=struct.unpack_from('<82Q',n['binary'],n['offset'](address))
for name,caller in [('MoveInLineOfSight','?MoveInLineOfSight_Safe@CreatureAI@@'),('UpdateAI','?Update@Creature@@')]:
 slot,=[i for i,v in enumerate(slots) if n['resolve'](v)==n['symbol'](method(name))]
 assert any(x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==slot*8 for x in ins(caller)),(name,slot)
from althea_guard_progress_pe import verify
verify(n)
print('PASS actual registered Althea LOS/update vtable, qualified Player/quest/native1123 gates, sole51549 followed by actual counter acknowledgement, mandatory exact guard/Garona handoff, independent bounded-rate2422/2424/2425 retries and diagnostics; instance-owned real guard deaths precede eligible50-yard selection, original jump/protection/attack and combat-tail dispatch retained. Native death/door proofs remain separate and retained; no client/full combat/traversal claim.')
