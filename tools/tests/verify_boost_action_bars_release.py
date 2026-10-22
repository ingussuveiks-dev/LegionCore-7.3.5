"""Check the deployed arranger and its real boost/departure callers in the PE."""
from pathlib import Path
import runpy

native = runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
symbol, calls = native['symbol'], native['calls']
arrange = symbol('?ArrangeBoostActionBar@CharacterService@@')
for caller in ('?Boost@CharacterService@@', '?FinalizeBoost@CharacterService@@', '?UpdateAI@DepartureAI@npc_boost_exit@@'):
    assert arrange in calls(caller), f'{caller} does not reach the new arranger'
for callee in ('?RemoveActionButton@Player@@', '?AddActionButton@Player@@', '?SendActionButtons@Player@@'):
    assert symbol(callee) in calls('?ArrangeBoostActionBar@CharacterService@@'), f'Arranger missing {callee}'
builders = [name for name in native['symbols'] if name.startswith('??$Build@') and 'ArrangeBoostActionBar@CharacterService' in name]
assert len(builders) == 1
assert symbol(builders[0]) in calls('?ArrangeBoostActionBar@CharacterService@@'), 'Not using the tested layout planner'
assert b'Boost action layout arranged for %s specialization %u (%zu actions)' in native['binary']
assert b'Boost action layout unavailable or full' in native['binary']
print('PASS: linked planner, action-state mutation and boost/finalization/departure callers; canonical implementation present')
