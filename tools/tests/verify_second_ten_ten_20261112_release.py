"""Selected compiler output checks complement full-method ASAN and native calls."""
from five_ten_20261111_pe import ins,sites,values,n
from second_ten_ten_20261112_scope import require_fixed,PLAN
require_fixed()
rows=ins('?Finish@PetBattle@@')
assert any(x.mnemonic=='cmp'and x.op_str=='eax, 2'and rows[i+1].mnemonic=='je'and rows[i+2].mnemonic=='cmp'and rows[i+2].op_str=='eax, 3'and rows[i+3].mnemonic=='je'and rows[i+1].op_str==rows[i+3].op_str for i,x in enumerate(rows[:-3])),'Both terminal statuses return before completion side effects'
assert not sites(rows,'?GetAvailablesPets@PetBattleTeam@@'),'XP admission must not reuse temporary swap admission'
assert any(x.mnemonic=='cmp'and x.op_str=='byte ptr [rsp + 0x4c], 0'and rows[i+1].mnemonic=='jne'for i,x in enumerate(rows[:-1])),'True ignore-abandon flag skips penalty'
assert any(x.mnemonic=='sub'and x.op_str=='rsi, rcx'for x in rows)and any(x.mnemonic=='cmp'and x.op_str=='r9d, 0x19'for x in rows),'Native64 XP remainder and level25 cap are linked'
assert sum(x.mnemonic=='mulss'for x in rows)>=5 and any(x.mnemonic=='jb'and x.op_str.startswith('0x')and int(x.op_str,16)<x.address for x in rows),'Native XP threshold recalculation/back edge remains'
rows=ins('?CreateRequest@PetBattleSystem@@');assert len(sites(rows,'?RemoveRequest@PetBattleSystem@@'))==1,'Owning request replacement releases previous allocation'
rows=ins('?HandlePetBattleInput@WorldSession@@');assert any(x.mnemonic=='test'and x.op_str=='r8d, r8d'and rows[i+1].mnemonic=='je'for i,x in enumerate(rows[:-1]))and sites(rows,'?ForfeitBattle@PetBattleSystem@@'),'Zero action reaches compiled leave switch'
rows=ins('?Update@PetBattleSystem@@');assert sum(x.mnemonic=='mov'and x.op_str.endswith('+ 0x1c], 0xffffffff')for x in rows)==2,'New proposals reset both native signed pending answers'
for method in ['HandlePetBattleRequestPVP','HandlePetBattleRequestUpdate']:
 expected=2 if method=='HandlePetBattleRequestUpdate'and any(row['batch']==3 for row in PLAN['changes'])else 1
 assert len(sites(ins('?'+method+'@WorldSession@@'),'?CanPlayerEnterInPetBattle@PetBattleSystem@@'))==expected,'Real duel request/acceptance routes run the declared participant admissions'
