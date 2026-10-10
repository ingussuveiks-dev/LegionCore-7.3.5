import sys,json,struct,re
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/progression-audit')
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in (197680,197487,130491,207140,305067)]
def effect(spell,kind,key,value):
    assert any(r['SpellID']==spell and r['Effect']==kind and r[key]==value for r in fx),(spell,kind,key,value)
effect(197680,6,'EffectMiscValue1',1146)
effect(197487,90,'EffectMiscValue1',92742)
effect(197487,252,'ImplicitTarget2',17)
effect(130491,6,'EffectAura',4)
effect(207140,134,'EffectMiscValue1',103022)
effect(207140,167,'ImplicitTarget2',31)
effect(305067,28,'EffectMiscValue1',103022) # existing custom overlay, not a native spell
assert 67955 in d.rows('CreatureDisplayInfo')
native=json.loads((ROOT/'docs/audits/valsharah-chain-native-2026-10-10.json').read_text())
objectives=[r for r in native['objectives'] if r['QuestID'] in (38377,38675,41724,41708,41890)]
assert len(objectives)==7
assert {r['ObjectID'] for r in objectives}=={92742,103022,104645}
p=ROOT/'build-extractors/bin/Release/dbc/enUS/SceneScriptText.db2';b=p.read_bytes()
lo=struct.unpack_from('<I',b,28)[0];catalog=struct.unpack_from('<I',b,60)[0]
off,size=struct.unpack_from('<IH',b,catalog+6*(13744-lo));name,script,*_=b[off:off+size].split(b'\0');script=script.decode()
code=re.sub(r'--[^\n]*','',script)
seconds=sum(float(n) for n in re.findall(r'\bWait\s*\(\s*([\d.]+)\s*\)',code))
assert seconds==28.5 and 'EndScene()' in code
out=dict(objectives=objectives,spell_effects=fx,custom_overlay_spell=305067,scene_text_id=13744,scene_name=name.decode(),scene_text=script,native_duration_seconds=seconds,mount_display=67955,runtime_hashes=d.hashes)
if '--write-evidence' in sys.argv:(ROOT/'docs/audits/valsharah-rituals-native-2026-10-10.json').write_text(json.dumps(out,indent=2)+'\n')
print('PASS: seven native objectives, 28.5-second revised scene, native credit/teleport and kneeling aura, area-credit hazard, existing custom escort summon and valid mount display.')
