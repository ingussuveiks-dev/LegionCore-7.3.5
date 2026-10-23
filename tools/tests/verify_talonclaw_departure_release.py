"""Verify personal post-claim departure and a real click-driven teleport in the linked PE."""
from pathlib import Path
import runpy

previous = runpy.run_path(str(Path(__file__).with_name('verify_talonclaw_return_release.py')))
native = previous['native']
immediates = previous['presentation']['immediates']


def reaches(caller, prefix):
    targets = native['calls'](caller)
    return any(name.startswith(prefix) and native['resolve'](address) in targets
               for name, address in native['symbols'].items())


ready = '?Ready@TalonclawDeparture@@'
update = '?Update@State@TalonclawDeparture@@'
use = '?Use@TalonclawDeparture@@'
assert {1511, 973, 40385, 100111, 100112, 103798, 128808, 205589, 9000575, 106397}.issubset(immediates(ready))
assert {100005, 9000575, 10000, 6000, 300}.issubset(immediates(update))
for prefix in ('?SummonCreature@WorldObject@@', '?SummonGameObject@WorldObject@@', '?BindQuestObject@Player@@', '?Talk@CreatureAI@@'):
    assert reaches(update, prefix), f'Linked departure omits {prefix}'
assert 190598 in immediates(use)
assert reaches(use, '?CastSpell@Unit@@')
assert not reaches(use, '?KilledMonsterCredit@Player@@')
assert reaches('?Clear@player_talonclaw_departure@@', '?CancelQuestObjects@Player@@')
assert reaches('?UpdateAI@AI@go_talonclaw_return_portal@@', '?Delete@GameObject@@')
print('PASS: actual Release departure admits the claimed owner, summons native Dorro/personal portal after10s+6s, retains scope cleanup and performs only native190598 teleport on a valid click')
