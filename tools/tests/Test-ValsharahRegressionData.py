"""Read-only checks of the installed Val'sharah routes (no migrations applied)."""
import csv
import io
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
MYSQL = Path('C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe')
config = (ROOT / 'build-extractors/bin/Release/worldserver.conf').read_text()
connection = re.search(r'^WorldDatabaseInfo\s*=\s*"([^"]+)"', config, re.M)[1].split(';')


def query(sql):
    env = dict(os.environ, MYSQL_PWD=connection[3])
    result = subprocess.run([str(MYSQL), '--host=' + connection[0], '--port=' + connection[1],
                             '--user=' + connection[2], '--database=' + connection[4],
                             '--batch', '--raw', '--execute=' + sql], env=env,
                            capture_output=True, text=True, check=True)
    return list(csv.DictReader(io.StringIO(result.stdout), delimiter='\t'))


native = json.loads((ROOT / 'docs/audits/valsharah-chain-native-2026-10-10.json').read_text())
scope = ','.join(map(str, native['scope']))
quests = {int(r['ID']): r for r in query(f'SELECT ID,AllowableRaces,StartItem,ItemDrop1,ItemDropQuantity1 FROM quest_template WHERE ID IN({scope})')}
assert len(quests) == 46
objectives = {int(r['ID']): r for r in query(f'SELECT * FROM quest_objectives WHERE QuestID IN({scope})')}
for expected in native['objectives']:
    row = objectives[expected['ID']]
    for field in ('QuestID', 'Type', 'StorageIndex', 'ObjectID', 'Amount', 'Flags', 'Flags2'):
        assert int(row[field]) == expected[field], (expected['ID'], field)

addons = {int(r['ID']): r for r in query(f'SELECT ID,PrevQuestID,SpecialFlags FROM quest_template_addon WHERE ID IN({scope})')}
conditions = query(f'SELECT SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1,NegativeCondition FROM conditions WHERE (SourceTypeOrReferenceId=19 AND SourceEntry IN({scope})) OR (SourceTypeOrReferenceId=23 AND SourceGroup=7558)')


def meets(source, entry, states):
    groups = {}
    for row in conditions:
        if int(row['SourceTypeOrReferenceId']) != source or int(row['SourceEntry']) != entry:
            continue
        kind = int(row['ConditionTypeOrReference'])
        assert kind in (8, 9, 28), ('unmodelled condition', row)
        value = states.get(int(row['ConditionValue1']), 0) == {8: 6, 9: 3, 28: 1}[kind]
        if int(row['NegativeCondition']):
            value = not value
        group = int(row['ElseGroup'])
        groups[group] = groups.get(group, True) and value
    return not groups or any(groups.values())


def available(quest, race, states):
    prev = int(addons[quest]['PrevQuestID'])
    assert prev >= 0, ('unmodelled negative prerequisite', quest)
    races = int(quests[quest]['AllowableRaces'])
    return (not races or races & race) and (not prev or states.get(prev) == 6) and meets(19, quest, states)


routes = (
    (1, [41056, 41708, 43576, 41724, 38684, 41893, 43702, 38687, 38743, 40890]),
    (2, [41054, 41890, 43576, 38675, 38684, 41749, 43702, 41763, 38743, 40890]),
)
for race, route in routes:
    states = {38753: 6}
    for quest in route:
        assert available(quest, race, states), ('route blocked', race, quest)
        states[quest] = 6
    assert not available(route[0], 3 ^ race, {38753: 6}), ('wrong faction admitted', race)
    for quest in (43576, 38684, 43702, 38743, 40890):
        assert not available(quest, race, {}), ('missing prerequisite guard', quest)

# The actual ender phase persists through COMPLETE -> REWARDED for either faction.
phases = {int(r['entry']): set(r['phaseId'].split()) for r in query('SELECT entry,phaseId FROM phase_definitions WHERE zoneId=7558')}
for entry, phase, variants in ((35, '6121', (41708, 41890)), (36, '6194', (41724, 38675)),
                               (37, '6185', (38687, 41763)), (38, '4731', (38743,))):
    assert phase in phases[entry]
    assert not meets(23, entry, {})
    for quest in variants:
        assert not meets(23, entry, {quest: 3})
        for state in (1, 6):
            assert meets(23, entry, {quest: state}), ('ender phase lost', quest, state)
for quest in (41724, 38675):
    assert not meets(23, 1500, {quest: 3})
    assert meets(23, 1500, {})  # Abandon restores the static escort starter.

enders = {int(r['quest']): int(r['id']) for r in query('SELECT id,quest FROM creature_questender WHERE quest IN(41708,41890,41724,38675,38687,41763,38743)')}
spawns = query('SELECT id,phaseId FROM creature WHERE map=1220 AND id IN(104885,104728,104799,104921)')
for quest, phase in ((41708,'6121'),(41890,'6121'),(41724,'6194'),(38675,'6194'),(38687,'6185'),(41763,'6185'),(38743,'4731')):
    assert any(int(r['id']) == enders[quest] and phase in r['phaseId'].split() for r in spawns), ('missing phased ender', quest)
assert (int(quests[40890]['StartItem']), int(quests[40890]['ItemDrop1']), int(quests[40890]['ItemDropQuantity1'])) == (139043,139043,1)
assert query('SELECT id FROM gameobject_questender WHERE quest=40890') == [{'id':'246466'}]
assert query('SELECT id FROM gameobject_queststarter WHERE quest=40890') == [{'id':'248534'}]
assert all(int(addons[q]['SpecialFlags']) & 2 == 0 for q in (38142,38381,38382,38384,38225,38235,38147,39384,38687,41763,38743))
print('PASS: live DB, 46 quests/58 native objectives; both faction routes, missing-prerequisite rejection, four ender phases through completion/reward, abandon restores escort starter, phased NPCs and Tears item/GO bindings.')
