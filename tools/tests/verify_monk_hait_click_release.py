"""Selected actual registered native197120 Hight admission and receipt path."""
from pathlib import Path
import hashlib
import json
import runpy
from capstone import CS_OP_MEM,CS_OP_IMM
from sharasdal_erunak_entry_pe import sites, values, fields, floats

p=runpy.run_path(str(Path(__file__).with_name('verify_monk_hait_timers_release.py')))
n,refs,table,resolved=p['n'],p['refs'],p['table'],p['resolved']
ins=n['instructions']
def spell(name):return '?'+name+'@spell_monk_hait_click@@'
def helper(name):return '?'+name+'@MonkHaitClick@@'
def pairs(body):return [(op.mnemonic,op.op_str) for op in body]
def require(body,*targets):
 for target in targets:assert sites(n,body,target),target

loader,loader_raw=table('??_7?$GenericSpellScriptLoader@Vspell_monk_hait_click@@@@6B@',16)
script,script_raw=table('??_7spell_monk_hait_click@@6B@',8)
factory=ins('?GetSpellScript@?$GenericSpellScriptLoader@Vspell_monk_hait_click@@@@')
assert loader in refs(ins('?AddSC_monk_intro_scenario@@')) and script in refs(factory)
assert n['symbol']('?GetSpellScript@?$GenericSpellScriptLoader@Vspell_monk_hait_click@@@@') in resolved(loader_raw)
assert all(n['symbol'](spell(name)) in resolved(script_raw) for name in ('Load','Validate','Register'))
registered=ins(spell('Register'))
assert all(n['symbol'](spell(name)) in refs(registered) for name in ('Check','Event','Credit','After'))
assert {61,134}<=values(registered) and {0xa8,0x120,0x138,0x198}<=fields(registered)
validation=ins(spell('Validate'))
assert {197120,194020,61,46736,134,97778,90,98514,1}<=values(validation)
assert len(sites(n,validation,'?GetTarget@SpellImplicitTargetInfo@@'))==2
assert 1014 in values(ins(spell('Load')))

actor=ins('?GetAI@npc_master_hait@@')
assert {1014,362105,0x1000000}<=values(actor) and {0xb8,0xc0,0xc8,0xd0}<=fields(actor)
setflag,=sites(n,actor,'?SetFlag@Object@@')
assert [('cmp','dword ptr [rcx + 0xe0], 0x3f6'),('cmp','qword ptr [rcx + 0x3a88], 0x58679')]==[(op.mnemonic,op.op_str) for op in actor[:setflag] if op.mnemonic=='cmp']
assert ins('?OnSpellClick@npc_master_haitAI@')[0].mnemonic=='ret'
check=ins(spell('Check'));contact=ins(helper('Contact'));current=ins(helper('Current'))
require(check,'?GetOriginalTargetGUID@SpellScript@@','?GetCreature@Map@@',helper('Contact'),helper('Current'),'?getQuestStatus@Player@@')
assert sites(n,check,'?GetOriginalTargetGUID@SpellScript@@')[0]<sites(n,check,'?GetCreature@Map@@')[0]<sites(n,check,helper('Contact'))[0]
assert {12103,88}<=values(check) and {0x18,0xb8,0xc8,0xd0}<=fields(check)
assert len(sites(n,check,'??4?$weak_ptr@E@std@@'))==3,'Quest/actor reservations and actual per-cast actor ticket'
require(contact,'?CanContact@Player@@','?GetQuestStatus@Player@@','?GetQuestRewardStatus@Player@@','?FindQuestSlot@Player@@','?GetScenarioId@Scenario@@','?GetQuestTemplate@QuestDataStoreMgr@@','?isRaidGroup@Group@@','?IsAllowedInRaid@Quest@@','?InSamePhase@WorldObject@@','?canSeeOrDetect@WorldObject@@','?IsWithinDistInMap@WorldObject@@','?IsWithinLOSInMap@WorldObject@@')
assert {1014,943,12103,97778,98001,98011,362105,362111,362135,0x220}<=values(contact)
assert {5.0,40.0,90.0}<=floats(n,contact)
assert len(sites(n,contact,'?FindNearestCreature@WorldObject@@'))==2
assert ('mov','rcx, qword ptr [rdi + 0x9c0]') in pairs(contact)
require(current,'?getQuestStatus@Player@@','?GetCreature@Map@@',helper('Contact'))
assert {0x10,0x18,0x20,0x28,0x30,0x38,0x40,0xb8,0xc8,0xd0}<=fields(current)
assert sum(op.mnemonic=='lock cmpxchg' for op in current)>=3
for name in ('Event','Credit'):
 body=ins(spell(name));require(body,helper('Current'),'?PreventHitDefaultEffect@SpellScript@@')
 assert not sites(n,body,'?KilledMonsterCredit@Player@@')