rows=ins('?CanPlayerEnterInPetBattle@PetBattleSystem@@');assert any(x.mnemonic=='mov'and x.op_str=='rcx, rsi'and rows[i+1].mnemonic=='call'for i,x in enumerate(rows[:-1])),'Admission retains actual initiating player receiver'
print('PASS linked second-hundred first10: terminal completion guards,swap-independent XP,native remainder/threshold loop,forfeit/input,request ownership,fresh consent,and both actual duel admission routes;ASAN/native/client scopes separate')
if any(row['batch']==2 for row in PLAN['changes']):
 rows=ins('?HandlePetBattleRequestUpdate@WorldSession@@');admit=sites(rows,'?CanPlayerEnterInPetBattle@PetBattleSystem@@')[0]
 import re
 guards=[re.fullmatch(r'byte ptr \[(r\w+) \+ 0x54\], 1',x.op_str)for x in rows[:admit]if x.mnemonic=='cmp'];guards=[m for m in guards if m];assert len(guards)==1,'Actual duel type gate precedes admission/flags'
 assert any(x.mnemonic=='lea'and x.op_str=='rcx, ['+guards[0][1]+' + 0x10]'for x in rows[:admit]),'Same request object supplies its challenged-player GUID before side effects'
 rows=ins('?CanCast@PetBattle@@');assert any(x.mnemonic=='cmp'and x.op_str=='dword ptr [r10 + 0x150], 0'and rows[i+1].mnemonic=='jne'for i,x in enumerate(rows[:-1])),'Native dead-pet state is rejected in the inlined CanCastAny path'
 rows=ins('?HandleHealPetType@PetBattleAbilityEffect@@');assert len(sites(rows,'?CalculateHit@PetBattleAbilityEffect@@'))==1 and len(sites(rows,'?CalculateHeal@PetBattleAbilityEffect@@'))==1 and len(sites(rows,'?Heal@PetBattleAbilityEffect@@'))==1,'Each type-heal row has one selected recipient and one amount calculation'
 rows=ins('?HandleDamageWithBonus@PetBattleAbilityEffect@@');assert len(sites(rows,'?CalculateDamage@PetBattleAbilityEffect@@'))==1 and any(x.mnemonic=='mov'and x.op_str=='edx, dword ptr [rbx + 0x3c]'for x in rows),'Bonus uses Target at0x3c and one native damage calculation'
 rows=ins('?HandleDamageStateBonus@PetBattleAbilityEffect@@');assert any(x.mnemonic=='or'and x.op_str.endswith(', 0x10000')for x in rows),'Fungus retains periodic bit16'
 rows=ins('?HandleRampingDamage@PetBattleAbilityEffect@@');assert len(sites(rows,'?CalculateHit@PetBattleAbilityEffect@@'))==1 and {156,930}<=values(rows)and len(sites(rows,'?SetPetState@PetBattle@@'))>=3,'Existing hit admission, two preserved hit-based variants and independent use bookkeeping are linked'
 rows=ins('?HandleRemoveAura@PetBattleAbilityEffect@@');assert len(sites(rows,'?CalculateHit@PetBattleAbilityEffect@@'))==1 and sites(rows,'?Expire@PetBattleAura@@'),'Removal uses actual chance and normal native expiration'
 rows=ins('?HandleSplittedDamage@PetBattleAbilityEffect@@');divide=next(i for i,x in enumerate(rows)if x.mnemonic=='div');assert any(x.mnemonic=='add'and x.op_str=='r9d, eax'for x in rows[:divide]),'Native weather bonus is added before team split'
 print('PASS linked second10: duel reply/type gates,inlined dead ability denial,one-recipient typed heal,target-state damage once,periodic fungus,native hit/use ramp bookkeeping,normal aura expiry,and bonus-before-split;actual property226 selection is independently verified by native full cast')
