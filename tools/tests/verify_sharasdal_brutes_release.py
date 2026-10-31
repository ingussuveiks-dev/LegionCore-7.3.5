"""Verify real two-Brute credit, native conversation and recruitment ordering."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM,CS_OP_MEM

guard=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_guards_release.py')))
n=guard['n'];instructions=n['instructions'];sites=guard['sites'];values=guard['values'];suffix=guard['suffix']
def events(body):
 result=[]
 for site in sites(body,'?UpdateAchievementCriteria@Player@@'):
  assets=[x.operands[1].imm for x in body[max(0,site-12):site] if x.mnemonic=='mov' and x.op_str.startswith('r8d, ') and x.operands[1].type==CS_OP_IMM]
  assert assets
  result.append(assets[-1])
 return result
def field(body,offset,mnemonic=None):
 return [i for i,x in enumerate(body) if (mnemonic is None or x.mnemonic==mnemonic) and any(o.type==CS_OP_MEM and o.mem.disp==offset for o in x.operands)]

dispatch=instructions('?CreatureDies'+suffix)
assert 102792 in values(dispatch) and sites(dispatch,'?RecordBruteDeath'+suffix)
record=instructions('?RecordBruteDeath'+suffix)
assert {362064,362067,1066,40341,2}<=values(record)
assert sites(record,'?Recipient@SharasdalGuard@@') and sites(record,'?AddQuestDelayedEvent@Player@@') and sites(record,'?UpdateBrutes'+suffix)
assert field(record,0x388,'test') # actual per-spawn seen mask, before native delivery
update=instructions('?UpdateBrutes'+suffix)
assert set(events(update))=={49861} and 1<=len(events(update))<=2
assert {1066,40341,1000,30,2}<=values(update)
assert sites(update,'?Count@SharasdalBrutes@@') and sites(update,'?BeginBrutes'+suffix)
credit_site=sites(update,'?UpdateAchievementCriteria@Player@@')[0]
assert any(i>credit_site for i in sites(update,'?Count@SharasdalBrutes@@'))
assert field(update,0x389) and any(x.mnemonic=='mov' and x.op_str=='byte ptr [rsi], 1' for x in update[:credit_site])
assert 47161 in values(instructions('?Count@SharasdalBrutes@@'))
begin=instructions('?BeginBrutes'+suffix)
assert 5000 in values(begin) and field(begin,0x38a,'cmp') and field(begin,0x38a,'mov') and field(begin,0x38f,'movups') and field(begin,0x3d8,'mov')

presentation=instructions('?UpdateBrutePresentation'+suffix)
assert {105298,362066,102826,362063,1693,19509,1000,30}<=values(presentation)
assert sites(presentation,'?BruteActor'+suffix)
assert field(presentation,0x9c0) and field(presentation,0x9c8) # active and suspended consumed-menu guards
create_site,=sites(presentation,'?CreateConversation@Conversation@@')
after=presentation[create_site+1:create_site+17]
assert field(after,0x4dc,'mov') and field(after,0x3d8,'mov') and any(x.mnemonic=='cmovb' for x in after)
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rsp + 0x31], 1' for x in after)
assert field(instructions('?CreateConversation@Conversation@@'),0x4dc,'mov')
fresh=[i for i,x in enumerate(presentation) if x.mnemonic=='cmp' and x.op_str=='byte ptr [rsp + 0x31], 0']
assert fresh
wait_sub=[i for i,x in enumerate(presentation[:-1]) if x.mnemonic=='sub' and x.op_str=='eax, r14d' and
          presentation[i+1].mnemonic=='mov' and presentation[i+1].operands[0].type==CS_OP_MEM and presentation[i+1].operands[0].mem.disp==0x3d8]
assert wait_sub and fresh[0]<wait_sub[0]
assert presentation[fresh[0]+1].mnemonic in ('jne','jnz') and presentation[fresh[0]+1].operands[0].imm>presentation[wait_sub[0]].address
assert not events(presentation)
set_flag,=sites(presentation,'?SetFlag@Object@@')
assert any(x.mnemonic=='mov' and x.op_str=='r8d, 1' for x in presentation[max(0,set_flag-10):set_flag])
assert field(presentation,0x38d,'cmp') and field(presentation,0x38d,'mov')
actor=instructions('?BruteActor'+suffix)
assert sites(actor,'?GetCreature@Map@@') and field(actor,0x3a88)
spawn=instructions('?OnCreatureCreate'+suffix)
assert {105298,362066,102826,362063,38821}<=values(spawn)
outer=instructions('?Update'+suffix)
presentation_site,=sites(outer,'?UpdateBrutePresentation'+suffix)
delivery_site,=sites(outer,'?UpdateBrutes'+suffix)
assert presentation_site<delivery_site and sites(outer,'?UpdateGuard'+suffix)
assert struct.unpack_from('<Q',n['binary'],n['offset'](guard['table'])+0x78)[0] in {a for name,a in n['symbols'].items() if name.startswith('?OnUnitDeath'+suffix)}
assert instructions('?OnUnitDeath'+suffix)[0].mnemonic=='ret'
assert instructions('?EnterEvadeMode@npc_erunak_stonespeakerAI@npc_erunak_stonespeaker@@')[0].mnemonic=='ret'
erunak_table=guard['retained']['grash']['old']['previous']['selected_tables']['npc_erunak_stonespeaker']
# The previous canonical nonempty implementation uniquely occupied0x120.
# Empty handlers are folded together, so another ret slot cannot identify it.
assert struct.unpack_from('<Q',n['binary'],erunak_table+0x120)[0] in {a for name,a in n['symbols'].items() if name.startswith('?EnterEvadeMode@npc_erunak_stonespeakerAI@npc_erunak_stonespeaker@@')}

recruit=instructions('?Recruitable@SharasdalGrash@@')
assert {47121,47180,47161,2}<=values(recruit)
assert any(x.mnemonic=='cmp' and x.op_str=='qword ptr [rcx + 0x28], 2' for x in recruit)
for prefix in ('?OnGossipHello@npc_grash@@','?sGossipSelect@npc_grashAI@npc_grash@@'):
 assert sites(instructions(prefix),'?Recruitable@SharasdalGrash@@')
for body in (record,update,presentation):
 for prefix in ('?AddDelayedEvent@Unit@@','?AddItem@Player@@','?CompleteQuest@Player@@'):
  assert not sites(body,prefix)
assert not sites(presentation,'?SetUInt32Value@Object@@')
print('PASS selected native real102792 pair362064/362067 -> owned40341/seen/acknowledged49861, two separate native counter contributions, bounded retries and full GUID/quest attempt. Native1693 map-owned creation reads actual duration before fresh-clock guard and role-only flag; consumed current/suspended actor/phase guards, missing-actor retry and old-clock-before-new-receipt ordering remain. State-only death and Erunak evade are empty; real2/2 plus heal/intro gate both Hello/raw Select before original route. Guard/rescue/heal/other native paths retained. Actual client combat, audio, route and full1066 remain separate.')
