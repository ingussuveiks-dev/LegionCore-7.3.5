"""Selected x64 proofs for the original lower-ascent route and recipient."""
import re
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values, fields, floats, method as erunak

SUFFIX='@instance_sharasdal_InstanceMapScript@instance_sharasdal@@'
def method(name):return '?'+name+SUFFIX

def verify_movement(n):
 body=n['instructions'](erunak('MovementInform'))
 assert {5,102826,362063,10993810,50336,1707}<=values(body)
 assert sites(n,body,'?Path@SharasdalWall@@')
 for target in ('?FindNearestPlayer@WorldObject@@','?CastSpell@Unit@@','?UpdateAchievementCriteria@Player@@'):
  assert not sites(n,body,target),target
 at,=[i for i,x in enumerate(body) if x.mnemonic=='call' and x.op_str=='rbx']
 assert any(x.op_str=='rbx, qword ptr [rax + 0x98]' for x in body[:at])
 assert [(x.mnemonic,x.op_str) for x in body[at-3:at]]==[
  ('lea','r8, [rsp + 0x30]'),('mov','edx, 0xc4a0'),('mov','rcx, r14')]
 assert any(x.op_str=='xmmword ptr [rsp + 0x30], xmm0' for x in body[at-5:at])

def verify(n):
 ins=n['instructions'];verify_movement(n)
 world=ins(method('AscentWorld'));actor=ins(method('AscentCreature'));quest=ins(method('AscentQuest'))
 player=ins(method('AscentPlayer'));attempt=ins(method('AscentSameAttempt'));path=ins(method('AscentPath'))
 pose=ins(method('AscentEndPose'));bind=ins(method('SetGuidData'));ack=ins(method('GetGuidData'))
 update=ins(method('UpdateAscent'));entry=ins(method('OnPlayerEnter'));tick=ins(method('Update'))
 assert {1600,1066}<=values(world) and sites(n,world,'?GetScenario@ScenarioMgr@@')
 for target in ('?GetCreature@Map@@','?IsInWorld@Object@@','?IsPreDelete@Object@@'):assert sites(n,actor,target)
 assert 0xa18 in fields(actor) # native ALIVE0
 for body in (quest,player,attempt):
  assert {40341,105805,7,1,3}<=values(body)
  for target in ('?GetQuestRewardStatus@Player@@','?GetQuestStatus@Player@@','?GetQuestObjectiveData@Player@@'):assert sites(n,body,target)
 for target in ('?CanContact@Player@@','?InSamePhase@WorldObject@@','?canSeeOrDetect@WorldObject@@','?IsWithinLOSInMap@WorldObject@@','?IsWithinDistInMap@WorldObject@@'):assert sites(n,player,target)
 assert {0x498,0x4a0,0x18,0x20}<=fields(attempt) and sites(n,attempt,'?getQuestStatus@Player@@')
 assert sites(n,path,'?GetMotionSlot@MotionMaster@@') and {10993810,2}<=values(path) and 0x2c in fields(path)
 assert {10993810}<=values(pose) and {36.0,1.0}<=floats(n,pose)
 assert sites(n,pose,'?instance@WaypointMgr@@') and 0x8c8 in fields(pose)
 # Inlined native size()==6 is end-begin-264 <44, with 44-byte nodes.
 assert any(x.op_str=='rax, 0x108' and x.mnemonic=='sub' for x in pose)
 assert any(x.op_str=='rax, 0x2c' and x.mnemonic=='cmp' for x in pose)
 assert any(x.op_str=='byte ptr [rax + 0x68], 0x20' and x.mnemonic=='test' for x in pose)
 assert {208580,10993810,50336,105298,362066,102826,362063,40341,2,3,5}<=values(bind)
 for target in (method('AscentWorld'),method('AscentCreature'),method('AscentPlayer'),method('AscentSameAttempt'),method('AscentPath'),method('AscentEndPose'),'?FindPlayer@ObjectAccessor@@','?AddQuestDelayedEvent@Player@@'):
  assert sites(n,bind,target),target
 assert 5.0 in floats(n,bind)
 for expected in ('xmmword ptr [rbx + 0x474], xmm0','xmmword ptr [rbx + 0x484], xmm0','byte ptr [rbx + 0x4a9], 1','byte ptr [rbx + 0x4ab], 1'):
  assert any(x.op_str==expected for x in bind),expected
 assert {208580}<=values(ack) and {0x474,0x4b0}<=fields(ack)
 # The real factory's registered instance table selects both message methods
 # and this Update. The retained guard proof follows its constructor install.
 pattern=re.compile(r'\s+\w+:\w+\s+\?\?_7instance_sharasdal_InstanceMapScript@instance_sharasdal@@6B@\s+([0-9a-fA-F]{16})')
 table,=[int(m[1],16) for line in (n['args'].release/'worldserver.map').read_text(errors='replace').splitlines() if (m:=pattern.match(line))]
 rows=struct.unpack_from('<128Q',n['binary'],n['offset'](table))
 assert n['resolve'](rows[0x98//8])==n['symbol'](method('SetGuidData'))
 assert n['resolve'](rows[0x90//8])==n['symbol'](method('GetGuidData'))
 assert n['symbol'](method('Update')) in rows
 calls=[(i,name) for name in ('UpdateAscent','UpdateLady','UpdateKraliss','UpdateBrutePresentation','UpdateBrutes','UpdateGuard') for i in sites(n,tick,method(name))]
 assert [name for i,name in sorted(calls)]==['UpdateAscent','UpdateLady','UpdateKraliss','UpdateBrutePresentation','UpdateBrutes','UpdateGuard']
 assert {47131,2,3,5,40341,83810,208580,1000,30}<=values(update) and 100.0 in floats(n,update)
 assert {0x474,0x484,0x498,0x4a0,0x4a8,0x4a9,0x4aa,0x4ab,0x4ac,0x4ad,0x4ae,0x4af,0x4b1,0x4b2,0x4b4,0x4b8,0x3b1c}<=fields(update)
 for target in (method('AscentCreature'),method('AscentPath'),method('AscentEndPose'),method('AscentQuest'),method('AscentSameAttempt'),method('AscentPlayer'),'?FindPlayer@ObjectAccessor@@','?AddQuestDelayedEvent@Player@@','?HasAura@Unit@@'):
  assert sites(n,update,target),target
 # The polled native Creature waypoint marker is checked after actual issue;
 # only a real observed middle node permits the removed-generator fallback.
 assert any(x.op_str=='dword ptr [r14 + 0x3b1c], 5' for x in update)
 assert any(x.op_str=='byte ptr [rdi + 0x4aa], 1' for x in update)
 casts=sites(n,update,'?CastSpell@Unit@@');assert len(casts)==2
 for at,receiver,spell in zip(casts,('r14','r15'),(83810,208580)):
  assert [(x.mnemonic,x.op_str) for x in update[at-4:at]]==[
   ('xor','r9d, r9d'),('mov',f'r8d, {spell:#x}'),('mov',f'rdx, {receiver}'),('mov',f'rcx, {receiver}')]
 camera=casts[1];after=sites(n,update,method('AscentSameAttempt'))[-1];aura=sites(n,update,'?HasAura@Unit@@')[-1]
 assert camera<after<aura
 assert any(x.op_str=='r15d, r15d' and x.mnemonic=='test' for x in update[after:aura])
 assert any(x.op_str=='byte ptr [rdi + 0x4af], 1' for x in update[aura:])
 # Inlined actual original-GUID map-entry recovery, with finite counter reset.
 assert {0x474,0x4a8,0x4ad,0x4b1,0x4b4}<=fields(entry)
 assert any(x.op_str=='word ptr [rbx + 0x4b1], 0x101' for x in entry)
 native=ins('?OnArrived@?$WaypointMovementGenerator@VCreature@@@@')
 assert any(x.mnemonic=='mov' and '0x3b1c]' in x.op_str for x in native)
 select=ins('?sGossipSelect@npc_grashAI@npc_grash@@')
 get,=[i for i,x in enumerate(select) if x.mnemonic=='call' and x.op_str=='qword ptr [rax + 0x90]']
 assert select[get-3].op_str=='r8d, 0x32ec4'
 assert any(x.op_str=='rdi, qword ptr [rcx + 0x98]' for x in select[:get])
 close,=sites(n,select,'?SendCloseGossip@PlayerMenu@@') if sites(n,select,'?SendCloseGossip@PlayerMenu@@') else sites(n,select,'?ClearMenus@PlayerMenu@@')
 assert get<close
 callbacks=[name for name in n['symbols'] if name.startswith('?_Do_call@') and '??DoAction@npc_grashAI@' in name]
 for route,line in ((10993808,2),(10993810,9)):
  speech,=[ins(name) for name in callbacks if route in values(ins(name))]
  talk,=sites(n,speech,'?Talk@CreatureAI@@')
  load,=[i for i,x in enumerate(speech[:talk]) if x.op_str=='rcx, qword ptr [rax + 0x9c8]']
  # One spill may intervene between the original load and its null test.
  checks=[i for i,x in enumerate(speech[load+1:talk],load+1) if x.op_str=='rcx, rcx' and x.mnemonic=='test']
  assert len(checks)==2 and speech[checks[0]+1].mnemonic=='jne' and speech[checks[1]+1].mnemonic=='je'
  assert any(x.op_str=='rcx, qword ptr [rax + 0x9c0]' for x in speech[checks[0]:checks[1]])
  assert speech[checks[1]+1].operands[0].imm>speech[talk].address
  assert any(x.op_str==f'dl, {line}' for x in speech[checks[1]+1:talk])
 bridge,=[ins(name) for name in callbacks if 10993810 in values(ins(name))]
 moves=sites(n,bridge,'?MovePath@MotionMaster@@');assert len(moves)==3
 issue,=[i for i,x in enumerate(bridge) if x.mnemonic=='call' and x.op_str=='rbx']
 assert moves[1]<issue<moves[2] and any(x.op_str=='edx, 0xa7c092' for x in bridge[issue-4:issue])
 for body in (world,actor,quest,player,attempt,path,pose,bind,ack,update,bridge):
  for forbidden in ('?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@','?FindNearestPlayer@WorldObject@@'):
   assert not sites(n,body,forbidden),forbidden
 assert b'Sharasdal earned lower ascent cannot reach its original quest participant or native camera after thirty attempts' in n['binary']
 print('PASS selected registered instance98/90 messages, original full GUID/40341+105805 witness and weak attempt; real installed10993810/node5/native marker/final pose, retained middle-node fallback and original map-entry recovery; UpdateAscent first, native3+unreceived47131, finite30 ordinary self208580 with aura acknowledgement. Actual Grash raw gossip acknowledgement precedes consumption, real route precedes issue, Erunak final callback only records arrival. No nearest-player/manual progression/teleport/item path. Native scene/client traversal remain separate.')
