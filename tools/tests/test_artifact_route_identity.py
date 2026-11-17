"""Exercise production route identity with actual 26972 metadata and no shop offers."""
import argparse
import csv
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/research'))
from audit_artifact_routes import resolve_artifact
from audit_spell_mechanics import Data

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--world', type=Path, required=True)
parser.add_argument('--items', type=Path, required=True)
parser.add_argument('--report', type=Path, required=True)
args = parser.parse_args()
catalog = {int(row['ID']): row for row in csv.DictReader(args.items.open(encoding='utf-8-sig'), delimiter='\t')}
effects = Data(ROOT / 'build-extractors/bin/Release/dbc/enUS', Path('__native_only__')).rows('SpellEffect')
targets = {row['EffectMiscValue1'] for row in effects.values() if row['Effect'] == 249}
quests = {row['ID']: row['LogTitle'] for row in map(json.loads, (args.world / 'quest_template_locale.jsonl').read_text(encoding='utf-8-sig').splitlines())}
report = json.loads(args.report.read_text(encoding='utf-8-sig'))
expected = json.loads((ROOT / 'docs/audits/artifact-routes-2026-10-23.json').read_text())
assert len(report['routes']) == 36 and len(report['native_race_class_pairs']) == 149
assert {r['response']: r['item'] for r in report['routes']} == {r['response']: r['item'] for r in expected['routes']}
assert not any(r['shop_offer_present'] for r in report['routes']), 'Fresh post-removal snapshot must have no artifact store dependency'
cases = 0
for route in report['routes']:
    title = quests[route['chosen']]
    assert resolve_artifact(catalog, targets, route['class_id'], title) == route['item']
    # An unused/dummy native record with the same name cannot become the
    # acquisition item merely because it is present in sparse Item data.
    alias = 990001
    altered = dict(catalog)
    altered[alias] = dict(catalog[route['item']], ID=str(alias))
    assert resolve_artifact(altered, targets, route['class_id'], title) == route['item']
    for choices in (targets - {route['item']}, targets | {alias}):
        try:
            resolve_artifact(altered, choices, route['class_id'], title)
        except ValueError:
            pass
        else:
            raise AssertionError('Missing/ambiguous native identity must remain a visible error')
    cases += 4
print(f'PASS all36 fresh route identities,149 native race/class pairs and{cases} identity/dummy/missing/ambiguity controls; zero artifact Shop offers; no live records changed')
