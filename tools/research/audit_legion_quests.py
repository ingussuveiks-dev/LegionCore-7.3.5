"""Read-only candidate audit, not a certificate of quest playability.

The optimistic graph models both QuestData::LoadQuests predecessor sources.
It intentionally ignores class/race/phase/group restrictions and scripted grants;
unreachable candidates require investigation, reachable quests may still be broken.
"""
import argparse
import collections
import csv
import hashlib
import json
from pathlib import Path


def predecessors(quests, addons):
    result = {q: set() for q in quests}
    for q, row in addons.items():
        prev, nxt = int(row['PrevQuestID']), int(row['NextQuestID'])
        if q in result and abs(prev) in quests:
            result[q].add(prev)
        if abs(nxt) in result and q in quests:
            result[abs(nxt)].add(q if nxt > 0 else -q)
    return result


def reachable(quests, disabled, prev):
    result = {q for q in quests if not prev[q] and q not in disabled}
    while True:
        added = {q for q in quests if q not in result and q not in disabled
                 and any(abs(p) in result for p in prev[q])}
        if not added:
            return result
        result.update(added)


def audit(directory):
    hashes = {}
    def read(name):
        path = directory / (name + '.tsv')
        hashes[path.name] = hashlib.sha256(path.read_bytes()).hexdigest()
        with path.open(encoding='utf-8-sig', newline='') as stream:
            return list(csv.DictReader(stream, delimiter='\t'))
    quests = {int(r['ID']): r for r in read('quests')}
    addons = {int(r['ID']): r for r in read('addons')}
    disabled = {int(r['entry']): r['comment'] for r in read('disabled')}
    scope = {q for q, r in quests.items() if 98 <= int(r['MinLevel']) <= 110
             or 100 <= int(r['QuestLevel']) <= 110 or int(r['Expansion']) == 6}
    prev = predecessors(quests, addons)
    reached = reachable(quests, disabled, prev)
    relations = collections.defaultdict(list)
    targets = {'npc': {int(r['entry']) for r in read('creature-templates')},
               'go': {int(r['entry']) for r in read('gameobject-templates')}}
    spawns = {'npc': {int(r['id']) for r in read('creature-spawns')},
              'go': {int(r['id']) for r in read('gameobject-spawns')}}
    missing_relations, unspawned = [], []
    for row in read('relations'):
        q, entry = int(row['quest']), int(row['id'])
        if q not in scope:
            continue
        relations[q].append(row)
        kind = row['kind'].split('_')[0]
        if kind in targets:
            if entry not in targets[kind]:
                missing_relations.append(row)
            elif entry not in spawns[kind]:
                unspawned.append(row)  # Summons/transports are not static spawns.
    missing_links = []
    for q in scope:
        for field in ['PrevQuestID', 'NextQuestID']:
            value = int(addons.get(q, {}).get(field, 0))
            if value and abs(value) not in quests:
                missing_links.append(dict(quest=q, field=field, target=value))
        value = int(quests[q]['RewardNextQuest'])
        if value and value not in quests:
            missing_links.append(dict(quest=q, field='RewardNextQuest', target=value))
    conditions = [r for r in read('conditions') if int(r['SourceEntry']) in scope]
    condition_candidates = [r for r in conditions
                            if int(r['ConditionTypeOrReference']) in (8, 9, 28, 41)
                            and not int(r['NegativeCondition'])
                            and (int(r['ConditionValue1']) not in quests
                                 or int(r['ConditionValue1']) in disabled)]
    objectives = [r for r in read('objectives') if int(r['QuestID']) in scope]
    missing_objectives = [r for r in objectives
                          if (int(r['Type']) in (0, 3, 11)
                              and int(r['ObjectID']) not in targets['npc'])
                          or (int(r['Type']) == 2
                              and int(r['ObjectID']) not in targets['go'])]
    events = [r for r in read('event-quests') if int(r['quest']) in scope]
    smart = read('smart-quests')
    def describe(q):
        return dict(id=q, title=quests[q]['Title'], level=int(quests[q]['QuestLevel']),
                    min_level=int(quests[q]['MinLevel']), predecessors=sorted(prev[q]),
                    relations=relations[q])
    return dict(scope_rule='MinLevel 98..110 OR QuestLevel 100..110 OR Expansion=6',
                limitations='Optimistic graph only; does not prove objectives, phases, loot, class/race restrictions or scripts can execute in game.',
                counts=dict(all_quests=len(quests), scoped_quests=len(scope),
                            disabled_in_scope=len(scope & disabled.keys()),
                            objectives=len(objectives), source_relations=sum(map(len, relations.values())),
                            conditions=len(conditions), event_relations=len(events),
                            smart_rows_exported=len(smart)),
                by_quest_sort=dict(sorted(collections.Counter(int(quests[q]['QuestSortID']) for q in scope).items())),
                blocked_candidates=[describe(q) for q in sorted(scope - reached - disabled.keys())],
                disabled=[dict(id=q, title=quests[q]['Title'], reason=disabled[q]) for q in sorted(scope & disabled.keys())],
                missing_chain_targets=missing_links,
                missing_source_templates=missing_relations,
                missing_creature_gameobject_objective_targets=missing_objectives,
                positive_conditions_on_missing_or_disabled_quests=condition_candidates,
                source_relations_without_static_spawn=unspawned,
                events=events, input_sha256=hashes)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exports', type=Path, default=Path('.codex/legion-quests'))
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = audit(args.exports)
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    print(json.dumps(report['counts']))
    for field in ['blocked_candidates', 'missing_chain_targets', 'missing_source_templates',
                  'missing_creature_gameobject_objective_targets',
                  'positive_conditions_on_missing_or_disabled_quests',
                  'source_relations_without_static_spawn']:
        print(field, len(report[field]))
