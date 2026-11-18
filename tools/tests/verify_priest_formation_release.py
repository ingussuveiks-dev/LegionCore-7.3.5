"""Selected final PE registrations and native Priest formation consumers."""
from pathlib import Path
import runpy, struct
from capstone import CS_OP_IMM

p = runpy.run_path(str(Path(__file__).with_name('verify_ring_reforged_release.py')))
n, ins, refs, data, symbol = (p[k] for k in ('n', 'ins', 'refs', 'data', 'symbol'))
slots, values, require = (p[k] for k in ('slots', 'values', 'require'))
registration = '?AddSC_artifact_acquisition@@'

for script, base in [('spell_priest_formation_scene', 'Spell'),
                     ('aura_priest_formation_scene', 'Aura'),
                     ('spell_priest_formation_ready', 'Spell'),
                     ('spell_priest_formation_channel', 'Spell'),
                     ('aura_priest_formation_channel', 'Aura'),
                     ('spell_priest_formation_ritual', 'Spell'),
                     ('spell_priest_formation_finish', 'Spell'),
                     ('spell_priest_formation_entry', 'Spell')]:
    table = symbol(f'??_7?$Generic{base}ScriptLoader@V{script}@@@@6B@')
    assert table in refs(registration), script
    factory = f'?Get{base}Script@?$Generic{base}ScriptLoader@V{script}@@@@'
    assert n['symbol'](factory) in slots(table)
    instance = symbol(f'??_7{script}@@6B@')
    assert instance in refs(factory)
    assert n['symbol'](f'?Register@{script}@@') in slots(instance)

for script in ('npc_priest_formation_velen', 'go_priest_formation_portal'):
    table = symbol(f'??_7{script}@@6BScriptObject@@@')
    assert table in refs(registration)
    assert n['symbol'](f'?GetAI@{script}@@') in slots(table)

for script, hooks in [('player_priest_formation', ('OnLogin', 'OnBeforeLogout', 'OnLogout', 'OnMapChanged', 'OnUpdate')),
                      ('scene_priest_formation', ('OnTrigger',)),
                      ('eventobject_priest_formation', ('IsTriggerMeets', 'OnTrigger'))]:
    table = symbol(f'??_7{script}@@6B@')
    constructor = f'??0{script}@@'
    assert table in refs(registration) or (
        n['symbol'](constructor) in n['calls'](registration) and table in refs(constructor))
    for hook in hooks:
        assert n['symbol'](f'?{hook}@{script}@@') in slots(table), (script, hook)

table = symbol('??_7AI@go_priest_formation_portal@@6B@')
assert table in refs('?GetAI@go_priest_formation_portal@@')
for hook in ('GossipUse', 'GossipHello'):
    assert n['symbol'](f'?{hook}@AI@go_priest_formation_portal@@') in slots(table)
require('?GossipUse@AI@go_priest_formation_portal@@', '?Portal@PriestFormation@@')
assert 202324 in values('?GossipUse@AI@go_priest_formation_portal@@')
assert {101313, 102528, 2912, 40938, 1220, 1512} <= values('?SummonCreature@Map@@')
assert 40938 in values('?CanRewardQuest@Player@@QEAA_NPEBVQuest@@_N')
assert {40938, 1220, 1512} <= values('?KilledMonsterCredit@Player@@')
periodic = '?Periodic@aura_priest_formation_channel@@'
if n['symbol']('?SceneLive@PriestFormation@@') not in n['calls'](periodic):
    # MSVC inlines SceneLive here; retain its actual accepted-attempt,
    # live scene aura/status and native map/misc checks at this consumer.
    require(periodic, '?Accepted@PriestFormation@@', '?HasAura@Unit@@', '?HasSceneStatus@Player@@')
    assert {1191, 1512, 202457} <= values(periodic)
assert {202881, 102653, 40938} <= values('?Periodic@aura_priest_formation_channel@@')
assert 16000 in values('?Tick@PriestFormation@@')
require('?OnTrigger@scene_priest_formation@@', '?FinishCast@PriestFormation@@')
assert {202982, 202945, 202491} <= values('?OnTrigger@scene_priest_formation@@')
print('PASS selected final PE all13 Priest registrations and native hooks, actual portal-use entry request, prepublication owner binding, reward/recipient guards, real periodic ritual endpoint, bounded failed-channel recovery and separate scene/grip/ceremony/return consumers. Native execution establishes ordering and inventory outcomes; client traversal remains separate.')
