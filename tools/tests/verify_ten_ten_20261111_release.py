from pathlib import Path
import re
from capstone import CS_OP_IMM
from ten_ten_20261111_scope import PLAN,ROOT,require_fixed
from five_ten_20261111_pe import n,ins,values,sites,callbacks,expanded
require_fixed()
language=int(re.search(r'LANG_PLAYER_NOT_FOUND\s*=\s*(\d+)',(ROOT/'src/server/game/Miscellaneous/Language.h').read_text())[1])
for e in PLAN['changes']:
 methods=[e['method']]if e['kind']=='console'else e.get('methods',[])
 for method in methods:
  rows=ins('?'+method+'@'+e.get('cls','misc_commandscript')+'@');assert language in values(rows),(e['key'],'explicit localized missing-player failure')
  assert sum(x.mnemonic=='test'and re.fullmatch(r'(r\w+), \1',x.op_str)is not None for x in rows[:45])>=2,(e['key'],'session and actual player gates')
  assert any(x.mnemonic=='mov'and x.op_str.startswith('byte ptr [')and x.op_str.endswith(', 1')for x in rows),(e['key'],'single custom error reporting remains explicit')
  assert sites(rows,'?SendSysMessage@ChatHandler@@'),(e['key'],'visible diagnostic retained')
for e in PLAN['changes']:
 if e['batch']!=2 or e['case']=='char_display':continue
 method={'scaling':'CalculateAmount','pursuit':'HandleExtraEffect','xt002':'OnRemove','augur':'OnTick'if 'FelEjection'in e['key']else'OnRemove','molten':'OnUpdate','absorb':'Absorb'}[e['case']]
 rows=ins('?'+method+'@'+e['cls']+'@')
 if e['case']=='absorb':
  assert sites(rows,'?GetTarget@AuraScript@@')and not sites(rows,'?GetCaster@AuraScript@@'),'Linked protection uses the actual aura recipient'
  assert {220110,220113,220073,220111,220103,250}<=values(rows),'Original protection branches/children/duration retained'
  assert len(sites(rows,'?CastSpell@Unit@@'))==2,'Both original protection casts remain'
  continue
 getter=sites(rows,'?GetCaster@AuraScript@@');assert len(getter)==(2 if e['case']=='molten'else 1),(e['key'],'actual independent caster lookups')
 for at in getter:
  next_rows=rows[at+1:at+6];test=next((i for i,x in enumerate(next_rows)if x.mnemonic=='test'and x.op_str=='rax, rax'),None)
  assert test is not None and next_rows[test+1].mnemonic=='je',(e['key'],'nullable caster exit dominates its dependent dereference')
 if e['case']=='pursuit':assert sum(x.mnemonic=='test'and x.op_str=='rax, rax'for x in rows)>=2,'Current typed Ick AI must also pass its own nullable gate'
third=[e for e in PLAN['changes']if e['batch']==3]
if third:
 def rows_for(case,method=None):
  e=next(e for e in third if e['case']==case)
  return ins('?'+(method or e['method'])+'@'+e['cls']+'@')
 def null_gate(rows,at):
  tail=rows[at+1:at+9]
  return any(x.mnemonic=='test'and x.op_str=='rax, rax'and tail[i+1].mnemonic in('je','jne')for i,x in enumerate(tail[:-1]))
 for case in ('pyramid','wind','elerethe','clones'):
  rows=rows_for(case);calls=sites(rows,'?SummonCreature@')
  assert calls and any(null_gate(rows,at)for at in calls),(case,'actual summon result must pass a native nullable gate before use')
 rows=rows_for('wind','OnGossipSelect');calls=sites(rows,'?SummonNPC@');assert len(calls)==1 and rows[calls[0]+1].mnemonic=='test'and rows[calls[0]+1].op_str=='al, al'and rows[calls[0]+2].mnemonic=='je','Actual shared offering consumption waits for successful selected summon'
 assert sites(rows,'?RemoveAura@Unit@@')or sites(rows,'?RemoveAura@Player@@'),'Offering still consumed after actual success'
 rows=rows_for('ravager');assert any(x.mnemonic=='cmp'and x.op_str.startswith('qword ptr [rdi + 8],')and rows[i+1].mnemonic=='je'for i,x in enumerate(rows[:-1])),'Actual Ravager target-list size gate'
 rows=rows_for('withering');assert any(x.mnemonic=='sete'and x.op_str=='dl'for x in rows)and any(x.mnemonic=='test'and x.op_str=='dl, dl'and rows[i+1].mnemonic=='je'for i,x in enumerate(rows[:-1])),'Actual visual-ship front/back selections are guarded independently from missile payload'
 rows=rows_for('arnath');calls=sites(rows,'?DoFindFriendlyMissingBuff@ScriptedAI@@');assert len(calls)==1;tail=rows[calls[0]+1:calls[0]+7];assert any(x.mnemonic=='test'and x.op_str=='rdx, rdx'and tail[i+1].mnemonic=='je'for i,x in enumerate(tail[:-1])),'Actual all-buffed list skips only shield selection'
 rows=rows_for('portals','SetData');assert any(x.mnemonic=='cmp'and x.op_str=='qword ptr [rcx + 0x2f8], 0'and rows[i+1].mnemonic=='je'for i,x in enumerate(rows[:-1])),'Actual inlined power-spark portal-list size guard'
 rows=rows_for('staffs');assert any(x.mnemonic=='mov'and x.op_str=='rdx, qword ptr [rcx + 0x3a8]'and rows[i+1].mnemonic=='test'and rows[i+1].op_str=='rdx, rdx'and rows[i+2].mnemonic=='je'for i,x in enumerate(rows[:-2])),'Actual animated-staff list size guard'
 rows=rows_for('shatter');calls=sites(rows,'?GetCaster@AuraScript@@');assert len(calls)==1 and null_gate(rows,calls[0]),'Actual Shatter caster guard'
 assert any(x.mnemonic=='test'and x.op_str=='rcx, rcx'and rows[i+1].mnemonic=='je'for i,x in enumerate(rows[:calls[0]-1])),'Independent crystal AI action has its own nullable current-AI guard before caster admission'
