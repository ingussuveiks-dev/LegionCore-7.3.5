"""Trace native rune/bulwark and banishment effects/visual relationships."""
import hashlib
import json
import re
import sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table
data=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
ids={208544,208547,209177,208811,208815,209241}
keys=('ID','SpellID','EffectIndex','Effect','EffectAura','EffectBasePoints','EffectMiscValue1','EffectMiscValue2','EffectTriggerSpell','ImplicitTarget1','ImplicitTarget2')
effects=[{k:r[k] for k in keys} for r in data.rows('SpellEffect').values() if r['SpellID'] in ids]
assert any(r['SpellID']==208544 and r['EffectAura']==40 and r['EffectMiscValue1']==127 for r in effects)
assert any(r['SpellID']==208544 and r['EffectAura']==23 and r['EffectTriggerSpell']==208547 for r in effects)
assert all(any(r['SpellID']==i and r['EffectIndex']==0 and r['ImplicitTarget2']==15 for r in effects) for i in (208547,209177))
assert any(r['SpellID']==209241 and r['EffectAura']==56 and r['EffectMiscValue1']==83693 for r in effects)
assert {201,321,40,60,67,254,278}<={r['EffectAura'] for r in effects if r['SpellID']==209241}
misc=[r for r in data.rows('SpellMisc').values() if r['SpellID'] in ids]
banish=next(r for r in misc if r['SpellID']==209241)
duration=data.rows('SpellDuration')[banish['DurationIndex']]
assert duration['Duration']==duration['MaxDuration']==20000
visuals=[r for r in data.rows('SpellXSpellVisual').values() if r['SpellID'] in ids]
assert any(r['SpellID']==209241 and r['SpellVisualID']==54974 for r in visuals)
def raw(name,fields):
 meta=re.split('struct '+name+'Meta\n',data.meta,flags=re.I)[1].split('\n};')[0]
 idx,layout=re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[\dA-Fa-f]+)',meta).groups()
 t=Table(data.dbc/(name+'.db2'),int(idx),int(layout,16));data.hashes[name]=t.sha256
 return [dict(ID=t.ids[i],parent=t.parents.get(i,0),**{k:t.value(i,f) for k,f in fields.items()}) for i in range(t.count)]
events=[r for r in raw('SpellVisualEvent',{'start':0,'end':3,'target':6,'kit':7}) if r['parent'] in {v['SpellVisualID'] for v in visuals}]
kits={r['kit'] for r in events if r['kit']};allkits=data.rows('SpellVisualKit')
for _ in range(10):
 added={allkits[k]['FallbackSpellVisualKitID'] for k in kits if allkits[k]['FallbackSpellVisualKitID']}
 if added<=kits:break
 kits|=added
kitEffects=[r for r in raw('SpellVisualKitEffect',{'type':0,'effect':1}) if r['parent'] in kits]
attachments={r['ID']:r for r in raw('SpellVisualKitModelAttach',{'effect_name':3,'attachment':4,'start_anim':15,'anim':16,'end_anim':17,'anim_kit':18})}
attached=[attachments[r['effect']] for r in kitEffects if r['type']==2]
names={r['ID']:r for r in raw('SpellVisualEffectName',{'model_file':10,'texture_file':8})}
models=[names[r['effect_name']] for r in attached]
assert any(r['parent']==54974 for r in events)
out=dict(effects=effects,misc=misc,banishment_duration=duration,visuals=visuals,visual_events=events,
 kits=[allkits[k] for k in sorted(kits)],kit_effects=kitEffects,attachments=attached,effect_models=models,
 runtime_hashes=data.hashes,
 limits='Native regular26972 visual relationships use the established extra ModelAttach relationship mapping omitted from its disabled core store. The full native209241 spell plays its own transform/fly/no-actions/visual and expires after20s; the script removes only a self-cast copy it actually applied before map transfer or cancellation. Native C++ DB2Storage separately validates transform display61276/model1731/file124640 via World creature83693, rune65123/model8446/file1139506, five current dialogue line IDs and seven sound kits. This is reference/handler evidence, not model, animation, voice, movement, camera or pet portrait certification in a client. Rune thresholds70/35percent, one-hit attack proxies and dialogue pauses are authored choices; no claim of exact retail combat tuning or film timing.')
path=ROOT/'docs/audits/stormheim-faljar-runestones-native-2026-10-15.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(path.read_text())
print('PASS: native self Bulwark/immunity/ticks, both area selectors, rune visuals and20s banishment controls/visual54974 -> kit/attachment/model/animation references. Client playback remains pending.')
