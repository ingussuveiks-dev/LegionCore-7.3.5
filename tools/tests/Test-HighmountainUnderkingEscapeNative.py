import sys,json,hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/escape-no-overlays')
effects=[r for r in d.rows('SpellEffect').values() if r['SpellID']==192468]
assert len(effects)==1 and effects[0]['Effect']==90 and effects[0]['EffectMiscValue1']==0
misc=next(r for r in d.rows('SpellMisc').values() if r['SpellID']==192468)
cast=d.rows('SpellCastTimes')[misc['CastingTimeIndex']]
assert cast['Base']==1300
visuals=[r for r in d.rows('SpellXSpellVisual').values() if r['SpellID']==192468]
assert len(visuals)==1 and visuals[0]['SpellVisualID']==49855
# SpellVisualEvent has an implicit parent column omitted by this core's load info.
t=Table(d.dbc/'SpellVisualEvent.db2',layout=0xAE75BC3C)
events=[dict(id=id,values=[t.value(i,j) for j in range(t.fields)]) for i,id in enumerate(t.ids) if t.parents.get(i)==49855]
assert {r['values'][7] for r in events}=={59128,59129}
kits={id:d.rows('SpellVisualKit')[id] for id in (59128,59129)}
source=(ROOT/'src/server/scripts/Legion/highmountain_underking_escape.cpp').read_text()
assert 'OnEffectHitTarget +=' in source and 'SPELL_EFFECT_KILL_CREDIT' in source and 'PreventHitDefaultEffect(index)' in source
out={'effects':effects,'cast_time':cast,'visuals':visuals,'visual_events':events,'kits':kits,'db2_hashes':dict(d.hashes,SpellVisualEvent=t.sha256)}
path=ROOT/'docs/audits/highmountain-underking-escape-native-2026-10-13.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(path.read_text())
print('PASS: original escape spell 192468, 1.3 s cast, visual 49855 and cast/impact kits 59129/59128; zero-entry credit suppressed.')