fourth=[e for e in PLAN['changes']if e['batch']==4]
if fourth:
 for cls in ('npc_bloodsnow_frostmane_bonechiller','npc_viletongue_sauroks'):
  reset=ins('?Reset@'+cls+'@');assert any(x.mnemonic=='mov'and x.op_str.startswith('byte ptr [')and x.op_str.endswith(', 0')for x in reset),(cls,'actual reset rearms its one-shot phase')
 rows=ins('?DamageTaken@npc_bloodsnow_frostmane_bonechiller@');assert sites(rows,'?GetAffectingPlayer@Unit@@'),'Actual NPC floor admits legitimate player/owner chain'
 rows=ins('?DamageTaken@npc_viletongue_sauroks@');cast,=sites(rows,'?CastSpell@Unit@@');tail=rows[cast+1:cast+6];assert tail[0].mnemonic=='test'and tail[0].op_str=='eax, eax'and any(x.mnemonic=='mov'and x.op_str.startswith('byte ptr [')and x.op_str.endswith(', 0')for x in tail),'Actual rejected poison request rearms its latch'
 rows=ins('?InitializeAI@npc_ebon_gargoyleAI@');parent,=sites(rows,'?InitializeAI@CasterAI@@');assert {'dword ptr [rcx + 0x60], eax','dword ptr [rcx + 0x88], eax'}<={x.op_str for x in rows[:parent]if x.mnemonic=='mov'}and any(x.mnemonic=='xor'and x.op_str=='eax, eax'for x in rows[:parent]),'Both actual gargoyle clocks are zeroed before parent/owner early-return paths'
 closures=[expanded(name)for name in callbacks('boss_fallen_avatar','EnterCombat')];assert len(closures)==1;rows=closures[0];assert any(x.mnemonic=='cmp'and x.op_str=='dword ptr [rax + 8], 0'and rows[i+1].mnemonic=='je'for i,x in enumerate(rows[:10]))and sites(rows,'?GetCreature@SummonList@@'),'Actual Avatar weak-owner gate precedes source summon-list access'
 for method,cls in [('PeriodicTick','spell_summoning_rhyme_aura_AuraScript'),('OnRemove','spell_drain_fel_enegry_AuraScript')]:
  rows=ins('?'+method+'@'+cls+'@');at,=sites(rows,'?GetCaster@AuraScript@@');assert null_gate(rows,at),(cls,'actual nullable caster gate')
 rows=ins('?UpdateAI@npc_lilet_lunarhAI@');assert len(sites(rows,'?GetPlayer@ObjectAccessor@@'))==1 and 16000 in values(rows),'Actual dialogue resolves current player rather than a retained raw owner pointer'
 reset=ins('?Reset@boss_mistress_alluradel@');rows=ins('?UpdateAI@boss_mistress_alluradel@');assert any(x.mnemonic=='mov'and x.op_str=='dword ptr [rbx + 0x58], 0'for x in reset)and any(x.mnemonic=='mov'and x.op_str=='dword ptr [rbx + 0x58], 0x1f4'for x in rows)and not any(x.mnemonic=='mov'and x.op_str=='dword ptr [rbx + 0x58], 0'for x in rows),'Actual Sadist poll resets only on Reset and honors its authored500ms interval'
 for cls in ('instance_temple_of_ahnqiraj_InstanceMapScript','instance_ruins_of_ahnqiraj_InstanceMapScript'):
  rows=ins('?Update@'+cls+'@');reads=sites(rows,'?getWorldState@World@@');apply,=sites(rows,'?ApplyOnEveryPlayer@Map@@');assert len(reads)==2 and max(reads)<apply,(cls,'one native score snapshot precedes every-player dispatch')
 rows=ins('?OnApply@spell_gen_clone_weapon_auraScript@');at,=sites(rows,'?GetCaster@AuraScript@@');assert null_gate(rows,at)and len(sites(rows,'?GetUInt16Value@Object@@'))>=4 and len(sites(rows,'?SetVirtualItem@Unit@@'))==1,'Actual clone captures/copies both appearance halves with native item setter'
 rows=ins('?OnRemove@spell_gen_clone_weapon_auraScript@');assert not sites(rows,'?GetCaster@AuraScript@@')and len(sites(rows,'?SetVirtualItem@Unit@@'))==1 and any(x.mnemonic=='test'and x.op_str.endswith(', 1')for x in rows),'Actual clone restore is caster-independent and keeps reapply snapshot until real removal'
