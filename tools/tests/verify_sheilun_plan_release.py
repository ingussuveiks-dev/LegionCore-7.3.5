"""Verify registered native menu/index consumers and real plan receipts."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values, fields, floats

retained=runpy.run_path(str(Path(__file__).with_name('verify_sheilun_taran_presentation_release.py')))
n=retained['n'];ins=n['instructions']
def helper(name):return '?'+name+'@SheilunPlan@@'
def method(role,name):return '?'+name+'@'+role+'AI@'+role+'@@'
def refs(body):
 return [x.address+x.size+o.mem.disp for x in body if x.mnemonic=='lea'
         for o in x.operands if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP]
packet=ins('?HandleGossipSelectOption@WorldSession@@')
assert any(x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0xc8 for x in packet)
assert sites(n,packet,'?OnGossipSelect@ScriptMgr@@')
for role,target in [('npc_tazhan_chu','Start'),('npc_sheylun_small_events','Select')]:
 factory=ins('?GetAI@'+role+'@@')
 tables=[at for at in refs(factory) if n['resolve'](struct.unpack_from('<Q',n['binary'],n['offset'](at+0xc8))[0])==n['symbol'](method(role,'sGossipSelect'))]
 assert len(tables)==1
 body=ins(method(role,'sGossipSelect'))
 assert sites(n,body,helper(target))==[3]
 assert [(x.mnemonic,x.op_str) for x in body[:3]]==[
  ('mov','rax, rdx'),('mov','rdx, qword ptr [rcx + 0x20]'),('mov','rcx, rax')]
 legacy=ins('?OnGossipSelect@'+role+'@@')
 assert [(x.mnemonic,x.op_str) for x in legacy[:2]]==[('mov','al, 1'),('ret','')]
action=ins(method('npc_tazhan_chu','DoAction'))
assert action[0].op_str=='edx, 1' and sites(n,action,helper('Refresh'))==[3]
assert not any(x.mnemonic in ('inc','add') for x in action)
life=ins(method('npc_tazhan_chu','UpdateLifecycle'))
assert sites(n,life,'?RemoveAura@Unit@@')[0]<sites(n,life,helper('Refresh'))[0]<sites(n,life,'?Update@FunctionProcessor@@')[0]

world=ins(helper('World'));player=ins(helper('PlayerContext'));menu=ins(helper('Menu'));receipt=ins(helper('Receipt'))
assert {1541,1007}<=values(world) and {0xa18,0x468}<=fields(world)
for target in ('?IsInWorld@Object@@','?IsPreDelete@Object@@','?GetScenario@ScenarioMgr@@','?GetScenarioId@Scenario@@'):assert sites(n,world,target)
assert {41003,10}<=values(player) and 5.0 in floats(n,player)
for target in ('?CanContact@Player@@','?InSamePhase@WorldObject@@','?canSeeOrDetect@WorldObject@@','?IsWithinLOSInMap@WorldObject@@','?IsWithinDistInMap@WorldObject@@','?GetQuestRewardStatus@Player@@','?GetQuestStatus@Player@@'):assert sites(n,player,target),target
assert sites(n,menu,'?GetMenuId@GossipMenu@@')
assert {0x28,0x2c,0x30,0x34,0x78}<=fields(menu)
assert menu[10].op_str=='edx, r9d' and menu[12].op_str=='r8d, dword ptr [rsp + 0x50]'
assert menu[11].mnemonic=='jne' and menu[13].mnemonic=='ja'
assert sites(n,receipt,'?GetCriteriaProgressMap@')
assert any(x.mnemonic=='cmp' and x.op_str=='qword ptr [rax + 0x28], 1' for x in receipt)
planned=ins(helper('Planned'));assert {46636,46637,46638}<=values(planned)
assert len(sites(n,planned,helper('Receipt')))==3

select=ins(helper('Select'));start=ins(helper('Start'));refresh=ins(helper('Refresh'))
rows=[(101882,362835,19225,48446,46636),(101882,362865,19225,48446,46636),
      (101883,362834,19226,48447,46637),(101883,362869,19226,48447,46637),
      (101885,362831,19227,48448,46638),(101885,362867,19227,48448,46638)]
payload=b''.join(struct.pack('<5I',*row) for row in rows)
tables=[at for at in refs(select) if n['binary'][n['offset'](at):n['offset'](at)+len(payload)]==payload]
table,=tables
assert table+len(payload) in refs(select), 'Actual six-record native loop end not referenced'
assert any(x.mnemonic=='add' and x.op_str=='rbx, 0x14' for x in select)
for body,is_start in [(select,False),(start,True)]:
 credit,=sites(n,body,'?UpdateAchievementCriteria@Player@@')
 checks=sites(n,body,helper('Receipt'))
 # The independently native-tested acquisition observer is inlined after
 # the two original acknowledgements. Certify its extra receipt explicitly;
 # retain every original admission/owner/menu/movement check below.
 observed=is_start and len(checks)==3
 assert len(checks)==(3 if observed else 2)
 found,=sites(n,body,'?FindPlayer@ObjectAccessor@@');actor,=sites(n,body,'?GetCreature@Map@@')
 remove,=sites(n,body,'?RemoveFlag@Object@@')
 assert checks[0]<credit<found<actor<checks[1]<remove
 if observed:
  bind,=sites(n,body,'?Bind@Instance@SheilunAcquisition@@')
  progress=sites(n,body,'?Progress@Instance@SheilunAcquisition@@')
  assert len(progress)==2 and checks[1]<bind<progress[0]<progress[1]<checks[2]<remove
  assert sites(n,body,'?Lease@Instance@SheilunAcquisition@@')
  assert body[checks[2]+1].op_str=='al, al' and body[checks[2]+2].mnemonic=='je'
  assert body[checks[2]+3].mnemonic=='mov' and body[checks[2]+3].op_str.startswith('byte ptr [') and body[checks[2]+3].op_str.endswith(', 1')
 assert len(sites(n,body,helper('PlayerContext')))==2
 assert len(sites(n,body,'?getQuestStatus@Player@@'))==2
 assert sites(n,body,'?AddQuestDelayedEvent@Player@@')[0]<credit
 assert sites(n,body,helper('Menu'))[0]<credit
 clear,=sites(n,body,'?ClearMenus@PlayerMenu@@');close,=sites(n,body,'?SendCloseGossip@PlayerMenu@@')
 assert sites(n,body,helper('Menu'))[0]<clear<close<credit
 if is_start:
  assert {19235,101881,362837,48494,46662}<=values(body)
  assert len(sites(n,body,helper('Planned')))==2
  assert remove<min(sites(n,body,'?MovePoint@MotionMaster@@'))
  assert {-1019.15,-2795.95,38.91}<=floats(n,body)
  assert {101882,101885,101883,246464,60958}<=values(body)
  assert sites(n,body,'?Talk@CreatureAI@@') and sites(n,body,'?CastSpell@Unit@@')
 else:
  tactic=sites(n,body,helper('ApplyTactic'))
  owner='rsi' if tactic else 'r14'
  assert select[credit-3].op_str==f'r8d, dword ptr [{owner} + 0xc]' and select[credit-2].op_str=='edx, 0x5c'
  if tactic:
   accepted,=tactic;assert accepted<credit
   assert any(x.mnemonic=='je' and x.operands[0].imm>body[credit].address for x in body[accepted:credit])
  assert sites(n,body,helper('Refresh'))[0]>remove
 for forbidden in ('?AddItem@Player@@','?CompleteQuest@Player@@','?KilledMonsterCredit@Player@@','?AddDelayedEvent@Unit@@'):
  assert not sites(n,body,forbidden)
assert {101881,362837,3,46662}<=values(refresh)
assert sites(n,refresh,helper('Planned')) and sites(n,refresh,helper('Receipt'))
assert sites(n,refresh,'?SetFlag@Object@@') and not sites(n,refresh,'?UpdateAchievementCriteria@Player@@')
print('PASS actual two registered AI gossip slotsC8/native packet dispatch and inert legacy scripts; exact six matching source records/20-byte stride/120-byte loop, menu+index+item admission, real quest lease/original player+actor re-resolution, native role receipt before flag cleanup, all three plan receipts and accepted48494 before original point0/speech60958/wall; lifecycle readiness recovery without progression. Tactical combat effects/full client/pickup/return remain separate.')
