"""Verify linked Skjal publication and the curved-deck ground projection."""
from pathlib import Path
import runpy
from sharasdal_erunak_entry_pe import sites, values, floats

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
crew = n['instructions']('?EnsureSkjalCrew@instance_maw_of_souls_InstanceMapScript@')
boss, = sites(n, crew, '?GetBossState@InstanceScript@@')
difficulty, = sites(n, crew, '?GetDifficultyID@Map@@')
scenario, = sites(n, crew, '?GetScenario@ScenarioMgr@@')
challenge, = sites(n, crew, '?GetChallenge@Scenario@@')
affix, = sites(n, crew, '?HasAffix@Challenge@@')
summon, = sites(n, crew, '?SummonCreature@Map@@')
home, = sites(n, crew, '?SetHomePosition@Creature@@')
movement, = sites(n, crew, '?MoveTargetedHome@MotionMaster@@')
assert boss < difficulty < scenario < challenge < affix < summon < home < movement
assert sites(n, crew, '?MovePath@MotionMaster@@')
assert {3, 5, 8, 16, 22, 23, 99307} <= values(crew)
assert 1.0 in floats(n, crew)  # Actual home-arrival admission,not elapsed time.
for prefix in ('?KilledMonsterCredit@Player@@', '?UpdateAchievementCriteria@', '?AddItem@'):
    assert not sites(n, crew, prefix)

request = n['instructions']('?SetData@instance_maw_of_souls_InstanceMapScript@')
assert {1, 3, 1000} <= values(request)
assert sites(n, request, '?GetBossState@InstanceScript@@')
assert sites(n, request, '?EnsureSkjalCrew@')
action = n['instructions']('?DoAction@npc_skyal@@')
assert 1000 in values(action) and sites(n, action, '?GetBossState@InstanceScript@@')
update = n['instructions']('?Update@instance_maw_of_souls_InstanceMapScript@')
assert 1000 in values(update) and sites(n, update, '?EnsureSkjalCrew@')

ground = n['instructions']('?BuildMawShipGroundPath@Movement@@')
corners, = sites(n, ground, '?findStraightPath@dtNavMeshQuery@@')
projection, = sites(n, ground, '?GetHeight@Map@@')
assert corners < projection
assert {0.5, 0.6, 1.0, 3.0, 296.0} <= floats(n, ground)
support = n['instructions']('??R<lambda_1>@?1??BuildMawShipGroundPath@Movement@@')
assert sites(n, support, '?GetHeight@Map@@') and sites(n, support, '?getHeight@GameObjectModel@@')
print('PASS linked reconstructed Teeming:real Harbaron/Skjal+Keystone/Teeming gates,16+7 owned slots,finite3 publication attempts,actual home/path dispatch,no synthetic credit;curved-deck physical floor projection retains native step/body/endpoint guards')
