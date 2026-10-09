"""Validate campaign IDs against the extracted 7.3.5 runtime; never fetch retail IDs."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table

p=argparse.ArgumentParser()
p.add_argument('--dbc', type=Path, default=Path('build-extractors/bin/Release/dbc/enUS'))
p.add_argument('--overlay', type=Path, default=Path('.codex/progression-audit'))
p.add_argument('--output', type=Path)
a=p.parse_args()
d=Data(a.dbc,a.overlay)
effects=d.rows('SpellEffect')
def effect(spell,kind,misc,aura=0):
    return any(r['SpellID']==spell and r['Effect']==kind and r['EffectAura']==aura and r['EffectMiscValue1']==misc for r in effects.values())
for args in [(158051,198,580),(158051,134,78290),(158282,134,78431),
             (158645,6,612,430),(158317,6,602,430),(169509,134,79434),(172784,6,875,430)]:
    assert effect(*args),args
assert d.rows('Map')[1374]['InstanceType']==5
lfg=d.rows('LFGDungeons')[870]
assert (lfg['MapID'],lfg['DifficultyID'],lfg['MinCountDamage'],lfg['CountDamage'])==(1374,12,1,5)
area=d.rows('AreaTable')
assert (area[8485]['ContinentID'],area[6941]['ContinentID'])==(1191,1116)
assert d.rows('Map')[1191]['InstanceType']==5  # cannot enable the old world-map controller blindly
members=d.rows('SceneScriptPackageMember')
packages={773:[11604,11605,11609],788:[11646,11654,11655],801:[11679],1003:[12424,12425]}
# SceneScriptText is sparse WDC1: its inline strings must not be read by the
# regular WDC1 string-offset reader. Check bounds and exact record count.
b=(a.dbc/'SceneScriptText.db2').read_bytes()
u=lambda n:struct.unpack_from('<I',b,n)[0]
assert b[:4]==b'WDC1' and struct.unpack_from('<H',b,44)[0]&1 and u(8)==2
first,last,catalog=u(28),u(32),u(60)
assert catalog+(last-first+1)*6<=len(b)
texts={}; records=set()
for i in range(first,last+1):
    off,size=struct.unpack_from('<IH',b,catalog+(i-first)*6)
    if size:
        assert 84<=off<catalog and off+size<=catalog
        fields=b[off:off+size].split(b'\0')
        assert len(fields)>=3
        texts[i]=(fields[0].decode(),fields[1].decode()); records.add((off,size))
assert len(records)==u(4)  # catalog aliases can share the same inline record
for package,scripts in packages.items():
    for script in scripts:
        assert any(r['SceneScriptPackageID']==package and r['SceneScriptID']==script for r in members.values()),(package,script)
        assert script in texts
assert '7321.39' in texts[11609][1] and '5099.22' in texts[11609][1]
assert '6280.62' in texts[11679][1] and '2224.26' in texts[11679][1]
assert 'Crowning an Exarch' in texts[12424][0]
assert '84974' in texts[12424][1] and '74.6788' in texts[12424][1]
report={
    'native_version':'Legion 7.3.5 runtime',
    'sha256':{name:hashlib.sha256((a.dbc/(name+'.db2')).read_bytes()).hexdigest() for name in ['SpellEffect','SceneScriptText','SceneScriptPackageMember','Map','LFGDungeons','AreaTable']},
    'scenes':{'580':773,'612':788,'602':801,'875':1003},
    'scene_script_names':{str(i):texts[i][0] for ids in packages.values() for i in ids},
    'trial_of_faith':{'map':1374,'lfg':870,'difficulty':12,'native_min_damage':1,'native_max_damage':5},
    'ashran_blocker':{'battle_map':1191,'instance_type':5,'battle_zone':8485,'legacy_registered_zone':6941,'legacy_zone_map':1116},
}
if a.output:
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('PASS: native spell effects, sparse scene scripts/positions, package links, quest instance and Ashran map mismatch')