rows=ins('?HandleScript@spell_sinestra_indomitable_AuraScript@');assert sum(x.mnemonic=='cmp'and x.op_str=='al, 5'for x in rows)==2 and any(x.mnemonic=='inc'and x.op_str=='al'for x in rows)and any(x.mnemonic=='mov'and x.op_str=='byte ptr [rdi + 0x360], al'for x in rows),'Actual per-instance bounded control counter'
factory=ins('?GetAuraScript@spell_sinestra_indomitable@@');assert any(x.mnemonic=='mov'and x.op_str=='byte ptr [rbx + 0x360], 0'for x in factory),'Each actual new AuraScript initializes its own counter'
raw=(n['args'].release/'worldserver.map').read_text(errors='replace');assert not any('controlTimer@'in line and'spell_sinestra_indomitable_AuraScript'in line for line in raw.splitlines()),'Shared function-static control counter remains in linked data'
print('PASS linked implemented repairs:actual command context gates/visible localized refusal,per-AuraScript bounded Indomitable counter,independent foreign-caster gates and actual protection recipient/children/duration;ASAN/native/full gameplay scopes separate')
fifth=[e for e in PLAN['changes']if e['batch']==5]
if fifth:
 rows=ins('?Register@spell_generic_clone_weapon_SpellScript@');assert any(x.mnemonic=='mov'and x.op_str=='r9d, 3'for x in rows),'Actual generic parent registration matches native DUMMY=3'
 rows=ins('?HandleScriptEffect@spell_generic_clone_weapon_SpellScript@');assert len(sites(rows,'?CastSpell@Unit@@'))==1 and sites(rows,'?PreventHitDefaultEffect@SpellScript@@'),'Single native parent child route prevents duplicate default effect'
 rows=ins('?OnCreatureCreate_sha_of_doubt@');assert len(sites(rows,'?GetVirtualItemId@Unit@@'))==3 and len(sites(rows,'?GetVirtualItemAppearanceMod@Unit@@'))==3 and len(sites(rows,'?GetUInt16Value@Object@@'))==3,'All3actual figment fallback slots read native item andboth appearance halves'
 for method in ('GetAI','Reset'):
  rows=ins('?'+method+'@'+('boss_king_dred@@'if method=='GetAI'else'boss_king_dredAI@'));assert any(x.mnemonic=='mov'and x.op_str=='dword ptr [rbx + 0x170], 0'for x in rows),'Dred raptor count starts/resets at native32bit zero'
 rows=ins('?Reset@boss_millificent_manastormAI@');assert any(x.mnemonic=='mov'and x.op_str=='byte ptr [rbx + 0x180], 0'for x in rows),'Actual laugh counter resets before each encounter'
 rows=ins('?GetAI@boss_millificent_manastorm@@');assert any(x.mnemonic=='mov'and x.op_str=='dword ptr [rdi + 0x180], eax'for x in rows)and any(x.mnemonic=='xor'and x.op_str=='eax, eax'for x in rows),'Actual Millificent constructor includes laugh counter in zero initialization'
 rows=ins('?DamageTaken@npc_salsalabimAI@');assert rows[0].mnemonic=='test'and rows[0].op_str=='rdx, rdx'and sites(rows,'?GetAffectingPlayer@Unit@@')and sites(rows,'?GroupEventHappens@Player@@'),'Actual nullable attacker resolves native eligible player before native group event'
 rows=ins('?UpdateAI@boss_malygosAI@');spawn=sites(rows,'?SummonCreature@');assert len(spawn)==4 and all(null_gate(rows,at)for at in spawn),'All4actual disk summon sites guard returned actor before AI/movement'
 rows=ins('?UpdateAI@boss_sartharionAI@');assert len(sites(rows,'?SummonCreature@'))==5 and sum(x.mnemonic=='test'and re.fullmatch(r'(r\w+), \1',x.op_str)is not None and rows[i+1].mnemonic=='je'for i,x in enumerate(rows[:-1]))>=10,'Actual five-actor tsunami method retains nullable movement branches'
 rows=ins('?SetBossState@instance_pit_of_saron_InstanceScript@');assert {37591,37592}<=values(rows)and any(null_gate(rows,at)for at in sites(rows,'?SummonCreature@')),'Both Garfrost faction IDs reach common guarded summon/movement path'
 rows=ins('?Reset@boss_halfus_wyrmbreakerAI@');assert len(sites(rows,'?SummonCreature@'))==6 and sum(x.mnemonic=='test'and re.fullmatch(r'(r\w+), \1',x.op_str)is not None and rows[i+1].mnemonic in('je','jne')for i,x in enumerate(rows[:-1]))>=25,'Actual whelp-loop/five-drake reset retains independent nullable GUID/cast gates'
 rows=ins('?UpdateAI@npc_static_orbAI@');stun=next(i for i,x in enumerate(rows)if x.mnemonic=='test'and x.op_str=='byte ptr [rsi + 0x3140], 8');consumed=next(i for i,x in enumerate(rows)if i>stun and x.mnemonic=='mov'and x.op_str=='byte ptr [rdi + 0x50], 1');assert stun<consumed and any(x.mnemonic=='mov'and x.op_str=='dword ptr [rdi + 0x54], 0x1f4'for x in rows),'Actual stun admission precedes transition consumption and retains500ms retry'
 print('PASS linked fifth10:actualDUMMYhook/onechild,threepairedfigmentslots,constructor/resetcounters,nativepetcredit,guardedtsunami/disks/bothfactionleader/drakes,andstun-before-chase admission')
