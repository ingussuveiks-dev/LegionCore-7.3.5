"""Check actual pet death health synchronization and revive admission in the PE."""
from pathlib import Path
import bisect
import re
import runpy

native = runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
def body(prefix):
    address = native['symbol'](prefix)
    index = bisect.bisect_right(native['starts'], address-native['base'])-1
    begin, end, unwind = native['functions'][index]
    assert begin <= address-native['base'] < end
    origin = native['root'](begin, unwind)
    for tail_begin,tail_end,tail_unwind in native['functions'][index+1:]:
        if tail_begin != end or native['root'](tail_begin,tail_unwind) != origin:
            break
        end = tail_end
    at = native['offset'](address)
    return native['binary'][at:at+native['base']+end-address]

death = body('?setDeathState@Pet@@')
current = [native['resolve'](address) for name,address in native['symbols'].items() if name.startswith('?GetCurrentPet@PetStable@@')]
assert not any(address in native['calls']('?setDeathState@Pet@@') for address in current), 'Death still writes through the current pet during loading'
# MSVC inlines UpdatePetHealth: native PetInfo PetNumber+0x40 and Health+0x50,
# active optional stride0x70 and unslotted record stride0x68. Both writes must
# be associated with identity comparisons, rather than a blanket null guard.
for pattern in (rb'\x39\x51\x40\x74', rb'\x39\x50\x40\x74', rb'\x44\x89\x71\x50', rb'\x44\x89\x70\x50', rb'\x48\x83\xc1\x70', rb'\x48\x83\xc0\x68'):
    assert re.search(pattern, death), 'Linked death handler lacks exact-record active/unslotted health update'
assert native['symbol']('?GetPetNumber@CharmInfo@@') in native['calls']('?setDeathState@Pet@@'), 'Missing persistent pet identity'
revive = body('?EffectResurrectPet@Spell@@')
assert re.search(rb'\x48\x85\xc9\x0f\x84', revive), 'Revive has no missing-stable admission guard'
assert re.search(rb'\x48\x3b\xc8\x0f\x84....\x48\x2b\xca', revive, re.DOTALL), 'Revive constructs a slot from end() when no dead active pet exists'
assert native['symbol']('?SummonPet@Player@@') in native['calls']('?EffectResurrectPet@Spell@@')
print('PASS: linked pet-number health updates for active/unslotted records and missing-stable/no-dead-pet revive guards')
