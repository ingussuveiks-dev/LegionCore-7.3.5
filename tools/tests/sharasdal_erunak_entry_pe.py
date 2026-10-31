"""Selected lower-entry implementation checks shared by retained PE inspectors."""
import struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
SUFFIX='@npc_erunak_stonespeakerAI@npc_erunak_stonespeaker@@'
def method(name):return '?'+name+SUFFIX
def sites(n,body,prefix):
 targets={n['resolve'](at) for name,at in n['symbols'].items() if name.startswith(prefix)}
 return [i for i,x in enumerate(body) if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm) in targets]
def values(body):return {o.imm for x in body for o in x.operands if o.type==CS_OP_IMM}
def fields(body):return {o.mem.disp for x in body for o in x.operands if o.type==CS_OP_MEM}
def floats(n,body):
 result=set()
 for x in body:
  for o in x.operands:
   if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP:
    try:result.add(round(struct.unpack_from('<f',n['binary'],n['offset'](x.address+x.size+o.mem.disp))[0],3))
    except (ValueError,struct.error):pass
 return result
def verify_lifecycle(n):
 if any(name.startswith(method('AcceptUpperIntro')) for name in n['symbols']):
  from sharasdal_erunak_upper_pe import verify_lifecycle as upper_lifecycle
  upper_lifecycle(n)
  return
 life=n['instructions'](method('UpdateLifecycle'))
 assert sites(n,life,'?Recover@SharasdalCompanion@@')==[8]
 assert sites(n,life,'?Update@FunctionProcessor@@')==[11]
 assert sites(n,life,method('CompleteLowerEntries'))==[17] and life[17].mnemonic=='jmp'
 assert [(x.mnemonic,x.op_str) for x in life[3:14]]==[
  ('mov','ebx, edx'),('lea','r8, [rcx + 0x59]'),('lea','rdx, [rcx + 0x5b]'),
  ('mov','rdi, rcx'),('mov','rcx, qword ptr [rcx + 0x20]'),('call',life[8].op_str),
  ('lea','rcx, [rdi + 0x60]'),('mov','edx, ebx'),('call',life[11].op_str),
  ('mov','edx, ebx'),('mov','rcx, rdi')]
