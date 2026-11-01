"""Check selected shipped Summerpetal factories, native timers and continuation."""
from pathlib import Path
import hashlib
import json
import runpy
import struct

prior=runpy.run_path(str(Path(__file__).with_name('verify_fuzan_belphiar_release.py')))
n,require=prior['n'],prior['require']
sites,values=prior['sites'],prior['values']
suffix='@npc_fuzan_summerpetal@@'
world=require('?World'+suffix,('?GetScenario@ScenarioMgr@@','?GetScenarioId@Scenario@@'),(1137,))
capture=require('?Capture'+suffix,('?World'+suffix,'?GetScript@SmartAIMgr@@'),(109355,109397,373405,49140,49356))
owner=require('?Owner'+suffix,('?GetPlayer@ObjectAccessor@@','?GetQuestCancellationReceipt@Player@@'),(42765,))
update=require('?UpdateAI'+suffix,('?UpdateAI@SmartAI@@','?IsSummonedBy@SmartAI@@','?Continue'+suffix), (49140,49356,1000,30))
resume=require('?Continue'+suffix,('?ProcessEvent@SmartScript@@',))
condition=require('?OnConditionCheck@condition_fuzan_summerpetal_continuation@@',(),(109355,22,6,7))
factory_prefix='?GetAI@?$GenericCreatureScript@Unpc_fuzan_summerpetal@@'
factory=require(factory_prefix,('??0SmartAI@@',))
registration=n['instructions']('?AddSC_instance_temple_of_the_jade_serpent@@')
assert n['symbol'](factory_prefix) in prior['installed_functions'](registration),'Actual maiden registration absent'
assert n['symbol']('?OnConditionCheck@condition_fuzan_summerpetal_continuation@@') in prior['installed_functions'](registration),'Actual condition vtable registration absent'
tables=[]
for address in prior['references'](factory):
    try:
        first,=struct.unpack_from('<Q',n['binary'],n['offset'](address))
        if n['base']<=first<n['base']+0x10000000 and n['resolve'](first)==n['symbol']('??_Gnpc_fuzan_summerpetal@@'):
            tables.append([n['resolve'](value) if n['base']<=value<n['base']+0x10000000 else value
                           for value in struct.unpack_from('<128Q',n['binary'],n['offset'](address))])
    except (ValueError,struct.error):pass
assert len(tables)==1,'Actual maiden AI vtable ambiguous'
for method in ('InitializeAI','Reset','IsSummonedBy','SetData','UpdateAI'):
    assert tables[0].count(n['symbol']('?'+method+suffix))==1,('Actual virtual hook',method)
assert len(sites(n,resume,'?ProcessEvent@SmartScript@@'))==2,'Retry only original King/follow links'
for body in (capture,owner,update,resume,condition):
    for target in ('?UpdateAchievementCriteria@Player@@','?UpdateAchievementCriteria@Scenario@@','?SetCurrentStep@Scenario@@',
                   '?SummonCreature@WorldObject@@','?AddItem@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@',
                   '?SetFollow@SmartAI@@','?BindQuestObject@Player@@'):
        assert not sites(n,body,target),('Preserve native grants/follow and reconnect lifetime',target)
receipt=require('?GetQuestCancellationReceipt@Player@@',('??$make_shared@UQuestObjectScope@@',))
assert any(op.mnemonic=='lock inc' for op in receipt),'Native receipt must retain its shared lifetime'
print('PASS linked Summerpetal: registered factory/five virtual hooks, real source/victory/attempt capture, original SmartAI speech timers and two native continuation links, exact condition scopes; no direct progress, replacement summon/follow or quest-bound despawn.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'summerpetal_selected_functions':9,'native_timer_dispatches':len(sites(n,update,'?UpdateAI@SmartAI@@'))}))
