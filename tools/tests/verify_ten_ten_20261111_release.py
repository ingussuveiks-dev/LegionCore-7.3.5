from pathlib import Path
import re
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