sixth=[e for e in PLAN['changes']if e['batch']==6]
if sixth:
 rows=ins('?FilterTargets@spell_activate_closes_anvil_SpellScript@');assert {218741,218704,218701,218705,218702,218706,218703}<=values(rows),'Actual seven-anvil admission comparisons are linked'
 rows=ins('?FilterTargets@spell_electric_discharge_SpellScript@');assert {70228,70039,70048}<=values(rows),'Actual three-sha admission comparisons are linked'
 rows=ins('?OnGameObjectCreate@BattlegroundSeethingShore@@');assert {281307,272471,3000}<=values(rows)and sites(rows,'?AddDelayedEvent@FunctionProcessor@@'),'Both native Azerite variants and3s animation queue are linked'
 rows=ins('?Absorb@spell_pri_guardian_spirit_AuraScript@');at,=sites(rows,'?CountPctFromMaxHealth@Unit@@');assert any(x.mnemonic=='cvtsi2ss'and x.op_str.endswith(', rax')for x in rows[at+1:at+15])and 48153 in values(rows),'Actual Guardian rescue converts native unsigned64 maximum-health amount without signed32 truncation'
 for method,cls in [('HandleOnHit','spell_pri_spirit_shell_SpellScript'),('HandleHeal','spell_pri_halo_heal_SpellScript'),('HandleHeal','spell_pri_penance_heal'),('HandleHeal','spell_sha_chain_heal')]:
  rows=ins('?'+method+'@'+cls+'@');assert len(sites(rows,'?GetHitHeal64@SpellScript@@'))==1 and len(sites(rows,'?SetHitHeal64@SpellScript@@'))==1 and not sites(rows,'?GetHitHeal@SpellScript@@')and not sites(rows,'?SetHitHeal@SpellScript@@'),(cls,'Actual effect reads/writes the native exact64 healing channel')
 rows=ins('?HandleProc@spell_sha_ancestral_guidance@');assert any(x.mnemonic=='cvtsi2ss'and x.op_str.endswith(', rcx')for x in rows[:25])and 114911 in values(rows)and len(sites(rows,'?CastCustomSpell@Unit@@'))==1,'Actual Guidance retains64bit heal/damage input before native percent conversion'
 rows=ins('?HandleHeal@spell_dk_death_strike_SpellScript@');at,=sites(rows,'?CastCustomSpell@Unit@@');assert any(x.mnemonic=='cmp'and re.fullmatch(r'r(?:[a-z]+|\d+), r(?:[a-z]+|\d+)',x.op_str)for x in rows[:at])and any(x.mnemonic=='cvtsi2ss'and re.search(r', r(?:[a-z]+|\d+)$',x.op_str)for x in rows[:at])and {45470,66188,137006,246426,48792}<=values(rows),'Actual DeathStrike chooses wide minimum/history andpreserves child,Frost andHeartOfIce gates'
 print('PASS linked sixth10:actual authored target IDs andAzerite queue,native64 Guardian/Shell/Halo/Penance/Chain/Guidance/DeathStrike calculations;ordinary effect accumulation remains separately scoped')
