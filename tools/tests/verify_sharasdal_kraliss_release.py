"""Verify real Kraliss death dispatch and the original participant's criterion."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP,X86_REG_R14,X86_REG_R14D,X86_REG_R14W,X86_REG_R14B

retained=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_wall_release.py')))
n=retained['n'];instructions=n['instructions'];sites=retained['sites'];values=retained['values'];fields=retained['fields'];events=retained['events']
suffix='@instance_sharasdal_InstanceMapScript@instance_sharasdal@@'
factory=instructions('?GetAI@npc_kraliss@@');tables=[]
for x in factory:
 for operand in x.operands:
  if x.mnemonic=='lea' and operand.type==CS_OP_MEM and operand.mem.base==X86_REG_RIP:
   at=n['offset'](x.address+x.size+operand.mem.disp)
   if struct.unpack_from('<Q',n['binary'],at+0x18)[0]==n['symbol']('?UpdateAI@npc_kralissAI@npc_kraliss@@'):tables.append(at)
table,=tables
# Slot130 was derived from the actual unique NONEMPTY old callback in E74E3E7C,
# not by scanning a new empty-return alias. Native Kill still issues that slot.
assert struct.unpack_from('<Q',n['binary'],table+0x130)[0]==n['symbol']('?JustDied@npc_kralissAI@npc_kraliss@@')
assert instructions('?JustDied@npc_kralissAI@npc_kraliss@@')[0].mnemonic=='ret'
kill=instructions('?Kill@Unit@@')
death=[i for i,x in enumerate(kill) if x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0x130]
assert len(death)==1
at=death[0]
assert [(x.mnemonic,x.op_str) for x in kill[at-3:at]]==[
 ('mov','rcx, qword ptr [r15 + 0x9c0]'),('mov','rax, qword ptr [rcx]'),('mov','rdx, r13')]
dispatch=instructions('?CreatureDies'+suffix)
assert 102839 in values(dispatch) and sites(dispatch,'?RecordKralissDeath'+suffix)
record=instructions('?RecordKralissDeath'+suffix)
assert {102839,362080,1066,40341,2}<=values(record)
assert sites(record,'?Recipient@SharasdalKraliss@@') and sites(record,'?AddQuestDelayedEvent@Player@@') and sites(record,'?UpdateKraliss'+suffix)
owner_at,=sites(record,'?Recipient@SharasdalKraliss@@')
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rsi + 0x408], 1' for x in record[:owner_at])
assert any(x.mnemonic=='movups' and x.op_str=='xmmword ptr [rsi + 0x3e4], xmm0' for x in record)
recipient=instructions('?Recipient@SharasdalKraliss@@')
assert {104853,362078}<=values(recipient)
assert sites(recipient,'?Recipient@SharasdalGuard@@')
assert sites(record,'?IsInWorld@Object@@') and sites(record,'?IsPreDelete@Object@@')
assert sites(recipient,'?IsInSameGroupWith@Player@@')
tag_at,=sites(recipient,'?GetLootRecipient@Creature@@')
owner_at,=sites(recipient,'?Recipient@SharasdalGuard@@')
assert tag_at<owner_at and sites(kill,'?GetLootRecipient@Creature@@')
assert sites(instructions('?GetLootRecipient@Creature@@'),'?FindPlayer@ObjectAccessor@@')
for segment,register in ((recipient[tag_at+1:owner_at],'rbx'),(recipient[owner_at+1:],'rdi')):
 if not sites(segment,'?Eligible@SharasdalGuard@@'):
  # Both native loot and extra Adelee admission predicates are inlined here.
  assert sites(segment,'?IsInWorld@Object@@') and sites(segment,'?CanContact@Player@@')
  assert sites(segment,'?GetQuestRewardStatus@Player@@') and len(sites(segment,'?GetQuestStatus@Player@@'))==2
  class_at,=sites(segment,'?GetByteValue@Object@@')
  assert segment[class_at+1].mnemonic=='cmp' and segment[class_at+1].op_str=='al, 7'
  assert any(x.mnemonic=='test' and x.op_str==f'byte ptr [{register} + 0x4ff0], 1' for x in segment)
  for offset in (0x6848,0x6849):assert any(x.mnemonic=='cmp' and x.op_str==f'byte ptr [{register} + {offset:#x}], 0' for x in segment)
  assert 40341 in values(segment)
 assert sites(segment,'?InSamePhase@WorldObject@@') and sites(segment,'?IsWithinDistInMap@WorldObject@@')
assert any(x.mnemonic=='movss' and x.operands[1].type==CS_OP_MEM and x.operands[1].mem.base==X86_REG_RIP and struct.unpack_from('<f',n['binary'],n['offset'](x.address+x.size+x.operands[1].mem.disp))[0]==100.0 for x in recipient)
assert not sites(recipient,'?FindNearestPlayer@WorldObject@@')
receipt=instructions('?Receipt@SharasdalKraliss@@');assert 47330 in values(receipt)
update=instructions('?UpdateKraliss'+suffix)
assert events(update)==[50240] and {40341,1066,2,1000,30}<=values(update)
assert {0x3e4,0x3f8,0x400,0x409,0x40a,0x40b,0x40c,0x410}<=fields(update)
at,=sites(update,'?UpdateAchievementCriteria@Player@@')
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rdi], 1' for x in update[:at])
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rdi], 0' for x in update[at:])
assert sites(update,'?Eligible@SharasdalGuard@@')
assert any(i<at for i in sites(update,'?Receipt@SharasdalKraliss@@')) and any(i>at for i in sites(update,'?Receipt@SharasdalKraliss@@'))
assert any(x.mnemonic=='cmp' and x.op_str=='dword ptr [rsi + 0x410], 0x1e' for x in update)
enter=instructions('?OnPlayerEnter'+suffix)
rearm=[i for i,x in enumerate(enter) if x.mnemonic=='mov' and x.op_str=='byte ptr [rdi + 0x40b], r14b']
assert len(rearm)==1
at=rearm[0];zeros=[i for i,x in enumerate(enter[:at]) if x.mnemonic=='xor' and x.op_str=='r14d, r14d']
assert zeros and not any(set(x.regs_access()[1])&{X86_REG_R14,X86_REG_R14D,X86_REG_R14W,X86_REG_R14B} for x in enter[zeros[-1]+1:at])
assert any(x.mnemonic=='mov' and x.op_str=='qword ptr [rdi + 0x40c], r14' for x in enter)
# The retained guard inspector proves r14 zero before these unchanged entry paths.
tick=instructions('?Update'+suffix)
assert sites(tick,'?UpdateKraliss'+suffix)[0]<sites(tick,'?UpdateBrutePresentation'+suffix)[0]
for body in (record,recipient,update):
 for prefix in ('?AddItem@Player@@','?CompleteQuest@Player@@','?CreateConversation@Unit@@','?AddDelayedEvent@Unit@@'):
  assert not sites(body,prefix)
for message in (b'Sharasdal Kraliss death has no eligible original player/owner or known companion recipient',b'Sharasdal Kraliss victory cancelled after its original quest attempt ended',b'Sharasdal Kraliss victory could not reach its original recipient/native criterion; retry on re-entry'):
 assert message in n['binary']
print('PASS actual selected Kraliss JustDied130 empty/native Kill callback and instance CreatureDies40 route exact102839/362080, native loot recipient then owner/pet/guardian/charmer or known lower3 ally recipient, full40341 GUID/lease and early real victory retained. Sole50240 at native step2/47330 acknowledgement, busy/seen guard, thirty retries and original-player entry rearm; wall49817 remains separate. Prior actual wall/guard/Brute/wounds/rescue retained; combat/roar/Lady/loot/outro/client/full1066 remain separate')
