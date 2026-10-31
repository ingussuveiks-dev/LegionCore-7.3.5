"""Selected x64 upper Erunak ownership, presentation and re-entry checks."""
from sharasdal_erunak_entry_pe import method,sites,values,fields,floats
from capstone import CS_OP_MEM

def verify_lifecycle(n):
 life=n['instructions'](method('UpdateLifecycle'))
 for prefix,at in [('?Recover@SharasdalCompanion@@',8),('?Update@FunctionProcessor@@',11),
                   (method('CompleteLowerEntries'),14),(method('CompleteUpperIntro'),17),
                   (method('ProbeErunakArrival'),22)]:
  assert sites(n,life,prefix)==[at], prefix
 assert life[22].mnemonic=='jmp'
 assert [(x.mnemonic,x.op_str) for x in life[3:8]]==[
  ('mov','ebx, edx'),('lea','r8, [rcx + 0x59]'),('lea','rdx, [rcx + 0x5b]'),
  ('mov','rdi, rcx'),('mov','rcx, qword ptr [rcx + 0x20]')]
 assert life[9].op_str=='rcx, [rdi + 0x60]'
 for at in (10,12,15):assert life[at].op_str=='edx, ebx'
 for at in (13,16,18):assert life[at].op_str=='rcx, rdi'