seventh=[e for e in PLAN['changes']if e['batch']==7]
if seventh:
 methods={'cloudburst':'HandleHeal','mists':'HandleHeal','upkeep':'HandleHeal','hymn':'HandleOnHit','mend':'HandleHeal','covenant':'HandleHeal','atonement':'HandleOnHit','storm':'HandleHeal','shock':'HandleOnHit','saves':'HandleOnHit'}
 for e in seventh:
  rows=ins('?'+methods[e['case']]+'@'+e['cls']+'@');assert sites(rows,'?GetHitHeal64@SpellScript@@')and not sites(rows,'?GetHitHeal@SpellScript@@')and not sites(rows,'?SetHitHeal@SpellScript@@'),(e['key'],'actual native64 source channel')
  if e['case']not in('mend','covenant'):assert sites(rows,'?SetHitHeal64@SpellScript@@'),(e['key'],'actual native64 result channel')
 for cls in('spell_sha_cloudburst_heal','spell_sha_restorative_mists'):
  rows=ins('?HandleHeal@'+cls+'@');assert any(x.mnemonic=='div'and x.op_str.startswith('r')for x in rows)and any(x.mnemonic=='test'and rows[i+1].mnemonic=='je'for i,x in enumerate(rows[:-1])),(cls,'real64 division retains nonzero recipient guard')
 rows=ins('?HandleHeal@spell_sha_cumulative_upkeep@');assert sites(rows,'?GetAnyOwner@Unit@@')and {207362,208205}<=values(rows),'Actual native owner/trait/stacked aura admission retained'
 rows=ins('?HandleOnHit@spell_pri_divine_hymn_SpellScript@');assert sites(rows,'?isRaidGroup@Group@@'),'Actual raid gate remains before doubled heal'
 rows=ins('?HandleHeal@spell_pri_shadow_mend@');assert {193063,193065,187464,81749,214205,194384}<=values(rows),'Actual Masochism/penalty/Atonement alternatives retained'
 rows=ins('?HandleHeal@spell_pri_shadow_covenant@');assert {219518,219521}<=values(rows),'Actual Covenant passive andabsorb child retained'
 rows=ins('?HandleOnHit@spell_pri_atonement_heal@');assert {197815,81782}<=values(rows)and sites(rows,'?IsCritForTarget@Spell@@'),'Actual own Barrier andcrit alternatives retained'
 rows=ins('?HandleHeal@spell_pal_healing_storm@');assert sites(rows,'?GetOwner@Unit@@')and 186788 in values(rows),'Actual image owner/Highlord trait retained'
 rows=ins('?HandleOnHit@spell_pal_holy_shock_heal_SpellScript@');assert 200657 in values(rows),'Actual one-use SilverHand aura retained'
 rows=ins('?HandleOnHit@spell_pal_the_light_saves_SpellScript@');assert {53563,156910,200025,200423}<=values(rows),'Actual3owned Beacon variants andone-use LightSaves buff retained'
 print('PASS linked seventh10:native64 recipient splitting andowner/raid/trait/critical/one-use healing modifiers;allfirst60 linked checks retained')
