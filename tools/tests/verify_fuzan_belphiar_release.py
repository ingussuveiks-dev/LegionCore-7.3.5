"""Inspect the registered scoped boss gate and retained native death dispatch."""
from pathlib import Path
import hashlib
import json
import runpy
import struct

prior=runpy.run_path(str(Path(__file__).with_name('verify_fuzan_temple_door_release.py')))
n,require=prior['n'],prior['require']
references,installed_functions=prior['references'],prior['installed_functions']
sites,values=prior['sites'],prior['values']
suffix='@npc_fuzan_belphiar@@'
blocked=require('?Blocked'+suffix,('?GetScenario@ScenarioMgr@@','?GetScenarioId@Scenario@@'),(109397,373405,1625,1137,2))
gate=require('?UpdateGate'+suffix,('?Blocked'+suffix,'?HasFlag@Object@@','?GetReactState@Creature@@','?SetFlag@Object@@',
    '?SetReactState@Creature@@','?isInCombat@Unit@@','?CombatStop@Unit@@','?RemoveFlag@Object@@'),(97,2))
pairs=lambda body:[(op.mnemonic,op.op_str) for op in body]
for operation in [('mov','byte ptr [rbx + 0x258], 1'),('mov','byte ptr [rbx + 0x259], al'),
                  ('mov','byte ptr [rbx + 0x25a], al'),('cmp','byte ptr [rbx + 0x259], 0'),
                  ('movzx','edx, byte ptr [rbx + 0x25a]'),('mov','byte ptr [rbx + 0x258], 0')]:
    assert operation in pairs(gate),('Owned flag/reaction lifecycle',operation)
assert len(sites(n,gate,'?SetFlag@Object@@'))==len(sites(n,gate,'?RemoveFlag@Object@@'))==1
raw=require('?DamageTaken'+suffix,('?UpdateGate'+suffix,'?DamageTaken@SmartAI@@'))
final=require('?DamageTakenAfterScaling'+suffix,('?UpdateGate'+suffix,))
assert ('mov','dword ptr [rbx], 0') in pairs(final)
assert pairs(final)[4][0]=='test' and pairs(final)[5][0]=='je','Retain guarded zero damage and ordinary fallthrough'
attack=require('?AttackStart'+suffix,('?UpdateGate'+suffix,'?AttackStart@SmartAI@@'))
admission=require('?CanAIAttack'+suffix,('?Blocked'+suffix,))
assert ('xor','al, 1') in pairs(admission)
initialize=require('?InitializeAI'+suffix,('?InitializeAI@SmartAI@@','?UpdateGate'+suffix))
reset=require('?Reset'+suffix,('?Reset@SmartAI@@','?UpdateGate'+suffix))
update=require('?UpdateAI'+suffix,('?UpdateGate'+suffix,'?UpdateAI@SmartAI@@'))
assert sites(n,update,'?UpdateGate'+suffix)[0]<sites(n,update,'?UpdateAI@SmartAI@@')[0]
destructor=require('??_Gnpc_fuzan_belphiar@@',('?RemoveFlag@Object@@','?SetReactState@Creature@@','??1SmartAI@@'),(97,2))
assert ('cmp','byte ptr [rcx + 0x259], 0') in pairs(destructor) and ('mov','byte ptr [rbx + 0x258], 0') in pairs(destructor)
factory_prefix='?GetAI@?$GenericCreatureScript@Unpc_fuzan_belphiar@@'
factory=require(factory_prefix,('??0SmartAI@@',))
registration=n['instructions']('?AddSC_instance_temple_of_the_jade_serpent@@')
assert n['symbol'](factory_prefix) in installed_functions(registration),'Actual registration missing'
tables=[]
for address in references(factory):
    try:
        first,=struct.unpack_from('<Q',n['binary'],n['offset'](address))
        if n['base']<=first<n['base']+0x10000000 and n['resolve'](first)==n['symbol']('??_Gnpc_fuzan_belphiar@@'):
            tables.append([n['resolve'](value) if n['base']<=value<n['base']+0x10000000 else value
                           for value in struct.unpack_from('<128Q',n['binary'],n['offset'](address))])
    except (ValueError,struct.error):
        continue
assert len(tables)==1,'Actual constructed AI vtable is ambiguous'
slots=tables[0]
for method in ('InitializeAI','Reset','UpdateAI','CanAIAttack','AttackStart','DamageTaken','DamageTakenAfterScaling'):
    assert slots.count(n['symbol']('?'+method+suffix))==1,('Actual virtual dispatch',method)
death=require('?JustDied'+suffix,('?GetQuestCancellationReceipt@Player@@','?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@','?JustDied@SmartAI@@'),(42765,109397,373405,1625))
assert slots.count(n['symbol']('?JustDied'+suffix))==1,'Actual death wrapper must snapshot the quest attempt'
assert len(sites(n,death,'?JustDied@SmartAI@@'))==1,'Keep one original native death/205/summon dispatch'
assert max(sites(n,death,'?GetQuestCancellationReceipt@Player@@'))<sites(n,death,'?JustDied@SmartAI@@')[0],'Snapshot before native reentrant callbacks'
for body in (blocked,gate,raw,final,attack,admission,initialize,reset,update,destructor,death):
    for target in ('?UpdateAchievementCriteria@Player@@','?UpdateAchievementCriteria@Scenario@@','?SetCurrentStep@Scenario@@',
                   '?SummonCreature@WorldObject@@','?AddItem@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?SetHealth@Unit@@'):
        assert not sites(n,body,target),('No manufactured progression/health/summon',target)
print('PASS linked Belphiar: actual registered factory/8 virtual hooks, quest-attempt snapshot before one native SmartAI JustDied, exact109397/373405/1625/1137/step2 scope, owned flag/reaction cleanup including destruction, early/final damage guard and ordinary native combat; no direct progression grant.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'belphiar_functions':12,
                  'final_damage_slot':slots.index(n['symbol']('?DamageTakenAfterScaling'+suffix)),
                  'native_death_wrapper_slot':slots.index(n['symbol']('?JustDied'+suffix))}))
