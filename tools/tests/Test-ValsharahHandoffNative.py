import sys,json,struct,hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/progression-audit')
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in(206723,228531,218440,203477)]
assert any(r['SpellID']==228531 and r['Effect']==45 and r['EffectMiscValue1']==473 for r in fx)
assert any(r['SpellID']==206723 and r['Effect']==90 and r['EffectMiscValue1']==92804 for r in fx)
assert any(r['SpellID']==206723 and r['EffectTriggerSpell']==228531 for r in fx)
assert any(r['SpellID']==218440 and r['EffectAura']==430 and r['EffectMiscValue1']==1350 for r in fx)
assert any(r['SpellID']==203477 and r['Effect']==28 and r['EffectMiscValue1']==102952 for r in fx)
p=ROOT/'build-extractors/bin/Release/dbc/enUS/SceneScriptText.db2';b=p.read_bytes()
lo=struct.unpack_from('<I',b,28)[0];catalog=struct.unpack_from('<I',b,60)[0]
off,size=struct.unpack_from('<IH',b,catalog+6*(14296-lo));name,script,*_=b[off:off+size].split(b'\0');script=script.decode('utf8')
assert 'scene:TriggerServerEvent( "TYRANDE" )' in script and 'scene:EndScene()' in script
out=dict(spell_effects=fx,scene_text_id=14296,scene_name=name.decode('utf8'),scene_text=script,scene_text_sha256=hashlib.sha256(b).hexdigest(),runtime_hashes=d.hashes)
if '--write-evidence' in sys.argv:(ROOT/'docs/audits/valsharah-handoff-native-2026-10-10.json').write_text(json.dumps(out,indent=2)+'\n')
print('PASS: native movie 473, search credit 92804, follow-up scene 1350, TYRANDE server event and companion summon 203477.')
