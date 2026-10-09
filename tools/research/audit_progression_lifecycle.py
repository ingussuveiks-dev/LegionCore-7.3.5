"""Read-only 7.3.5 references; candidates are not automatic deletion decisions."""
import argparse
import collections
import csv
import hashlib
import json
from pathlib import Path
import sys
sys.dont_write_bytecode = True
from audit_spell_mechanics import Data


def audit(dbc, exports):
    data = Data(dbc, exports)
    hashes = {}

    def read(name):
        path = exports / name
        hashes[name] = hashlib.sha256(path.read_bytes()).hexdigest()
        with path.open(encoding='utf-8-sig', newline='') as stream:
            return list(csv.DictReader(stream, delimiter='\t'))

    tables = ['Spell', 'SpellMisc', 'SpellEffect', 'SpellRange', 'SpellRadius',
              'SpellCastTimes', 'SpellDuration', 'SpellTargetRestrictions',
              'SpellInterrupts', 'SpellAuraRestrictions', 'SpellShapeshift',
              'Item', 'ItemSet', 'ItemSetSpell', 'ItemEffect', 'CurrencyTypes',
              'SkillLine', 'Faction', 'Scenario', 'ScenarioStep', 'Criteria', 'CriteriaTree']
    for count in read('counts.tsv'):
        overlay = read('mechanics-' + count['table_name'] + '.tsv')
        assert len(overlay) == int(count['rows_count']), count
    for table in tables:
        data.rows(table)
    candidates = []

    def ref(table, row, field, targets):
        value = int(row[field])
        if value and abs(value) not in targets:
            candidates.append({'table': table, 'id': int(row['ID']), 'field': field,
                               'target': value, 'context':{k:int(row[k]) for k in ['SpellID','ItemID','QuestID'] if k in row},
                               'origin': 'sql' if table not in data.native or int(row['ID']) in data.overlaid.get(table, set()) else 'native'})

    for row in data.rows('SpellMisc').values():
        for field, target in [('RangeIndex','SpellRange'), ('DurationIndex','SpellDuration'), ('CastingTimeIndex','SpellCastTimes')]:
            ref('SpellMisc', row, field, data.rows(target))
    for row in data.rows('SpellEffect').values():
        for field in ['EffectRadiusIndex1', 'EffectRadiusIndex2']:
            ref('SpellEffect', row, field, data.rows('SpellRadius'))
    for row in data.rows('SpellAuraRestrictions').values():
        for field in ['CasterAuraSpell', 'TargetAuraSpell', 'ExcludeCasterAuraSpell', 'ExcludeTargetAuraSpell']:
            ref('SpellAuraRestrictions', row, field, data.rows('Spell'))
    for row in data.rows('ItemSet').values():
        ref('ItemSet', row, 'RequiredSkill', data.rows('SkillLine'))
    for row in data.rows('ItemSetSpell').values():
        ref('ItemSetSpell', row, 'SpellID', data.rows('Spell'))
        ref('ItemSetSpell', row, 'ItemSetID', data.rows('ItemSet'))
    for row in data.rows('ItemEffect').values():
        ref('ItemEffect', row, 'SpellID', data.rows('Spell'))
        ref('ItemEffect', row, 'ItemID', data.rows('Item'))

    quests = {int(r['ID']): r for r in read('quests.tsv')}
    addons = {int(r['ID']): r for r in read('quest-addon.tsv')}
    objectives = read('objectives.tsv')
    disabled_quests = {int(r['entry']) for r in read('disabled-quests.tsv')}
    creatures = {int(r['entry']) for r in read('creatures.tsv')}
    gameobjects = {int(r['entry']) for r in read('gameobjects.tsv')}
    for r in quests.values():
        for field, targets in [('RewardNextQuest',quests), ('RewardSpell',data.rows('Spell')),
                               ('StartItem',data.rows('Item')), ('RewardSkillLineID',data.rows('SkillLine'))]:
            ref('quest_template', r, field, targets)
        for n in range(1, 5):
            ref('quest_template', r, f'RewardItem{n}', data.rows('Item'))
            ref('quest_template', r, f'RewardCurrencyID{n}', data.rows('CurrencyTypes'))
        for n in range(1, 7):
            ref('quest_template', r, f'RewardChoiceItemID{n}', data.rows('Item'))
    for r in addons.values():
        for field, targets in [('ID',quests), ('PrevQuestID',quests), ('NextQuestID',quests),
                               ('SourceSpellID',data.rows('Spell')), ('RequiredSkillID',data.rows('SkillLine'))]:
            ref('quest_template_addon', r, field, targets)
    for r in objectives:
        ref('quest_objectives', r, 'QuestID', quests)
        target = {0:creatures, 1:data.rows('Item'), 2:gameobjects, 3:creatures,
                  4:data.rows('CurrencyTypes'), 5:data.rows('Spell'), 6:data.rows('Faction'),
                  7:data.rows('Faction'), 11:creatures, 14:data.rows('CriteriaTree'),
                  16:data.rows('CurrencyTypes'), 17:data.rows('CurrencyTypes')}.get(int(r['Type']))
        if target is not None:
            ref('quest_objectives', r, 'ObjectID', target)
    for candidate in candidates:
        if candidate['table'] == 'SpellAuraRestrictions':
            candidate['owner_spell_present'] = candidate['context']['SpellID'] in data.rows('Spell')
        if candidate['table'].startswith('quest_'):
            quest_id = candidate['context'].get('QuestID', candidate['id'])
            candidate['quest_disabled'] = quest_id in disabled_quests
    sources = {}
    for name, targets in [('npc-quests.tsv', creatures), ('go-quests.tsv', gameobjects)]:
        rows = read(name)
        sources[name] = len(rows)
        for r in rows:
            source = {'ID':r['id'], 'quest':r['quest']}
            ref(name, source, 'ID', targets)
            ref(name, source, 'quest', quests)

    # Only positive PrevQuestID edges demand completed prerequisite quests.
    # Exclusive groups, negative active-quest prerequisites and scripts require
    # separate interpretation; they are not folded into this graph.
    edges = {id:int(r['PrevQuestID']) for id,r in addons.items() if int(r['PrevQuestID']) > 0}
    cycles, visited = [], set()
    for start in edges:
        trail, positions, node = [], {}, start
        while node in edges and node not in visited:
            if node in positions:
                cycles.append(trail[positions[node]:]); break
            positions[node] = len(trail); trail.append(node); node = edges[node]
        visited.update(trail)

    boost_ids = {1073,1083,1084,1090,1091,1093,1094,1095,1096,1132,1133,1181,1182,1214}
    boost_steps = [r for r in data.rows('ScenarioStep').values() if r['ScenarioID'] in boost_ids]
    for row in boost_steps:
        ref('ScenarioStep', row, 'ScenarioID', data.rows('Scenario'))
        ref('ScenarioStep', row, 'Criteriatreeid', data.rows('CriteriaTree'))
    path_scripts = {int(r['id']) for r in read('path-scripts.tsv')}
    ordinary = {int(r['action']) for r in read('path-actions.tsv')}
    scripted = {int(r['action']) for r in read('script-paths.tsv')}
    return {
        'build':'7.3.5.26972', 'date':'2026-10-09',
        'counts':{**{name:len(data.rows(name)) for name in tables},
                  'quests':len(quests), 'quest_addons':len(addons), 'quest_objectives':len(objectives), **sources},
        'boost':{'scenarios':sorted(boost_ids), 'steps':len(boost_steps),
                 'retired_step_overlap':sorted({r['ID'] for r in boost_steps} & {1947,2233,2234}),
                 'routes':read('scenario-routes.tsv'), 'instances':read('instance-routes.tsv'),
                 'departure_binding':read('departure-binding.tsv')},
        'profession_sets':[{k:r[k] for k in ['ID','RequiredSkill','RequiredSkillRank']} for r in data.rows('ItemSet').values() if r['RequiredSkill']],
        'positive_prerequisite_cycles':cycles,
        'cycle_external_next_edges':[
            {'cycle':cycle, 'external_predecessors':[
                {'from':id, 'to':int(row['NextQuestID'])}
                for id,row in addons.items() if id not in cycle and int(row['NextQuestID']) in cycle]}
            for cycle in cycles],
        'waypoint_scripts_unreferenced_before':sorted(path_scripts-ordinary),
        'waypoint_scripts_unreferenced_after':sorted(path_scripts-ordinary-scripted),
        'candidate_counts':dict(collections.Counter(r['table'] for r in candidates)),
        'candidates':candidates,
        'db2_hashes':data.hashes, 'sql_hashes':{**data.sql_hashes, **hashes},
        'limits':['Missing references are review candidates, not proof that reachable gameplay is broken.',
                  'Server scripts, disabled/retired quests and custom records require individual interpretation.',
                  'This checks reference existence, not retail combat values, all immunity interactions or client rendering.']}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--dbc',type=Path,default=Path('build-extractors/bin/Release/dbc/enUS'))
    parser.add_argument('--exports',type=Path,default=Path('.codex/progression-audit'))
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args()
    result = audit(args.dbc, args.exports)
    args.output.write_text(json.dumps(result, indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:result[k] for k in ['counts','candidate_counts','positive_prerequisite_cycles','waypoint_scripts_unreferenced_before','waypoint_scripts_unreferenced_after']},indent=2))
