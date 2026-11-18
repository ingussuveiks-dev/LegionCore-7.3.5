"""Confirm the existing native Explosive chain in the canonical x64 PE."""
from pathlib import Path
import runpy

from sharasdal_erunak_entry_pe import sites, values, floats

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
summoned = n['instructions']('?IsSummonedBy@npc_challenger_fel_explosives@@')
assert {240445, 500} <= values(summoned)
assert sites(n, summoned, '?DoCast@UnitAI@@')
assert sites(n, summoned, '?ScheduleEvent@EventMap@@')
update = n['instructions']('?UpdateAI@npc_challenger_fel_explosives@@')
assert 240446 in values(update) and sites(n, update, '?DoCast@UnitAI@@')
assert sites(n, update, '?ExecuteEvent@EventMap@@')
finish = n['instructions']('?SpellFinishCast@npc_challenger_fel_explosives@@')
assert {240446, 100} <= values(finish)
assert sites(n, finish, '?DespawnOrUnsummon@Creature@@')
spawn = n['instructions']('?OnUpdate@spell_challengers_might@@')
assert {243110, 8, 16} <= values(spawn) and 60.0 in floats(n, spawn)
for target in ('?GetAnyOwner@Unit@@', '?IsHostileToPlayers@Unit@@',
               '?IsInControl@CreatureAI@@', '?IsDungeonBoss@Creature@@',
               '?IsValidAttackTarget@Unit@@', '?CastSpell@Unit@@'):
    assert sites(n, spawn, target), target
print('PASS linked existing Explosive chain:500ms visual-to-event,actual240446 '
      'cast,100ms completion despawn,and Might243110 summon with8..16-second '
      'recurrence,60-yard eligible player,hostile/control/owner guards. '
      'Native behavior and actual client traversal remain separate.')
