"""Original cage/objective and native 26972 lock/model/effect evidence."""
import hashlib
import json
import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/research'))
from audit_spell_mechanics import Data

archive = ROOT / '.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
selected = {'quest_objectives', 'gameobject_template', 'creature_template', 'gameobject', 'creature'}
columns, records, table = {}, {}, None
for line in archive.open(encoding='utf8'):
    match = re.match(r'CREATE TABLE `([^`]+)`', line)
    if match:
        table = match[1] if match[1] in selected else None
        columns[table] = []
    elif table:
        if line.startswith(')'):
            table = None
        else:
            match = re.match(r'  `([^`]+)`', line)
            if match:
                columns[table].append(match[1])
    match = re.match(r'INSERT INTO `([^`]+)`', line)
    if not match or match[1] not in selected:
        continue
    name = match[1]
    for match in re.finditer(r"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)", line):
        row = dict(zip(columns[name], re.findall(r"'(?:\\.|''|[^'\\])*'|[^,]+", match[0][1:-1])))
        keep = ((name == 'quest_objectives' and row['QuestID'] == '39837') or
                (name == 'gameobject_template' and row['entry'] == '241778') or
                (name == 'creature_template' and row['entry'] == '91531') or
                (name == 'gameobject' and row['id'] == '241778') or
                (name == 'creature' and row['id'] == '91531'))
        if keep:
            records.setdefault(name, []).append(row)

obj, = records['quest_objectives']
assert all(obj[k] == v for k, v in {'ID':'280004','Type':'2','StorageIndex':'1','ObjectID':'241778','Amount':'1','Flags':'0'}.items())
cage, = records['gameobject_template']
assert all(cage[k] == v for k, v in {'type':'10','displayId':'7469','Data0':'1691','Data1':'39837','Data3':'3000','Data10':'0','Data14':'21400','Data20':'1','Data26':'1'}.items())
d = Data(ROOT / 'build-extractors/bin/Release/dbc/enUS', ROOT / '.codex/stormheim-no-overlays')
native = {t:d.rows(t)[i] for t, i in [('Lock',1691), ('GameObjectDisplayInfo',7469), ('CreatureDisplayInfo',63385), ('CreatureModelData',2594), ('Map',1463)]}
native['opening'] = [r for r in d.rows('SpellEffect').values() if r['SpellID'] == 3365]
assert native['Lock']['Type2'] == 2 and native['Lock']['Index2'] == 5
assert len(native['opening']) == 1 and all(native['opening'][0][k] == v for k, v in {'Effect':33,'EffectMiscValue1':5,'ImplicitTarget1':23}.items())
assert native['GameObjectDisplayInfo']['FileDataID'] == 195417
assert native['CreatureDisplayInfo']['ModelID'] == 2594 and native['CreatureModelData']['FileDataID'] > 0
assert native['Map']['InstanceType'] == 0
records.update(archive_sha256=hashlib.sha256(archive.read_bytes()).hexdigest(), native=native, runtime_hashes=d.hashes,
               limits='Original template/objective records; archive has no corresponding cage/Colborn world-spawn rows. Installed spawn/phase/quest relations are checked separately in live SQL. Native lock/effect/model existence is not rendered animation, client collision or retail escape choreography evidence. Outdoor map1463 uses quest objectives/phases, not ScenarioResume.')
path = ROOT / 'docs/audits/stormheim-unworthy-task-native-2026-10-15.json'
if '--write-evidence' in sys.argv:
    path.write_text(json.dumps(records, indent=2) + '\n')
else:
    assert json.loads(json.dumps(records)) == json.loads(path.read_text())
print('PASS: original GO241778/39837 objective, native lock1691/opening3365, cage195417/Colborn63385 model chain and outdoor Helheim. Client presentation pending.')
