"""Follow Lady Calindris's actual factory, scaled hook and actor-owned callbacks."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM

p = runpy.run_path(str(Path(__file__).with_name('verify_tuure_native_pickup_release.py')))
n, instructions, references, values = p['n'], p['instructions'], p['references'], p['values']
scope = 'npc_lady_kalindriaAI@npc_lady_kalindria@@'
post, update, reset, hit = ('?' + method + '@' + scope for method in ('DamageTakenAfterScaling', 'UpdateAI', 'Reset', 'SpellHitTarget'))
factory = '?GetAI@npc_lady_kalindria@@'
slots = []
for address in references(factory):
    table = struct.unpack_from('<80Q', n['binary'], n['offset'](address))
    if table[0xa8 // 8] == n['symbol'](post):
        slots.append(table)
assert len(slots) == 1, 'Factory does not select the actual scaled guard'
assert slots[0][0xa0 // 8] == n['symbol']('?DamageTaken@UnitAI@@'), 'Old raw damage override remains'
assert slots[0][0x18 // 8] == n['symbol'](update)
assert n['symbol'](reset) in slots[0] and n['symbol'](hit) in slots[0]
for prefix in ('?Eligible@TuureDefeat@@', '?Participant@TuureDefeat@@', '?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@', '?GetHealth@Unit@@QEBA_KPEAV1@@', '?CountPctFromMaxHealth@Unit@@',
               '?UpdateAchievementCriteria@Player@@', '?ReleaseAllies@TuureDefeat@@', '?StopAttack@Creature@@', '?InterruptNonMeleeSpells@Unit@@'):
    matches = {n['resolve'](address) for name, address in n['symbols'].items() if name.startswith(prefix)}
    assert matches & n['calls'](post), f'Missing actual defeat dependency {prefix}'
assert {3000, 50695, 210715, 2015, 13, 9} <= values(post)
assert {106318, 1489, 41993, 1085, 5} <= values('?Eligible@TuureDefeat@@')
# Recipient is inlined into the selected final guard in this MSVC Release build.
assert {106134, 106137, 106253} <= values(post)
assert n['symbol']('?Participant@TuureDefeat@@') in n['calls'](update)
for method in (post, update, '?Participant@TuureDefeat@@'):
    assert not {n['resolve'](a) for name, a in n['symbols'].items() if name.startswith('?FindNearestPlayer@')} & n['calls'](method)
assert {210703, 210807} <= values('?ReleaseAllies@TuureDefeat@@')
for method in (reset, hit, '?ReleaseAllies@TuureDefeat@@'):
    assert n['symbol']('?RemoveAurasDueToSpell@Unit@@') in n['calls'](method) or (
        method == reset and n['symbol']('?ReleaseAllies@TuureDefeat@@') in n['calls'](reset))
callbacks = [name for name in n['symbols'] if name.startswith('?_Do_call@') and 'DamageTakenAfterScaling@'+scope in name]
assert len(callbacks) == 1
death = callbacks[0]
assert n['symbol']('?GetUnit@ObjectAccessor@@') in n['calls'](death)
assert n['symbol']('?Kill@Unit@@') in n['calls'](death)
assert not n['symbol']('?UpdateAchievementCriteria@Player@@') in n['calls'](death), 'Physical cleanup duplicates victory'
speech = [name for name in n['symbols'] if name.startswith('?_Do_call@') and 'UpdateAI@'+scope in name]
assert len(speech) == 1 and 2 in values(speech[0])
assert any(name.startswith('?SendChat@CreatureTextMgr@@') and n['resolve'](a) in n['calls'](speech[0]) for name,a in n['symbols'].items())
print('PASS selected Lady factory uses native post-scaled guard, actual participant/owner/native50695, original3000ms actor queue and real Kill fallback, scoped strangle removal and native delayed text; old raw/nearest-player hooks absent. Native host owns lifetime/reset/retry and vanished-killer tests; client fight pending.')