if any(row['batch']==3 for row in PLAN['changes']):
 rows=ins('?HandleBattlePetSummon@WorldSession@@');owned,=sites(rows,'?GetBattlePet@Player@@');dismiss,=sites(rows,'?UnsummonCurrentBattlePetIfAny@Player@@');summon,=sites(rows,'?SummonBattlePet@Player@@');assert owned<dismiss<summon and any(x.mnemonic=='cmp'and x.op_str=='dword ptr [rax + 0x60], 0'for x in rows[owned:dismiss]),'Owned living pet precedes conditional dismiss/new summon branches'
 rows=ins('?HandleBattlePetSetFlags@WorldSession@@');assert any(x.mnemonic=='cmp'and x.op_str=='al, 1'for x in rows)and any(x.mnemonic=='cmp'and x.op_str=='al, 2'for x in rows)and any(x.mnemonic=='or'and x.op_str.startswith('dword ptr [rcx + 0x6c]')for x in rows)and any(x.mnemonic=='and'and x.op_str.startswith('dword ptr [rcx + 0x6c]')for x in rows),'Actual native control operations set/remove journal flag bits'
 rows=ins('?HandleBattlePetSetSlot@WorldSession@@');assert sites(rows,'?GetUnlockedPetBattleSlot@Player@@')and 0x80 in values(rows),'Slot admission retains real unlock query and native noncombat mask'
 rows=ins('?UpdateBattlePetCombatTeam@Player@@');assert 0x80 in values(rows),'Native roster refresh also filters noncombat species'
 rows=ins('?HandlePetBattleRequestUpdate@WorldSession@@');admissions=sites(rows,'?CanPlayerEnterInPetBattle@PetBattleSystem@@');assert len(admissions)==2,'Actual duel acceptance checks both participants'
 rows=ins('?HandleLockActiveAbility@PetBattleAbilityEffect@@');assert any(x.mnemonic=='mov'and x.op_str=='dword ptr [rcx + 0x140], eax'for x in rows),'Native lock writes separate Lockdowns base0x140'
 rows=ins('?CanCast@PetBattle@@');assert any(x.mnemonic=='cmp'and x.op_str=='dword ptr [rdx + rcx*4 + 0x140], 0'for x in rows),'Actual recast admission checks Lockdowns independently of ordinary cooldown'
 rows=ins('?Cast@PetBattle@@');assert 49 in values(rows)and sites(rows,'?SelectTargets@PetBattleAbilityEffect@@'),'Native proc owner override is scoped to property49 after ordinary selectors'
 rows=ins('?AddPet@PetBattle@@');assert any(x.mnemonic=='mov'and x.op_str=='dword ptr [rcx + 0x47c], eax'for x in rows)and any(x.mnemonic=='mov'and x.op_str=='dword ptr [rax + 0x150], r14d'for x in rows),'Actual native XP snapshot and initial dead state are linked'
 rows=ins('?Begin@PetBattle@@');assert any(x.mnemonic=='cmp'and '+ 0x150]'in x.op_str for x in rows),'Actual initial-front scan checks living state'
 rows=ins('?SendPetBattleFinalRound@WorldSession@@');assert any(x.mnemonic=='cmp'and x.op_str=='dword ptr [rcx + 0x47c], eax'for x in rows)and any(x.mnemonic=='cmp'and x.op_str=='dword ptr [rcx + 0x478], eax'for x in rows),'Actual XP award flag compares saved XP and saved level'
 rows=ins('?Update@PetBattleSystem@@');assert {0x7fffffff,0xffffffff}<=values(rows)and any(x.mnemonic=='imul'and x.op_str=='rax, rcx'and any(y.mnemonic=='div'and y.op_str=='r8'for y in rows[i+1:i+9])for i,x in enumerate(rows)),'Queue estimator uses a64bit weighted product/division and bounded sample count'
 helper='?RemoveBattlePetJournalEntry@@';rows=ins(helper);assert sites(rows,'?UnsummonCurrentBattlePetIfAny@Player@@')and sites(rows,'?UpdateBattlePetCombatTeam@Player@@')and sites(rows,'?SendPetBattleSlotUpdates@WorldSession@@'),'Actual shared removal helper dismisses companion and refreshes roster/UI'
 for method in ['HandleCageBattlePet','HandleBattlePetDelete']:assert len(sites(ins('?'+method+'@WorldSession@@'),helper))==1,'Both actual journal-removal routes call cleanup once'
 rows=ins('?HandleBattlePetNameQuery@WorldSession@@');assert len(sites(rows,'?GetGuidValue@Object@@'))==1 and 0x2c in values(rows),'Name query checks the real creature companion GUID field'
 print('PASS linked third10: owned summon toggle,packet flag control,unlock/noncombat roster,both duel admissions,separate native lockdown/proc-owner removal,initial dead/living front,actual XP snapshot/award comparisons,64bit queue mean,shared cage/release cleanup andbound name query;native full sequences/ASAN/client scopes separate')
if any(row['batch']==4 for row in PLAN['changes']):
 checks=[('npc_corstilax_quarantineAI','SpellHit','GetPlayer'),('npc_corstilax_suppression_protocolAI','UpdateAI','GetUnit'),('npc_hatecoil_monsoonAI','UpdateAI','GetUnit'),('npc_skovald_honored_ancestorAI','UpdateAI','GetUnit'),('npc_stillpine_capitiveAI','MovementInform','GetPlayer'),('npc_bethtilac_cinderweb_spiderlingAI','UpdateAI','GetCreature'),('npc_magma_jetAI','UpdateAI','GetCreature'),('npc_chogall_fire_elementalAI','UpdateAI','GetCreature'),('npc_chogall_shadow_lordAI','UpdateAI','GetCreature'),('npc_wyrmrest_skytalonAI','UpdateAI','GetPlayer'),('npc_amber_pool_stalkerAI','UpdateAI','GetUnit')]
 for cls,method,lookup in checks:
  rows=ins('?'+method+'@'+cls+'@');calls=sites(rows,'?'+lookup+'@ObjectAccessor@@');assert len(calls)==1,(cls,'exact live GUID lookup')
  tail=rows[calls[0]+1:calls[0]+5];assert any(x.mnemonic=='test'and x.op_str=='rax, rax'for x in tail)and any(x.mnemonic in ('je','jne')for x in tail),(cls,'lookup null result is branched before use')
 rows=ins('?MovementInform@npc_stillpine_capitiveAI@');assert any(x.mnemonic=='cmp'and x.op_str=='byte ptr [rcx + 0x68], 0'for x in rows[:14]),'Captive repeated movement callbacks are rejected before live owner lookup'
 print('PASS linked fourth10: eleven actual AI callbacks perform typed live GUID lookups and null-result branches; captive credit is guarded against repeats. Both Cho gall elementals are one owner-lifetime family. Complete-class ASAN, native script bindings, ordinary startup and client traversal are separate scopes.')
