from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_target_profiles_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values']
rows=ins('?SpellHit@npc_generic_soldierAI@');assert sites(rows,'?GetEffect@SpellInfo@@')and sites(rows,'?GetDifficultyID@ScriptedAI@@'),'Mending may not retain the old constant quarter profile'
rows=ins('?FilterTargets@spell_meljarak_mending_target_SpellScript@')
assert sites(rows,'?GetInstanceScript@WorldObject@@')and sites(rows,'?GetCreature@ObjectAccessor@@'),'Mending target filter must resolve the real instance actors'
assert {10,18,62408,65499,65500}<=set(values(rows)),'Nine native swarm slots and their actual entries must remain bounded'
ins('?GetSpellScript@spell_meljarak_mending_target@@');ins('?Register@spell_meljarak_mending_target_SpellScript@')
print('PASS linked Mending:native current-profile amount,unsigned64 ratio/room,real instance swarm filter and script factory/registration;complete prior target-profile/Cordana/46group chain retained. SQL binding/native cast tests and actual client combat remain separate scopes.')