eighth=[e for e in PLAN['changes']if e['batch']==8]
if eighth:
 for cls,method in [('spell_dru_tranquility_SpellScript','HandleOnHit'),('spell_monk_zen_pulse_SpellScript','HandleHeal'),('spell_monk_sheiluns_gift_SpellScript','HandleHeal')]:
  rows=ins('?'+method+'@'+cls+'@');assert sites(rows,'?GetHitHeal64@SpellScript@@')and sites(rows,'?SetHitHeal64@SpellScript@@')and not sites(rows,'?GetHitHeal@SpellScript@@'),(cls,'actual native64 input/output')
 for cls in('spell_monk_zen_pulse_SpellScript','spell_monk_sheiluns_gift_SpellScript'):
  rows=ins('?HandleHeal@'+cls+'@');assert any(x.mnemonic=='div'and x.op_str.startswith('r')for x in rows)and any(x.mnemonic=='imul'and x.op_str.startswith('r')for x in rows),(cls,'bounded integer multiply retains low bits')
 rows=ins('?HandleHeal@spell_monk_revival@');assert sites(rows,'?GetHitHeal64@SpellScript@@')and {199665,199668}<=values(rows),'Actual Revival trait andchild use full input'
 rows=ins('?HandleHeal@spell_warl_health_funnel_SpellScript@');assert sites(rows,'?SetHitHeal64@SpellScript@@')and sites(rows,'?CountPctFromMaxHealth@Unit@@'),'Actual Funnel maximum-health heal uses exact64 setter'
 rows=ins('?CalculateAmount@spell_dru_frenzied_regeneration@');assert sites(rows,'?GetDamageTakenInPastSecs@Unit@@')and any(x.mnemonic=='div'and x.op_str.startswith('r')for x in rows)and any(x.mnemonic=='cvttss2si'and x.op_str.startswith('rbx,')for x in rows),'Actual Frenzied minimum/history retain wide calculation andtick division'
 rows=ins('?After@spell_highmountain_justice_heal@');assert sites(rows,'?GetHitHeal64@SpellScript@@')and not sites(rows,'?GetHitHeal@SpellScript@@')and {97508,97498,97506,1220,5165,40594}<=values(rows),'Actual Justice exact64 admission preserves actor/map/phase/quest gates'
 rows=ins('?HandleHeal@spell_monk_purified_healing_SpellScript@');assert sites(rows,'?SetHitHeal64@SpellScript@@')and {145055,124275,124274,124273}<=values(rows)and any(x.mnemonic=='cvtsi2ss'and x.op_str.endswith(', rbx')for x in rows),'Actual PurifiedHealing accumulates positive Stagger pools before percent conversion'
 rows=ins('?Absorb@spell_gen_intangible_presence@')
 outlined=sites(rows,'?FromFloat@HealingAmount@@')
 if outlined:
  assert len(outlined)==1 and any(x.mnemonic=='cmp'and x.op_str=='rax, rcx'for x in rows[outlined[0]+1:outlined[0]+8])and any(x.mnemonic=='cmovb'and x.op_str=='rcx, rax'for x in rows[outlined[0]+1:outlined[0]+8]),'Actual outlined conversion returns64bit RAX before native legal-damage minimum'
  # This compiler outlined the shared conversion into a stackless leaf >64 bytes.
  # Its exact public symbol interval, terminal return and internal branch targets
  # prove the bounds without interpreting arbitrary adjacent code.
  from five_ten_20261111_pe import decoder
  address=n['symbol']('?FromFloat@HealingAmount@@');end=min(a for a in n['symbols'].values()if a>address);assert 0<end-address<512
  at=n['offset'](address);helper=list(decoder.disasm(n['binary'][at:at+end-address],address))
  while helper and helper[-1].mnemonic in ('int3','nop'):helper.pop()
  assert helper and helper[-1].mnemonic=='ret'and not any(x.mnemonic=='call'or 'rsp'in x.op_str for x in helper),'Proven stackless conversion leaf'
  for x in helper:
   if x.mnemonic.startswith('j'):assert address<=int(x.op_str,16)<helper[-1].address+helper[-1].size
  assert any(x.mnemonic=='cvttss2si'and x.op_str.startswith('rax,')for x in helper)and any(x.mnemonic=='mov'and x.op_str=='rax, 0xffffffffffffffff'for x in helper),'Shared positive conversion retains64bit result and saturation'
 else:
  assert any(x.mnemonic=='cvttss2si'and x.op_str.startswith('rdi,')for x in rows)and any(x.mnemonic=='cmp'and x.op_str=='rdi, rax'for x in rows),'Actual inlined positive conversion precedes64bit minimum'
 assert {230080,230088}<=values(rows),'Actual Intangible source and child spells remain linked;damage remains native32'
 rows=ins('?Trigger@spell_dk_anti_magic_shell_self_AuraScript@');assert {49088,210852,210862,207188}<=values(rows)and any(x.mnemonic=='div'and x.op_str.startswith('r')for x in rows)and sites(rows,'?CountPctFromMaxHealth@Unit@@'),'Actual AMS health threshold/remainder division is64bit andretainsnative resource/optionalheal gates'
 print('PASS linked eighth10:native64 class healing,integer multipliers,Justice heal admission,positiveStagger/aura amounts andAMS threshold/remainders;Smite bounded-damage falsepositive withdrawn,ordinarycorehealing remainsseparate')