def verify(n):
 verify_lifecycle(n)
 ins=n['instructions'];world=ins(method('UpperIntroWorld'));actor=ins(method('UpperIntroActor'))
 accept=ins(method('AcceptUpperIntro'));complete=ins(method('CompleteUpperIntro'))
 cancel=ins(method('CancelUpperIntro'));probe=ins(method('ProbeErunakArrival'));rearm=ins(method('SetGUID'))
 assert {102826,362093,1600,1066}<=values(world)
 for target in ('?IsInWorld@Object@@','?IsPreDelete@Object@@','?GetScenario@ScenarioMgr@@'):assert sites(n,world,target)
 for target in ('?IsInWorld@Object@@','?IsPreDelete@Object@@','?InSamePhase@WorldObject@@'):assert sites(n,actor,target)
 assert sites(n,accept,method('UpperIntroWorld')) and sites(n,accept,method('LowerEntryPlayer'))
 assert {40341,3}<=values(accept) and 20.0 in floats(n,accept)
 for target in ('?AddQuestDelayedEvent@Player@@','?getQuestStatus@Player@@',method('CompleteUpperIntro')):assert sites(n,accept,target)
 assert any(x.op_str=='xmmword ptr [rbx + 0xf2], xmm0' for x in accept)
 assert {0x108,0x110,0x13a,0x13c}<=fields(accept)
 # MSVC combines adjacent byte/counter clears; prove their actual widths.
 for expected in ('word ptr [rcx + 0x13e], 0','qword ptr [rcx + 0x144], rdi',
                  'dword ptr [rcx + 0x140], edi','byte ptr [rcx + 0x13d], 0',
                  'word ptr [rcx + 0x13a], 0','qword ptr [rbx + 0x108], rdi',
                  'qword ptr [rbx + 0x110], rdi'):
  assert any(x.mnemonic=='mov' and x.op_str==expected for x in cancel),expected
 assert any(x.mnemonic=='add' and x.op_str=='rcx, 0xf2' for x in cancel)
 assert not {0x138,0x139,0x13c}&fields(cancel) # acknowledged paths/talks and RAII busy survive cancellation
 for target in (method('UpperIntroWorld'),method('LowerEntryAttempt'),method('LowerEntryPlayer'),
                method('UpperIntroActor'),'?Receipt@SharasdalGrash@@','?GetPlayer@ObjectAccessor@@',
                '?GetSpellInfo@SpellMgr@@','?MovePath@MotionMaster@@','?GetMotionSlot@MotionMaster@@'):
  assert sites(n,complete,target),target
 assert {3,4,47131,362092,362094,105298,104853,10993811,5000,1000,30,209312,1868}<=values(complete)
 assert {20.0,30.0,100.0}<=floats(n,complete)
 required={0xf2,0x108,0x118,0x128,0x138,0x139,0x13a,0x13b,0x13c,0x13d,0x13e,0x13f,0x140,0x144,0x148}
 assert required<=fields(complete),sorted(required-fields(complete))
 create,=sites(n,complete,'?CreateConversation@Conversation@@')
 # Native bool factory receives the same original-player register later
 # reloaded by full recipient GUID and used as BindQuestObject's receiver.
 assert [(x.mnemonic,x.op_str) for x in complete[create-4:create]]==[
  ('mov','r9, r12'),('mov','r8d, 0x74c'),('mov','rdx, rax'),('mov','rcx, r15')]
 assert complete[create+1].op_str=='al, al' and complete[create+2].mnemonic=='je'
 speakers=sites(n,complete,'?GetCreature@Conversation@@');assert len(speakers)==2 and create<speakers[0]<speakers[1]
 assert complete[speakers[0]-2].op_str=='edx, edx' and complete[speakers[1]-2].op_str=='dl, 1'
 bind,=sites(n,complete,'?BindQuestObject@Player@@')
 assert [(x.mnemonic,x.op_str) for x in complete[bind-3:bind]]==[
  ('mov','r8, r15'),('mov','edx, 0x9d95'),('mov','rcx, r12')]
 assert complete[bind+1].op_str=='byte ptr [rsi + 0x58], 1'
 assert any(speakers[-1]<i<bind for i in sites(n,complete,'?GetPlayer@ObjectAccessor@@'))
 for prefix in (method('LowerEntryAttempt'),method('LowerEntryPlayer')):
  assert any(speakers[-1]<i<bind for i in sites(n,complete,prefix))
 assert sites(n,complete,'?Remove@Conversation@@')
 # Two complete 128-bit speaker identities are compared before binding.
 compare_region=complete[speakers[-1]:bind]
 assert len(sites(n,compare_region,'?GetLowPart@ObjectGuid@@'))==2
 assert len(sites(n,compare_region,'?GetHighPart@ObjectGuid@@'))==2
 comparisons=[i for i,x in enumerate(compare_region) if x.mnemonic=='cmp' and x.op_str=='rdi, rax']
 assert len(comparisons)==4
 assert all(compare_region[i+1].mnemonic=='jne' for i in comparisons)
 assert len({compare_region[i+1].operands[0].imm for i in comparisons})==1
 for prefix in (method('LowerEntryWorld'),method('UpperIntroWorld'),method('LowerEntryPlayer'),method('AcceptLowerEntry'),method('AcceptUpperIntro')):
  assert sites(n,probe,prefix)
 assert 20.0 in floats(n,probe)
 assert {40341}<=values(rearm) and {0xe0,0xf2,0x13a,0x13d,0x140}<=fields(rearm)
 assert sites(n,rearm,'??8ObjectGuid@@')
 entry=ins('?OnPlayerEnter@instance_sharasdal_InstanceMapScript@instance_sharasdal@@')
 assert {0x3af,0x464,0x9c0,0x9c8}<=fields(entry) and {102826,362063,362093,40341}<=values(entry)
 dispatch=[i for i,x in enumerate(entry) if x.mnemonic=='call' and x.op_str=='qword ptr [rax + 0x80]']
 assert len(dispatch)==2
 current=entry[dispatch[0]-1].op_str.split(', ')[1]
 suspended=entry[dispatch[1]-1].op_str.split(', ')[1]
 assert current=='rsi' and suspended in ('rbx','rdi')
 for at,receiver in zip(dispatch,(current,suspended)):
  assert [(x.mnemonic,x.op_str) for x in entry[at-3:at]]==[
   ('mov','r8d, 0x9d95'),('mov','rdx, r14'),('mov',f'rcx, {receiver}')]
 assert any(x.mnemonic=='cmp' and x.op_str==f'{suspended}, {current}' for x in entry[dispatch[0]:dispatch[1]])
 created=ins('?OnCreatureCreate@instance_sharasdal_InstanceMapScript@instance_sharasdal@@')
 assert {102826,362093}<=values(created) and 0x464 in fields(created)
 los=ins(method('MoveInLineOfSight'))
 lower,=[x for x in los if x.mnemonic=='je' and x.operands[0].imm==n['symbol'](method('AcceptLowerEntry'))]
 assert lower.address>los[5].address and los[5].op_str=='qword ptr [rax + 0x3a88], 0x5864f'
 assert sites(n,los,method('AcceptUpperIntro'))
 for body in (accept,complete,cancel,probe,rearm,los):
  for forbidden in ('?AddItem@Player@@','?CompleteQuest@Player@@','?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@',
                    '?AddDelayedEvent@Unit@@','?CreateConversation@Unit@@','?FindNearestPlayer@WorldObject@@'):
   assert not sites(n,body,forbidden),forbidden
 assert b'Sharasdal upper Erunak original ascent/companions/path or conversation unavailable after thirty attempts' in n['binary']
 print('PASS selected upper362093/native3/real47131/player40341 full-GUID lease, exact companions/acknowledged10993811 paths, owned5s numeric delay and finite30 retries; bool1868 factory uses original Player, two native full-GUID speakers acknowledged before quest-scope bind/latch. Actual lower+upper instance re-entry calls current/distinct suspended SetGUID40341; clocks precede arrival probes. No progression/item grant or actual client traversal claim.')
