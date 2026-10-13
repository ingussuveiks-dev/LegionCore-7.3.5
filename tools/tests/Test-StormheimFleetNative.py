import sys,json
from pathlib import Path
sys.dont_write_bytecode=True;root=Path(__file__).resolve().parents[2];sys.path.insert(0,str(root/'tools/research'))
from audit_spell_mechanics import Data
d=Data(root/'build-extractors/bin/Release/dbc/enUS',root/'.codex/stormheim-no-overlays')
ids={43671,184420,184427,186276,186304,187308,187309,185004,185614,185616,185613,185615,186528,228560,189626,189270,189630,184641,187776,187794}
effects=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in ids]
links=[r for r in d.rows('SpellXSpellVisual').values() if r['SpellID'] in ids]
visualIds={r['SpellVisualID'] for r in links};visuals=[r for r in d.rows('SpellVisual').values() if r['ID'] in visualIds]
out={'effects':effects,'visual_links':links,'visuals':visuals,'native_transport_path':[r for r in d.rows('TaxiPathNode').values() if r['PathID']==5130],
 'vehicles':{i:d.rows('Vehicle')[i] for i in(1439,4151,4283,4285,4286)},'movie_484':d.rows('Movie')[484], 'hashes':d.hashes}
assert len(out['native_transport_path'])==13 and all(r['ContinentID']==1475 for r in out['native_transport_path'])
for spell,teleport in ((185614,185613),(185616,185615)):
 assert any(r['SpellID']==spell and r['Effect']==45 and r['EffectMiscValue1']==484 for r in effects)
 assert any(r['SpellID']==spell and r['EffectAura']==23 and r['EffectTriggerSpell']==teleport for r in effects)
 assert any(r['SpellID']==spell and r['Effect']==90 and r['EffectMiscValue1']==113320 for r in effects)
assert any(r['SpellID']==189626 and r['EffectAura']==56 and r['EffectMiscValue1']==45382 and r['ImplicitTarget1']==1 for r in effects)
assert any(r['SpellID']==43671 and r['EffectAura']==236 for r in effects)
assert all(out['vehicles'][i]['SeatID1'] for i in (1439,4151,4283,4285,4286))
out['limits']='Native records/visual references only. Playback, vehicle/camera behavior and client pose are not verified by this audit.'
evidence=root/'docs/audits/stormheim-fleet-native-2026-10-13.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: installed 7.3.5 native transport path, five vehicles, ride control aura, weapon chains, native finale movie/teleport/credit effects and Greymane transform; recorded spell visual references.')