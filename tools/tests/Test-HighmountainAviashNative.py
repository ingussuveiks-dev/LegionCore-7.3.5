"""Identify Aviash's native landing points; does not claim a recovered flight path."""
import hashlib
import json
import math
from pathlib import Path
import struct
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table

# An absent overlay directory deliberately selects only extracted client records.
d = Data(ROOT / 'build-extractors/bin/Release/dbc/enUS', ROOT / '.codex/aviash-native-no-overlays')
assert not d.sql.exists(), 'Native evidence must not use hotfix overlays'
safe = d.rows('WorldSafeLocs')
landing = safe[5363]
witchqueen = safe[5364]
safe_table = Table(d.dbc / 'WorldSafeLocs.db2', layout=0x605EA8A6)
labels = {str(i): safe_table.name(safe_table.ids.index(i)) for i in (5363, 5364)}
assert labels == {'5363': '7.0 Highmountain: zto - Aviash drop off point',
                  '5364': '7.0 Highmountain: zto - Aviash drop off point (Witchqueen)'}
for row, expected in [(landing, (4226.38, 4421.39, 663.389, 203.758)),
                      (witchqueen, (5139.84, 4445.25, 1022.68, 203.919))]:
    assert row['MapID'] == 1220
    for key, value in zip(('LocX', 'LocY', 'LocZ', 'LocO'), expected):
        assert abs(row[key] - value) < 0.001, (key, row[key], value)

effects = [r for r in d.rows('SpellEffect').values() if r['SpellID'] == 194359]
assert any(r['Effect'] == 28 and r['EffectMiscValue1'] == 98778 and
           r['EffectMiscValue2'] == 3054 for r in effects)
props = d.rows('SummonProperties')[3054]
assert props['Control'] == 0 and props['Title'] == 0
vehicle = d.rows('Vehicle')[4428]
assert vehicle['SeatID1'] == 16028
seat = d.rows('VehicleSeat')[16028]

source = (5121.46, 4461.57, 1021.69)  # Existing spellclick spawn 340585.
dest = tuple(landing[k] for k in ('LocX', 'LocY', 'LocZ'))
locations = d.rows('Location')
path_nodes = d.rows('PathNode')
near_path_nodes = []
for node in path_nodes.values():
    loc = locations[node['LocationID']]
    point = tuple(loc[k] for k in ('Pos1', 'Pos2', 'Pos3'))
    if min(math.dist(point, source), math.dist(point, dest)) < 100:
        near_path_nodes.append(node['ID'])
# Path has no map field: this deliberately searches all maps, without inferring
# that a geographically nearby path would necessarily belong to this quest.
assert not near_path_nodes
taxi_nodes = d.rows('TaxiPathNode')
near_taxi_nodes = []
for node in taxi_nodes.values():
    point = tuple(node[k] for k in ('LocX', 'LocY', 'LocZ'))
    if node['ContinentID'] == 1220 and min(math.dist(point, source), math.dist(point, dest)) < 70:
        near_taxi_nodes.append(node['ID'])
assert not near_taxi_nodes

# SceneScriptText uses sparse inline strings. Validate the catalog before
# searching both record names and bodies; never parse it as regular WDC1.
b = (d.dbc / 'SceneScriptText.db2').read_bytes()
u = lambda offset: struct.unpack_from('<I', b, offset)[0]
assert b[:4] == b'WDC1' and struct.unpack_from('<H', b, 44)[0] & 1 and u(8) == 2
first, last, catalog = u(28), u(32), u(60)
assert catalog + (last - first + 1) * 6 <= len(b)
records = set()
scene_matches = []
for i in range(first, last + 1):
    offset, size = struct.unpack_from('<IH', b, catalog + (i - first) * 6)
    if not size:
        continue
    assert 84 <= offset < catalog and offset + size <= catalog
    fields = b[offset:offset + size].split(b'\0')
    assert len(fields) >= 3
    records.add((offset, size))
    text = b' '.join(fields[:2]).lower()
    if any(term in text for term in (b'aviash', b'39387', b'98778', b'194359', b'skies of highmountain')):
        scene_matches.append(i)
assert len(records) == u(4) and not scene_matches
d.hashes['SceneScriptText'] = hashlib.sha256(b).hexdigest()

report = {
    'version': 'Legion 7.3.5 extracted runtime data',
    'landing': landing,
    'witchqueen_landing': witchqueen,
    'landing_orientation_radians': math.radians(landing['LocO']),
    'corroborating_source': 'https://github.com/TrinityCore/TrinityCore/blob/978e4f5736d289e2d7d12a6a878dacc108fd5496/sql/old/8.x/world/19071_2020_06_17/2019_10_26_01_world.sql',
    'native_labels': labels,
    'summon_effects': effects,
    'summon_properties': props,
    'vehicle': vehicle,
    'seat': seat,
    'search': {'path_node_count': len(path_nodes), 'path_radius_m': 100,
               'near_path_nodes': near_path_nodes, 'taxi_path_node_count': len(taxi_nodes),
               'taxi_radius_m': 70, 'near_taxi_nodes': near_taxi_nodes,
               'unique_scene_record_count': len(records), 'scene_matches': scene_matches},
    'hashes': d.hashes,
    'flight_path_recovered': False,
}
evidence = ROOT / 'docs/audits/highmountain-aviash-native-2026-10-12.json'
if '--write-evidence' in sys.argv:
    evidence.write_text(json.dumps(report, indent=2) + '\n', encoding='utf8')
else:
    assert report == json.loads(evidence.read_text(encoding='utf8'))
print('PASS: native Aviash landing points, summon and vehicle; bounded route searches reproduced.')
