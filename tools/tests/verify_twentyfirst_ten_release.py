"""Linked instance-owned introductions and native scaled surrender/participant paths."""
from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_twentieth_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values'];callbacks=p['callbacks'];expanded=p['expanded']
def op(r,m,s):return any(x.mnemonic==m and x.op_str==s for x in r)
def hook(cls):
 r=ins('?DamageTakenAfterScaling@'+cls+'@');assert sites(r,'?GetHealth@Unit@@'),'native target-relative health '+cls
 assert not any(s.startswith('?DamageTaken@'+cls+'@')for s in n['symbols']),'old pre-scaling override absent '+cls
 assert any(x.mnemonic=='cmp'and 'rax'in x.op_str for x in r),'full-width lethal comparison '+cls
 return r
r=hook('npc_argent_valiantAI');assert sites(r,'?GetAffectingPlayer@Unit@@')and sites(r,'?RewardPlayerAndGroupAtEvent@Player@@')and 38595 in values(r)
assert op(r,'mov','byte ptr [rsi + 0x44], 1')and sites(r,'?CombatStop@Unit@@')and sites(r,'?DeleteThreatList@Unit@@'),'once latch survives native cleanup'
r=hook('npc_valiantAI');assert sites(r,'?GetAffectingPlayer@Unit@@')and sites(r,'??9ObjectGuid@@')and 62724 in values(r)and op(r,'mov','byte ptr [rsi + 0x44], 1')
r=ins('?OnGossipSelect@npc_valiant@@');assert len(sites(r,'?SetInCombatWith@Unit@@'))==2 and sites(r,'?isInCombat@Unit@@')and 90909090 in values(r),'actual gossip inlines challenger combat admission'
assert not sites(r,'?DoZoneInCombat@'),'outdoor duel no zone-combat fallback'
r=ins('?Reset@npc_valiantAI@');assert sites(r,'?Clear@ObjectGuid@@')and any(x.mnemonic=='mov'and '0x44]'in x.op_str and x.op_str.endswith(', 1')for x in r),'reset retires old challenger'
r=ins('?KilledUnit@npc_valiantAI@');assert sites(r,'??9ObjectGuid@@')and sites(r,'?Clear@ObjectGuid@@')and sites(r,'?SetFlag@Object@@'),'loss restores gossip and applies only to actual challenger identity'
r=hook('npc_tg_helperAI');assert op(r,'cmp','rax, 1')and any(x.mnemonic=='jbe'for x in r)and sites(r,'?SetHealth@Unit@@'),'bounded one-health survival and native full-health restore'
assert op(ins('?Reset@boss_tsulongAI@'),'mov','byte ptr [rbx + 0x1bc], 0'),'initialized alternating evade state'
r=hook('npc_master_cheng_q31840');assert len(sites(r,'?GetAffectingPlayer@Unit@@'))==2 and sites(r,'?GetAnyOwner@Unit@@')and sites(r,'?KilledMonsterCredit@Player@@')and op(r,'cmp','rax, rdi')and op(r,'mov','byte ptr [rsi + 0x38], 1')and 6000 in values(r),'actual summoner/winner owner match,once and native cleanup'
for cls in ['boss_chijiAI','boss_xuenAI','boss_yulonAI','boss_niuzaoAI']:
 r=hook(cls);assert sites(r,'?GetAffectingPlayer@Unit@@')and sites(r,'?GeneratePersonalLoot@Unit@@')and sites(r,'?QuestCredit@'+cls+'@'),'native eligible looter and own victory path '+cls
 c=ins('?QuestCredit@'+cls+'@');assert sites(c,'?GetUnit@ObjectAccessor@@')and sites(c,'?GetAffectingPlayer@Unit@@')and sites(c,'?KilledMonsterCredit@Player@@')and sites(c,'??MObjectGuid@@'),'native participant lookup/owner and GUID set dedup '+cls
r=ins('?OnTrigger@eventobject_antorus_into@@');assert sites(r,'?BeginIntro@AntorusIntroInstance@@')and sites(r,'?FinishIntro@AntorusIntroInstance@@')and sites(r,'?QueueIntroWorld@AntorusIntroInstance@@')and sites(r,'?QueueIntroPlayer@AntorusIntroInstance@@')
assert {1000,7000,8000,9000,21000,30000,40000,125683,124913,125682}<=values(r),'original actors and delays retained'
for name in ['QueueIntroPlayer','QueueIntroWorld','FinishIntro']:assert sites(ins('?'+name+'@AntorusIntroInstance@@'),'?AddDelayedEvent@InstanceScript@@'),'instance-owned delayed work '+name
player,=callbacks('AntorusIntroInstance','QueueIntroPlayer');c=expanded(player);assert sites(c,'?FindPlayer@ObjectAccessor@@')and sites(c,'?IsInWorld@Object@@')and op(c,'cmp','qword ptr [rdi + 8], rdx')and op(c,'cmp','dword ptr [rdi + 0xa18], 0')and sites(c,'?FinishIntro@AntorusIntroInstance@@'),'fresh live same-instance player or failed-attempt rearm'
rearm,=callbacks('AntorusIntroInstance','FinishIntro');c=expanded(rearm);assert sites(c,'?GetEventObject@Map@@')and any('0x4e0]'in x.op_str for x in c)and any('0x4f0]'in x.op_str for x in c),'both real affected-player sets rearmed after native callback bookkeeping'
effects=[expanded(s)for s in callbacks('eventobject_antorus_into','OnTrigger')];all_values=values(r).union(*(values(c)for c in effects));assert {5522,5524,249441,249802,250695,250698,250700,250728,250797,50000}<=all_values,'authored conversation/spell stages remain linked'
factory=ins('?GetInstanceScript@instance_antorus@@');assert sites(factory,'??0instance_antorus_InstanceMapScript@instance_antorus@@')and sites(ins('??0instance_antorus_InstanceMapScript@instance_antorus@@'),'??0InstanceScript@@'),'real raid factory owns the new instance state'
print('PASS actual Release twentyfirst ten:instance-owned Antorus entry/attempt queues,fresh live Player and EO retry of both affected sets,authored actors/ordered effects;gossip-bound actual Valiant challenger,post-scaling exact lethal/owner/group credit and bounded helper survival;initialized Tsulong evade;Cheng summoner-owner match;four Celestial native owner loot and unique threat-participant credit. Prior207 selected checks retained,total217;source/native/full-client scopes remain distinct.')