ninth=[e for e in PLAN['changes']if e['batch']==9]
if ninth:
 from five_ten_20261111_pe import decoder
 def pet_leaf(prefix):
  try:return ins(prefix)
  except AssertionError as error:
   assert str(error).startswith('Unknown leaf bounds:')
   assert prefix=='?CanSwap@PetBattleTeam@@'
   address=n['symbol'](prefix);end=min(a for a in n['symbols'].values()if a>address);assert 0<end-address<1024
   at=n['offset'](address);return list(decoder.disasm(n['binary'][at:at+end-address],address))
 rows=ins('?Update@PetBattle@@');calls=sites(rows,'?Update@PetBattleTeam@@');assert len(calls)==2 and all(rows[i+1].mnemonic=='test'and rows[i+2].mnemonic=='jne'for i in calls),'Both actual team terminal results dominate the shared finish exit'
 assert any(x.mnemonic=='mov'and x.op_str.endswith(', 2')for x in rows),'Actual disabled-pet loop covers two teams'
 rows=pet_leaf('?CanSwap@PetBattleTeam@@');assert 5 in values(rows)and any(x.mnemonic=='cmp'and x.op_str=='dl, 0xff'for x in rows),'Actual incoming slot zero is distinguished from negative no-replacement sentinel'
 rows=ins('?SwapPet@PetBattle@@');assert 5 in values(rows)and sites(rows,'?CanSwap@PetBattleTeam@@'),'Actual bounded incoming replacement passes native owner/lock admission'
 rows=ins('?Execute@PetBattleAbilityEffect@@')
 assert 235 in values(rows[:25])and any(x.mnemonic in('jae','jb')for x in rows[:25]),'Native execution table boundary precedes indexing'
 # Scoped native selectors can return without touching the handler table.
 # Prove the fallback guard dominates its real indexed read, rather than
 # assuming the guard has to remain among the first25 instructions.
 rows=ins('?SelectTargets@PetBattleAbilityEffect@@')
 guard=next(i for i,x in enumerate(rows)if(x.mnemonic,x.op_str)==('mov','ecx, 0xeb'))
 assert [(x.mnemonic,x.op_str)for x in rows[guard+1:guard+2]]==[('cmp','ax, cx')]
 branch=rows[guard+2];assert branch.mnemonic=='jb'
 lookup=next(i for i,x in enumerate(rows)if x.address==branch.operands[0].imm)
 assert [(x.mnemonic,x.op_str)for x in rows[lookup:lookup+2]]==[('mov','rdx, rax'),('add','rdx, rdx')]
 assert rows[lookup+2].mnemonic=='lea'and rows[lookup+2].op_str.startswith('rax, [rip + ')
 assert (rows[lookup+3].mnemonic,rows[lookup+3].op_str)==('mov','edx, dword ptr [rax + rdx*8]')
 def native_reaches(start,blocked=None):
  table={x.address:x for x in rows};pending=[start];seen=set()
  while pending:
   at=pending.pop()
   if at==blocked or at in seen or at not in table:continue
   seen.add(at);x=table[at]
   if x.mnemonic in('ret','int3','ud2'):continue
   if x.mnemonic.startswith('j')and x.operands[0].type==CS_OP_IMM:pending.append(x.operands[0].imm)
   if x.mnemonic!='jmp':pending.append(x.address+x.size)
  return seen
 indexed=rows[lookup+3].address
 assert indexed not in native_reaches(rows[0].address,rows[guard+1].address),'Every actual selector table read must pass the native235 comparison'
 assert indexed not in native_reaches(branch.address+branch.size),'An out-of-range native effect cannot reach the table read'
 rows=ins('?Execute@PetBattleAbilityEffect@@');at=next(i for i,x in enumerate(rows)if x.mnemonic=='call'and x.op_str=='rdx');assert rows[at+1].mnemonic=='and'and rows[at+2].mnemonic=='add'and rows[at+3].mnemonic=='cmp'and rows[at+4].mnemonic=='jne','Actual effect loop advances by selected target rather than exiting on success'
 rows=ins('?HandleHealPercent@PetBattleAbilityEffect@@')
 from second_ten_ten_20261112_scope import PLAN as SECOND_PLAN
 if any(e['key']=='PetNativePercentHealing'for e in SECOND_PLAN['changes']):
  assert not sites(rows,'?CalculateHeal@PetBattleAbilityEffect@@')and sites(rows,'?GetMaxHealth@BattlePetInstance@@')and sites(rows,'?Heal@PetBattleAbilityEffect@@'),'Unified native percentage path preserves the humanoid rule and excludes power scaling for every real percentage heal'
 else:assert 726 in values(rows)and sites(rows,'?Heal@PetBattleAbilityEffect@@'),'Actual humanoid passive reaches native health/event update'
 rows=ins('?Process@PetBattleAura@@');assert any(x.mnemonic=='cmp'and x.op_str.endswith(', -1')and rows[i+1].mnemonic=='je'and rows[i+2].mnemonic=='dec'for i,x in enumerate(rows[:-2])),'Actual aura tick preserves infinite sentinel before decrement'
 assert sites(rows,'?Execute@PetBattleAbilityEffect@@')and any(x.mnemonic=='mov'and x.op_str=='dword ptr [rax + 0x20], ecx'for x in rows),'Actual periodic dispatch writes BuffTurn on appended event nodes'
 assert ins('?AddAura@PetBattle@@')and ins('?CalculateDamage@PetBattleAbilityEffect@@'),'Actual native slot/critical calculation paths linked; complete behavior separately exercised in the native probe'
 print('PASS linked ninth10:two-team terminal flow,valid incoming slot0,owner/lock call,bounded table dispatch,all-target execution,actual humanoid heal,infinite duration andappended BuffTurn;native slot/crit/selection behavior separately passed')

