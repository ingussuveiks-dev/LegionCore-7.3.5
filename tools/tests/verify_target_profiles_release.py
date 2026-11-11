from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_cordana_walls_release.py')))
loader=p
while 'callbacks'not in loader:loader=loader['p']
loader=loader|p;n=loader['n'];ins=loader['ins'];sites=loader['sites'];values=loader['values']
for prefix in ['?CallScriptObjectAreaTargetSelectHandlers@Spell@@','?CallScriptObjectTargetSelectHandlers@Spell@@']:
 rows=ins(prefix);call,=sites(rows,'?IsEffectAffectedForDifficulty@TargetHook@SpellScript@@')
 assert any(x.mnemonic=='movzx'and x.op_str.startswith('r9d, byte ptr [')for x in rows[max(0,call-16):call]),'Native caster difficulty must reach the real target-hook dispatch'
rows=ins('?IsEffectAffectedForDifficulty@TargetHook@SpellScript@@')
assert sites(rows,'?GetEffect@SpellInfo@@')and sites(rows,'?CheckTarget@TargetHook@SpellScript@@'),'Runtime must use the authored current/fallback effect data'
rows=ins('?CheckEffect@TargetHook@SpellScript@@');assert sites(rows,'?CheckTarget@TargetHook@SpellScript@@'),'Startup must validate authored target profiles'
print('PASS linked target profiles:real area/single dispatch passes current difficulty;runtime uses native GetEffect/target matching;startup profile validation present. Unreferenced jump dispatcher is source/native-predicate scope,not a linked runtime claim. Complete prior46groups and Cordana retained.')