if any(row['batch']==5 for row in PLAN['changes']):
 checks=[('npc_anchorite_truuenAI','WaypointReached','GetCreature',2),('boss_infinite_corruptorAI','JustReachedHome','GetCreature',1),('go_q39262AI','UpdateAI','GetPlayer',1),('npc_amber_monsterAI','SpellHitTarget','GetPlayer',1),('npc_amber_monsterAI','UpdateAI','GetPlayer',1),('boss_mephistrothAI','JustSummoned','GetCreature',1),('boss_naltiraAI','SpellHitTarget','GetUnit',1),('boss_advisor_melandrusAI','UpdateAI','GetCreature',1),('boss_morchokAI','UpdateAI','GetCreature',1)]
 for cls,method,lookup,count in checks:
  rows=ins('?'+method+'@'+cls+'@');calls=sites(rows,'?'+lookup+'@ObjectAccessor@@');assert len(calls)==count,(cls,'actual delayed GUID lookups')
  if cls=='boss_morchokAI':
   at=calls[0];assert rows[at+2].mnemonic=='mov'and rows[at+2].op_str=='r14, rax','Actual twin lookup is retained in r14'
   first=next(i for i,x in enumerate(rows)if '[r14 +'in x.op_str);assert any(x.mnemonic=='test'and x.op_str=='r14, r14'and rows[i+1].mnemonic=='je'for i,x in enumerate(rows[:first])),'Current twin result is guarded before its first dereference'
   continue
  for at in calls:
   tail=rows[at+1:at+9];assert any(x.mnemonic=='test'and x.op_str=='rax, rax'and tail[i+1].mnemonic in ('je','jne')for i,x in enumerate(tail[:-1])),(cls,'lookup result is guarded')
 rows=ins('?UpdateAI@npc_monk_wind_spiritAI@');calls=sites(rows,'?GetUnit@ObjectAccessor@@');assert len(calls)==2
 tail=rows[calls[-1]+1:calls[-1]+7];assert any(x.mnemonic=='test'and x.op_str=='rbx, rbx'for x in tail)and any(x.mnemonic=='test'and x.op_str=='rax, rax'for x in tail),'Wind launch validates both resolved actors'
 rows=ins('?SpellHitTarget@npc_amber_monsterAI@');remove,=sites(rows,'?RemoveAllPassengers@Vehicle@@');lookup,=sites(rows,'?GetPlayer@ObjectAccessor@@');assert remove<lookup,'Fling resolves actual passenger again after native removal callbacks'
 rows=ins('?PassengerBoarded@npc_amber_monsterAI@');assert sites(rows,'?CancelEvent@EventMap@@'),'Actual unboarding cancels pending fling'
 rows=ins('?UpdateAI@boss_advisor_melandrusAI@');summon,=sites(rows,'?SummonCreature@');tail=rows[summon+1:summon+10];assert 5000 in values(tail)and any(x.mnemonic=='test'and x.op_str=='rax, rax'for x in tail),'Failed authored actor retry retains five-second dialogue cadence and guards its result'
 rows=ins('?UpdateAI@npc_temple_adeptAI@');nearest,=sites(rows,'?FindNearestCreature@');assert any(x.mnemonic=='add'and x.op_str=='rdi, 4'for x in rows[nearest:])and any(x.mnemonic=='jmp'and x.op_str==hex(rows[nearest-3].address)for x in rows[nearest:]),'Four authored ally types are searched with a genuine loop rather than nested else branches'
 assert 87779 in values(rows)and any(x.mnemonic=='comiss'for x in rows),'Original Greater Heal and injured-health admission remain linked'
 print('PASS linked fifth10: complete native GUID guards for escort/rift/cage/spirit/passenger/Illidan/web/dialogue/twin callbacks, post-unboarding rider re-resolution, canceled stale fling, guarded authored intro retry and genuine first-injured ally search; native catalogue, source ASAN and client scopes separate.')
