import sys,json,struct
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/progression-audit')
ids=(221373,221466,221375,221449,208444,208446,208292,208291,190406,190407,194213)
effects=d.rows('SpellEffect').values()
fx=[r for r in effects if r['SpellID'] in ids]
assert set(ids)=={r['SpellID'] for r in fx}
def effect(spell,kind,key,value):
    assert any(r['SpellID']==spell and r['Effect']==kind and r[key]==value for r in fx),(spell,kind,key,value)
for spell in (221373,221466,221375):effect(spell,3,'ImplicitTarget1',25)
effect(221449,219,'EffectMiscValue1',3137)
effect(208444,198,'EffectMiscValue1',1246)
effect(208446,134,'EffectMiscValue1',104764)
effect(194213,45,'EffectMiscValue1',472)
effect(208292,6,'EffectTriggerSpell',208291)
effect(208291,2,'ImplicitTarget1',104)
effect(190406,6,'EffectTriggerSpell',190407)
effect(190407,2,'ImplicitTarget2',15)
completion=[r for r in effects if r['Effect']==16 and r['EffectMiscValue1'] in (38687,41763,38743)]
assert not completion,completion
native=json.loads((ROOT/'docs/audits/valsharah-chain-native-2026-10-10.json').read_text())
objectives=[r for r in native['objectives'] if r['QuestID'] in (38687,41763,38743)]
assert len(objectives)==16
for quest,entries in ((38687,(104799,111258,111260,111259,104764)),(41763,(104799,111203,111198,111204,104764)),(38743,(104799,93065))):
    assert set(entries)<={r['ObjectID'] for r in objectives if r['QuestID']==quest}
p=ROOT/'build-extractors/bin/Release/dbc/enUS/SceneScriptText.db2';b=p.read_bytes()
lo=struct.unpack_from('<I',b,28)[0];catalog=struct.unpack_from('<I',b,60)[0]
off,size=struct.unpack_from('<IH',b,catalog+6*(14053-lo));name,script,*_=b[off:off+size].split(b'\0')
assert b'EndScene()' in script
out=dict(objectives=objectives,spell_effects=fx,scene_text_id=14053,scene_name=name.decode(),scene_text=script.decode(),runtime_hashes=d.hashes)
if '--write-evidence' in sys.argv:(ROOT/'docs/audits/valsharah-finale-native-2026-10-10.json').write_text(json.dumps(out,indent=2)+'\n')
print('PASS: native faction objectives, click pings, choice scene/credit, Ysera damage/channel effects and movie 472; no completion-spell event lock.')
