"""Select the actual registered Fury cemetery override and native dispatch."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM

n=runpy.run_path(str(Path(__file__).with_name('verify_warswords_arrival_release.py')))
native,instructions=n['native'],n['instructions']
suffix='@instance_warswords_InstanceMapScript@instance_warswords@@'
prefix='?GetClosestGraveYard'+suffix
address=native['symbol'](prefix)
table,=n['selected']
assert table.count(address)==1, 'Registered Fury instance does not select exactly one cemetery callback'
slot=table.index(address)*8
dispatch=instructions('?GetClosestGraveYard@InstanceMap@@')
assert any(x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM
           and x.operands[0].mem.disp==slot for x in dispatch), 'Native instance cemetery dispatch does not use selected override slot'
body=instructions(prefix)
values={v.imm for x in body for v in x.operands if v.type==CS_OP_IMM}
assert 944 in values and 1511 in values and (5228 in values or 5228*8 in values), 'Actual Fury scenario/native cemetery identity missing'
calls=native['calls'](prefix)
def needs(dependency):
    assert any(name.startswith(dependency) and native['resolve'](a) in calls
               for name,a in native['symbols'].items()), 'Cemetery callback missing '+dependency
needs('?GetScenario@ScenarioMgr@@')
needs('?IsValidMapCoord@MapManager@@')
assert b'Fury graveyard: invalid native WorldSafeLocs 5228 for instance %u' in native['binary'], 'Native data diagnostic removed'
for forbidden in ('?TeleportTo@Player@@','?ResurrectPlayer@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?KilledMonsterCredit@Player@@','?UpdateAchievementCriteria@Player@@','?SetCurrentStep@Scenario@@'):
    assert not any(name.startswith(forbidden) and native['resolve'](a) in calls for name,a in native['symbols'].items()), 'Cemetery callback performs unrelated progression or direct player mutation'
print('PASS selected registered Fury graveyard:actual944/1511/native5228 override selected by InstanceMap virtual dispatch,real scenario and coordinate validation/contextual diagnostic,retained native arrival controller. Source18/native64/release56 and full client remain separate.')