require(ins(spell('Credit')),'?GetHitUnit@SpellScript@@')
after=ins(spell('After'));complete=ins(helper('Complete'))
opening_successor=any(name.startswith('?Accept@State@MonkHaitOpening@@') for name in n['symbols'])
require(after,helper('Current'),'?GetHitUnit@SpellScript@@','?GetReqKillOrCastCurrentCount@Player@@','?CastSpell@Unit@@',helper('Complete'),'?getMSTime@@','?outMessage@Log@@')
assert {12103,97778,98514,194020}<=values(after) and {0x1c,0x50,0x51,0x208,0x209}<=fields(after)
counts=sites(n,after,'?GetReqKillOrCastCurrentCount@Player@@');cast,=sites(n,after,'?CastSpell@Unit@@')
assert len(counts)==2 and max(counts)<cast<sites(n,after,helper('Complete'))[0]
require(complete,helper('Current'),'?getMSTime@@','?GetReqKillOrCastCurrentCount@Player@@','?AddQuestDelayedEvent@Player@@','?RemoveFlag@Object@@','?outMessage@Log@@')
assert {2000,12103,98514,10,0x1000000}<=values(complete)
assert [('sub','edx, ebx'),('lea','ecx, [rdx - 1]'),('cmp','ebx, eax'),('cmovbe','ecx, edx'),('cmp','ecx, 0x7d0')] in [pairs(complete)[i:i+5] for i in range(len(complete)-4)]
remove,=sites(n,complete,'?RemoveFlag@Object@@');count,=sites(n,complete,'?GetReqKillOrCastCurrentCount@Player@@')
assert count<remove
if opening_successor:
 require(complete,'?GetInstanceScript@WorldObject@@',helper('Current'))
 assert {94354,94355}<=values(complete)
 assert any(op.mnemonic=='mov' and op.operands[0].type==CS_OP_MEM and op.operands[0].mem.disp==0x50 and op.operands[1].type==CS_OP_IMM and op.operands[1].imm==1 for op in complete[count:remove])
 # MSVC retains SetGuidData's actual vtable slot in RBX across GetGUID.
 segment=pairs(complete[count:remove])
 load=segment.index(('mov','rbx, qword ptr [rax + 0x98]'))
 call=segment.index(('call','rbx'))
 assert load<call and ('mov','edx, 0x17092') in segment[load:call]
 assert any(op.mnemonic=='call' and op.operands[0].type==CS_OP_MEM and op.operands[0].mem.disp==0x90 for op in complete[count:remove])
 assert len(sites(n,complete,helper('Current')))==2
 assert not any(op.mnemonic=='call' and op.operands[0].type==CS_OP_MEM and op.operands[0].mem.disp==0x48 for op in complete[remove+1:])
else:
 assert ('mov','byte ptr [rax + 0x50], 1') in pairs(complete[count:remove])
 assert any(op.mnemonic=='call' and op.operands[0].type==CS_OP_MEM and op.operands[0].mem.disp==0x48 for op in complete[remove+1:]),'Original AI DoAction after consuming actual receipt'
callback,=[name for name in n['symbols'] if name.startswith('?_Do_call@') and '??Complete@MonkHaitClick@@' in name]
require(ins(callback),helper('Complete'))
assert b'Monk intro: native194020 rejected (%u); retain Hight click for retry' in n['binary']
assert b'Monk intro: native194020 produced no timely98514 receipt; retain Hight click for retry' in n['binary']
for body in (check,contact,current,after,complete,ins(callback)):
 for forbidden in ('?KilledMonsterCredit@Player@@','?RewardPlayerAndGroupAtEvent@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@'):
  assert not sites(n,body,forbidden)
require(ins('?EffectKillCredit@Spell@@'),'?RewardPlayerAndGroupAtEvent@Player@@')
require(ins('?EffectKillCreditPersonal@Spell@@'),'?KilledMonsterCredit@Player@@')
require(ins('?EffectSendEvent@Spell@@'),'?UpdateAchievementCriteria@Player@@')
transfer='instance acceptance' if opening_successor else 'native DoAction'
print(f'PASS actual197120 registered factory/four hooks/native61+134+90 validation; real original-target admission/initial spawn943/0/native raid policy, weak quest+actor+ticket, receipts97778/98514, native194020 result, exact elapsed2s/10ms quest-owned retry and consume-before-flag/{transfer}. No synthetic credit/item/quest/teleport calls; retained NPC clocks/cleanup verified; client/full intro remains separate.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'registered_hooks':4,'spell_id':197120}))