tenth=[e for e in PLAN['changes']if e['batch']==10]
if tenth:
 def tenth_leaf(prefix):
  try:return ins(prefix)
  except AssertionError as error:
   assert str(error).startswith('Unknown leaf bounds:')and prefix in('?CalcDamage@spell_pri_shadow_mend_aura@','?GetSpeed@BattlePetInstance@@')
   address=n['symbol'](prefix);end=min(a for a in n['symbols'].values()if a>address);assert 0<end-address<1024;at=n['offset'](address);return list(decoder.disasm(n['binary'][at:at+end-address],address))
 rows=ins('?HandleOnHit@spell_pal_light_of_the_protector_SpellScript@');assert sites(rows,'?SetEffectValueFloat@SpellScript@@')and not sites(rows,'?SetEffectValue@SpellScript@@'),'Actual Protector stores its wide-health percentage in the native float effect channel'
 rows=ins('?SetEffectValueFloat@SpellScript@@');assert any(x.mnemonic=='movss'and x.op_str.startswith('dword ptr [')for x in rows),'Named setter writes the native float effect field behind its hook guard'
 rows=tenth_leaf('?CalcDamage@spell_pri_shadow_mend_aura@');assert any(x.mnemonic=='div'and x.op_str.startswith('r')for x in rows)or any(x.mnemonic=='mul'and x.op_str.startswith('r')for x in rows),'Actual penalty budget divides unsigned64 between ten ticks'
 rows=ins('?HandleDummy@spell_warr_commanding_shout@');assert sites(rows,'?CastCustomSpell@Unit@@')and any(x.mnemonic=='cvtsi2ss'and x.op_str.endswith(', rax')for x in rows),'Actual Shout preserves full maximum health before float child request'
 rows=ins('?UpdateMaxHealth@Player@@');assert sites(rows,'?SetMaxHealth@Unit@@')and any(x.mnemonic=='cvttss2si'and x.op_str.startswith('rdx,')for x in rows),'Actual Player stat refresh reaches unsigned64 maximum-health setter'
 rows=ins('?HandleAuraModIncreaseHealth@AuraEffect@@');assert any(x.mnemonic=='cvttss2si'and x.op_str.startswith('r')for x in rows)and sites(rows,'?ModifyHealth@Unit@@'),'Actual flat-health lifecycle retains signed64 delta domain'
 for method in ('HandleAuraModIncreaseHealthOrPercent','HandleAuraModIncreaseHealthPercent'):
  rows=ins('?'+method+'@AuraEffect@@');assert sites(rows,'?CountPctFromMaxHealth@Unit@@')and sites(rows,'?SetHealth@Unit@@'),(method,'actual native percent/positive gain path remains linked')
 assert not any('spell_copy_melee_weapon@'in name for name in n['symbols']),'Obsolete duplicate clone class/registration is absent from linked Release'
 for prefix in ('?GetNpcDamageTakenInPastSecs@Unit@@','?GetPlayerDamageTakenInPastSecs@Unit@@','?GetDamageTakenInPastSecs@Unit@@'):
  names=[name for name in n['symbols']if name.startswith(prefix)];assert (len(names)==1 or prefix!='?GetDamageTakenInPastSecs@Unit@@'and not names)and all('@@QEBA_K'in name for name in names),(prefix,'referenced canonical history ABI returns unsigned64;unreferenced origin-only getters may be discarded by OPTREF andarenative-tested')
 rows=tenth_leaf('?GetSpeed@BattlePetInstance@@')
 if any(e['key']=='PetDynamicMaximumHealth'for e in SECOND_PLAN['changes']):
  assert sites(rows,'?GetMaxHealth@BattlePetInstance@@')and any(x.mnemonic=='movsxd'and x.op_str=='rax, dword ptr [rbx + 0x60]'for x in rows)and any(x.mnemonic=='add'and x.op_str=='rax, rax'for x in rows)and any(x.mnemonic=='cmp'and x.op_str=='rax, rdx'for x in rows)and any(x.mnemonic=='jle'for x in rows),'Actual flying threshold compares widened doubled health with the new dynamic maximum'
 else:assert any(x.mnemonic=='cmp'and x.op_str=='rax, rcx'for x in rows)and any(x.mnemonic=='jle'for x in rows),'Actual flying speed path checks current versus maximum health'
 rows=ins('?HandleWeatherAura@PetBattleAbilityEffect@@');assert (sites(rows,'?AddAura@PetBattleAbilityEffect@@')or sites(rows,'?AddAura@PetBattle@@'))and sites(rows,'?Expire@PetBattleAura@@'),'Actual weather replacement tracks application and expires previous auras'
 rows=ins('?Apply@PetBattleAura@@');assert sites(rows,'?SetPetState@PetBattle@@'),'Actual aura application still emits native state diagnostics'
 removal=[name for name in n['symbols']if name.startswith('?Remove@PetBattleAura@@')]
 rows=ins('?Remove@PetBattleAura@@')if removal else ins('?Expire@PetBattleAura@@')
 assert sites(rows,'?SetPetState@PetBattle@@'),'Actual removal restores captured state deltas through native event handling, including the compiler-inlined removal body'
 print('PASS linked tenth10:unsigned64 history ABI,wide penalty/effect/health paths,obsolete clone registration removed,actual Smart dispatch andnative state/weather/flying methods;live migration andcomplete native behavior separately required')
