"""Native quest, summon actor and control/passenger seat evidence for39652."""
import json
import sys
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/research'))
from audit_spell_mechanics import Data

prior = json.loads((ROOT / 'docs/audits/stormheim-heart-native-2026-10-14.json').read_text())
quest = next(q for q in prior['quests'] if q['ID'] == '39652')
objectives = sorted((q for q in prior['objectives'] if q['QuestID'] == '39652'), key=lambda q: int(q['StorageIndex']))
assert [(q['ObjectID'], q['Type'], q['StorageIndex'], q['Flags'], q['Amount']) for q in objectives] == [
    ('96466', '0', '0', '0', '1'), ('96467', '0', '1', '2', '1')]
assert quest['RewardNextQuest'] == '38624'
d = Data(ROOT / 'build-extractors/bin/Release/dbc/enUS', ROOT / '.codex/stormheim-no-overlays')
effects = [r for r in d.rows('SpellEffect').values() if r.get('SpellID') == 182921]
assert len(effects) == 1 and effects[0]['Effect'] == 28 and effects[0]['EffectMiscValue1'] == 92302
vehicles = [d.rows('Vehicle')[i] for i in (4133, 4367)]
seats = [d.rows('VehicleSeat')[v['SeatID1']] for v in vehicles]
assert [v['SeatID1'] for v in vehicles] == [15388, 15925]
assert seats[0]['Flags'] & 0x800 and not seats[1]['Flags'] & 0x800
assert all(s['AttachmentID'] == 21 and s['RideAnimLoop'] == 91 for s in seats)
out = {'original_TDB_reference': 'stormheim-heart-native-2026-10-14.json',
       'archive_sha256': prior['archive_sha256'], 'quest': quest, 'objectives': objectives,
       'ride_spell': effects, 'vehicles': vehicles, 'first_seats': seats,
       'runtime_hashes': d.hashes,
       'limits': 'Native182921 identifies actor92302. Its default4133 has a control seat. The personal scripted ride explicitly overrides only its own vehicle kit to4367, taken from this quest giver96465, using native non-control seat15925. Neither template vehicle is edited. Direct summon replaces the old teleport; this is an authored repair, not a recovered retail path. Original TDB reward141399x1 differs from installed0x0; this change preserves installed rewards. Models/visual metadata are retained in the referenced Heart audit; client seat, camera, wings, pet frame and flight remain unverified.'}
path = ROOT / 'docs/audits/stormheim-dragons-native-2026-10-15.json'
if '--write-evidence' in sys.argv:
    path.write_text(json.dumps(out, indent=2) + '\n')
else:
    assert json.loads(json.dumps(out)) == json.loads(path.read_text())
print('PASS: native39652 stages/next, Ride Vethir summon92302 and contrasting control/passenger seats4133/4367.')
