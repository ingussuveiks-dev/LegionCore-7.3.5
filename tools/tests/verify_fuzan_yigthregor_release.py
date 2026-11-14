"""Selected shipped Yigthregor factories, cleanup delegates and late-hit guard."""
from pathlib import Path
import runpy
import hashlib
import json
from sharasdal_erunak_entry_pe import sites, values
n=runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
def check(prefix, calls=(), constants=()):
    body=n['instructions'](prefix)
    for call in calls: assert sites(n,body,call),(prefix,call)
    assert set(constants)<=values(body),(prefix,constants)
    return body
for prefix,native in [('?Reset@npc_fuzan_yigthregor@@','?Reset@SmartAI@@'),('?JustDied@npc_fuzan_yigthregor@@','?JustDied@SmartAI@@')]:
    body=check(prefix,('?GetScenario@ScenarioMgr@@','?CastStop@Unit@@','?RemoveAreaObject@Unit@@',native),(109820,14670030,1625,1137,217787))
    assert sites(n,body,'?RemoveAreaObject@Unit@@')[0]<sites(n,body,native)[0]
check('?Load@spell_fuzan_felfire_bomb@@',('?GetScenario@ScenarioMgr@@',),(109820,14670030,1625,1137))
check('?Hit@spell_fuzan_felfire_bomb@@',('?PreventHitDefaultEffect@SpellScript@@',))
check('?AddSC_instance_temple_of_the_jade_serpent@@',('??0CreatureScript@@',))
check('?GetAI@?$GenericCreatureScript@Unpc_fuzan_yigthregor@@',('??0SmartAI@@',))
assert b'npc_fuzan_yigthregor\0' in n['binary'] and b'spell_fuzan_felfire_bomb\0' in n['binary']
print('PASS linked Yigthregor exact source/factories, owned217787 cleanup, native SmartAI reset/death delegation and late dead-caster hit guard')
print(json.dumps({'world_sha256':hashlib.sha256(n['binary']).hexdigest(),'client_fidelity':'PENDING'}))