def verify(n):
 verify_lifecycle(n)
 ins=n['instructions'];world=ins(method('LowerEntryWorld'));player=ins(method('LowerEntryPlayer'))
 accept=ins(method('AcceptLowerEntry'));complete=ins(method('CompleteLowerEntries'));begin=ins(method('BeginLowerIntro'));attempt=ins(method('LowerEntryAttempt'))
 assert {102826,362063,1600,1066,1}<=values(world)
 for target in ('?IsInWorld@Object@@','?IsPreDelete@Object@@','?GetScenario@ScenarioMgr@@'):assert sites(n,world,target)
 assert {40341,7,1,3}<=values(player)
 for target in ('?CanContact@Player@@','?InSamePhase@WorldObject@@','?canSeeOrDetect@WorldObject@@','?IsWithinLOSInMap@WorldObject@@','?IsWithinDistInMap@WorldObject@@','?GetQuestRewardStatus@Player@@','?GetQuestStatus@Player@@'):assert sites(n,player,target)
 assert {40341,1,3}<=values(attempt) and {0x18,0x20}<=fields(attempt)
 assert sites(n,attempt,'?getQuestStatus@Player@@') and sites(n,attempt,'?GetQuestRewardStatus@Player@@')
 for body in (accept,complete):
  assert sites(n,body,method('LowerEntryWorld')) and sites(n,body,method('LowerEntryPlayer'))
  assert sites(n,body,'?GetQuestObjectiveData@Player@@') and sites(n,body,method('BeginLowerIntro'))
  assert {40341,105805}<=values(body) and 20.0 in floats(n,body)
 assert {0xe0,0xe8,0xf0,0xf1}<=fields(accept) and 40 in values(accept)
 assert sites(n,accept,'?AddQuestDelayedEvent@Player@@') and sites(n,accept,method('CompleteLowerEntries'))
 assert not sites(n,accept,'?KilledMonsterCredit@Player@@')
 credit,=sites(n,complete,'?KilledMonsterCredit@Player@@')
 # Native KilledMonsterCredit sends generic achievement events before its
 # quest loop. Its known quest/raid denials must precede every retry call.
 for target in ('?GetQuestTemplate@QuestDataStoreMgr@@','?GetQuestStatus@Player@@','?isRaidGroup@Group@@','?IsAllowedInRaid@Quest@@'):
  assert any(i<credit for i in sites(n,complete,target))
 denied=complete[credit+1].address
 flag,=[i for i,x in enumerate(complete[:credit]) if x.mnemonic=='test' and x.op_str=='dword ptr [rsi + 0x404], 0x220']
 assert complete[flag+1].mnemonic=='je' and complete[flag+1].operands[0].imm==denied
 assert complete[flag-2].mnemonic=='cmp' and complete[flag-2].op_str=='eax, 3'
 assert complete[flag-1].mnemonic=='jne' and complete[flag-1].operands[0].imm==denied
 raid,=sites(n,complete,'?IsAllowedInRaid@Quest@@')
 assert complete[raid+2].mnemonic=='je' and complete[raid+2].operands[0].imm==denied
 assert {1000,30}<=values(complete) and {0xe0,0xe8,0xf0,0x30,0x38,0x40,0x44,0x48,0x49}<=fields(complete)
 assert sites(n,complete,'?GetPlayer@ObjectAccessor@@') and sites(n,complete,method('LowerEntryAttempt'))
 counts=sites(n,complete,'?GetQuestObjectiveData@Player@@');assert any(i<credit for i in counts) and any(i>credit for i in counts)
 assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rbx], 1' for x in complete[:credit])
 assert any(i>credit for i in sites(n,complete,method('BeginLowerIntro')))
 assert {40341,3000}<=values(begin) and {0x58,0x5a,0x60}<=fields(begin)
 assert sites(n,begin,'?AddQuestDelayedEvent@Player@@') and sites(n,begin,'?getQuestStatus@Player@@')
 schedule,=sites(n,begin,'?AddDelayedEvent@FunctionProcessor@@')
 assert any(x.op_str=='rcx, [rdi + 0x60]' for x in begin[schedule-4:schedule])
 callbacks=[name for name in n['symbols'] if name.startswith('?_Do_call@') and '<lambda_2>' in name and '??BeginLowerIntro@npc_erunak_stonespeakerAI@' in name]
 callback,=callbacks;body=ins(callback)
 for target in ('?GetPlayer@ObjectAccessor@@',method('LowerEntryWorld'),method('LowerEntryAttempt'),method('LowerEntryPlayer'),'?Talk@CreatureAI@@','?FindNearestCreature@WorldObject@@','?InSamePhase@WorldObject@@'):assert sites(n,body,target)
 assert {362064,362067,102792}<=values(body) and {0x10,0x20,0x58,0x5a,0x9c0}<=fields(body) and {70.0,100.0}<=floats(n,body)
 assert any(x.mnemonic=='test' and x.op_str=='rcx, rcx' for x in body)
 assert any(x.mnemonic=='jmp' and x.op_str=='qword ptr [rax + 0x10]' for x in body)
 for candidate in (accept,complete,begin,body):
  for forbidden in ('?AddItem@Player@@','?CompleteQuest@Player@@','?UpdateAchievementCriteria@Player@@','?AddDelayedEvent@Unit@@'):
   assert not sites(n,candidate,forbidden)
 for text in (b'Sharasdal lower Erunak entry reached forty pending original visitors',b'Sharasdal lower Erunak original visitor entry unavailable after thirty attempts'):assert text in n['binary']
 print('PASS actual lower362063/1600/1066/0..1/active40341/player admission, per-full-GUID weak quest attempts capped40, sole native105805 with actual objective acknowledgement before owned3s opening, busy/finite30/1s/presence-edge rearm, original caller/token callback and only actual two Brutes. Original wound/queue clock order retained; no manual quest/item/scenario grant or client/full1066 claim.')
