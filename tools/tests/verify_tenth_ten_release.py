"""Actual retained event/actor/participant paths; client encounters are separate."""
from pathlib import Path
import json,runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_ninth_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];ref=p['ref'];values=p['values']
def op(rows,m,s):return any(x.mnemonic==m and x.op_str==s for x in rows)
def body(cls,method):return ins('?'+method+'@'+cls+'@')
plan=json.loads(Path(__file__).with_name('TenthTenRepairs.json').read_text());total=0
for e in plan:
 if not e.get('event_abort'):continue
 prefixes=['?'+method+'@'+e['cls']+'@'for method in e['methods']]
 if e['key']=='LichKing':prefixes+=['?HandleScript@spell_the_lich_king_restore_soul_SpellScript@']
 count=0
 for prefix in prefixes:
  # Elisande's retained event-abort path moved to the final-damage hook.
  if prefix=='?DamageTaken@boss_elisande@' and not any(s.startswith(prefix)for s in n['symbols']):
   prefix='?DamageTakenAfterScaling@boss_elisande@'
  rows=ins(prefix)
  for at in sites(rows,'?KillAllEvents@EventProcessor@@'):
   assert op(rows[max(0,at-7):at],'xor','edx, edx'),(e['key'],'non-force argument')
   count+=1
 assert count==e['abort_count'],(e['key'],count);total+=count
assert total==11
for cls,method in [('spell_folnuna_nausea','Tick'),('dread_spray_buff_aura_script','OnTick'),('dread_spray_buff_aura_script','OnApply'),('dread_spray_buff_aura_script','OnRemove'),('spell_forging_AuraScript','OnTick')]:
 assert ref(body(cls,'Register'),n['symbol']('?'+method+'@'+cls+'@')),(cls,method,'native hook')
rows=body('boss_matron_folnuna','Reset');at,=sites(rows,'?SetPowerType@Unit@@');maximum,=sites(rows,'?SetMaxPower@Unit@@');assert at<maximum and op(rows[max(0,at-4):at],'mov','edx, 3')
rows=body('spell_folnuna_nausea','Tick');at,=sites(rows,'?SetPower@Unit@@');assert any(x.mnemonic=='inc'for x in rows[at:])and op(rows,'div','rcx') and op(rows,'mov','dword ptr [rdi + 0x380], edx')
rows=body('dread_spray_buff_aura_script','OnTick');lookup,=sites(rows,'?GetCreature@ObjectAccessor@@');cast,=sites(rows,'?CastSpell@Unit@@');assert lookup<cast and any(x.mnemonic=='test'and x.op_str=='rax, rax'and rows[i+1].mnemonic=='je'and rows[i+1].operands[0].imm>rows[cast].address for i,x in enumerate(rows)if lookup<i<lookup+6)
assert sites(body('dread_spray_buff_aura_script','OnApply'),'?GetGuidValue@Object@@')
rows=body('spell_forging_AuraScript','OnTick');casts=sites(rows,'?CastSpell@Unit@@');assert len(casts)==2 and {138875,140489}<=values(rows)
assert op(rows[casts[0]:casts[1]],'cmp','qword ptr [rcx], rdi')and op(rows[casts[0]:casts[1]],'cmp','qword ptr [rcx + 8], rdi')
lambdas=[]
for name in n['symbols']:
 if name.startswith('?_Do_call@')and'boss_tectus' in name:
  rows=ins(name)
  if sites(rows,'?Kill@Unit@@'):lambdas.append(rows)
rows,=lambdas;cast,=sites(rows,'?Kill@Unit@@');assert op(rows[:cast],'cmp','qword ptr [rcx], 0')and op(rows[:cast],'cmp','qword ptr [rcx + 8], 0')
print('PASS actual Release tenth ten:11nonforce cleanup calls in6encounters;native Folnuna energytype before maxpower and bounded index advance;Dread registered GUID apply/resolve/null-safe tick/remove;Forge and actual8s Tectus closure empty-list guard and nativeKill boss rather than self-target117624. Prior97 selected paths retained;source/native/client scopes distinct.')
