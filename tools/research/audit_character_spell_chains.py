"""Audit the 40 reviewed Legion spell wrappers and their DB2 visual dependencies.

Run from the repository root. --sql-directory contains TSV exports documented
in docs/audits/character-spell-chains-2026-10-08.md. Does not modify game data.
"""
import argparse
import collections
import csv
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
from audit_spell_visuals import Table

# Manually reviewed C++ dispatch/parent presentation paths, not inferred from
# every numeric constant in a script. DB2 and SQL casts are collected below.
REVIEWED_PATHS = {
    1784: [158185], 47540: [47757, 47758], 47541: [47632, 196263],
    52610: [62071], 55095: [49184], 55709: [54114, 55711], 68996: [97709],
    81782: [62618], 106832: [106830, 77758], 109132: [107427],
    115546: [116189, 118635], 190780: [190778],
    198304: [147833, 218104, 198337, 223658, 199038], 200163: [200167],
    206505: [131900, 131637, 131951, 131952], 213764: [106785, 213771],
    219432: [200851],
}


def read_tsv(path):
    with path.open(encoding='utf-8-sig', newline='') as stream:
        return list(csv.DictReader(stream, delimiter='\t'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dbc', type=Path, default=Path('build-extractors/bin/Release/dbc/enUS'))
    parser.add_argument('--audit', type=Path, required=True, help='Output from audit_spell_visuals.py')
    parser.add_argument('--sql-directory', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    audit = json.loads(args.audit.read_text(encoding='utf-8'))
    roots = audit['class_active_without_direct_visual']
    tables = {}
    metadata = Path('src/server/game/DataStores/DB2Metadata.h').read_text(encoding='utf-8')

    def table(name):
        if name not in tables:
            block = metadata.split('struct ' + name + 'Meta\n', 1)[1].split('\n};', 1)[0]
            index, layout = re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[0-9A-Fa-f]+)', block).groups()
            tables[name] = Table(args.dbc / (name + '.db2'), int(index), int(layout, 16))
        return tables[name]

    def records(name, fields):
        t = table(name)
        rows = {t.ids[i]: {key: t.value(i, field) for key, field in fields.items()} for i in range(t.count)}
        for i, parent in t.parents.items():
            rows[t.ids[i]]['parent'] = parent
        for dest, source in t.copy_pairs:
            rows[dest] = dict(rows[source])
        return rows

    spell = table('Spell')
    names = dict(zip(spell.ids, (spell.name(i) for i in range(spell.count))))
    for dest, source in spell.copy_pairs:
        names[dest] = names[source]
    for row in read_tsv(args.sql_directory / 'visual-hotfix-spells.tsv'):
        names.setdefault(int(row['ID']), '(hotfix-only)')
    effects = records('SpellEffect', {'Effect': 1, 'EffectIndex': 3, 'EffectAura': 4,
                                      'DifficultyID': 5, 'EffectTriggerSpell': 16})
    for row in read_tsv(args.sql_directory / 'chain-hotfix-effects.tsv'):
        effects[int(row['ID'])] = dict(parent=int(row['SpellID']), **{k: int(row[k]) for k in
            ('Effect', 'EffectIndex', 'EffectAura', 'DifficultyID', 'EffectTriggerSpell')})
    edges = collections.defaultdict(set)
    by_spell = collections.defaultdict(list)
    for rid, row in effects.items():
        by_spell[row['parent']].append(dict(ID=rid, **row))
        if row['EffectTriggerSpell']:
            edges[row['parent']].add(row['EffectTriggerSpell'])
    for root, children in REVIEWED_PATHS.items():
        edges[root].update(children)
    for row in read_tsv(args.sql_directory / 'chain-spell_dummy_trigger.tsv'):
        if int(row['spell_trigger']) > 0:
            edges[abs(int(row['spell_id']))].add(int(row['spell_trigger']))
    for row in read_tsv(args.sql_directory / 'chain-linked-spells.tsv'):
        if int(row['spell_trigger']) > 0 and int(row['spell_effect']) > 0 and int(row['actiontype']) == 0:
            edges[int(row['spell_trigger'])].add(int(row['spell_effect']))
    for row in read_tsv(args.sql_directory / 'chain-spell_pet_auras.tsv'):
        if int(row['spellId']) == 119904:
            edges[119898].add(int(float(row['bp0'])))
    bindings = collections.defaultdict(list)
    for row in read_tsv(args.sql_directory / 'chain-script-bindings.tsv'):
        bindings[abs(int(row['spell_id']))].append(row['ScriptName'])

    link_rows = records('SpellXSpellVisual', {'visual': 0})
    for row in read_tsv(args.sql_directory / 'visual-hotfix.tsv'):
        link_rows[int(row['ID'])] = dict(parent=int(row['SpellID']), visual=int(row['SpellVisualID']))
    spell_visuals = collections.defaultdict(set)
    for row in link_rows.values():
        if row['visual']:
            spell_visuals[row['parent']].add(row['visual'])
    visual_rows = records('SpellVisual', {'missile_set': 3, 'targeting_kit': 8,
                                          'hostile': 11, 'caster': 12, 'low_violence': 13})
    events = records('SpellVisualEvent', {'kit': 7})
    visual_kits = collections.defaultdict(set)
    for row in events.values():
        if row['kit']:
            visual_kits[row['parent']].add(row['kit'])
    kits = records('SpellVisualKit', {'fallback': 2})
    kit_effects = records('SpellVisualKitEffect', {'type': 0, 'effect': 1})
    effects_by_kit = collections.defaultdict(list)
    for row in kit_effects.values():
        effects_by_kit[row['parent']].append(row)
    attachments = records('SpellVisualKitModelAttach', {'effect_name': 3, 'low_def': 19})
    missiles = records('SpellVisualMissile', {'effect_name': 5})
    # Build 26972 layout B930A934: model and texture are fields 10 and 8.
    effect_names = records('SpellVisualEffectName', {'model': 10, 'texture': 8})
    errors, results = [], []
    visited_visuals, visited_kits, visited_names, file_ids = set(), set(), set(), set()
    other_kit_types = collections.Counter()

    def effect_name(value):
        value &= 0xffff
        if not value or value in visited_names:
            return
        visited_names.add(value)
        if value not in effect_names:
            errors.append(['missing_effect_name', value])
            return
        file_ids.update(v for v in effect_names[value].values() if v and v < 0x80000000)

    visited_attachments = set()
    def attachment(value):
        if not value or value in visited_attachments:
            return
        visited_attachments.add(value)
        if value not in attachments:
            errors.append(['missing_attachment', value])
            return
        effect_name(attachments[value]['effect_name'])
        attachment(attachments[value]['low_def'])

    def kit(value):
        if not value or value in visited_kits:
            return
        visited_kits.add(value)
        if value not in kits:
            errors.append(['missing_kit', value])
            return
        kit(kits[value]['fallback'])
        for row in effects_by_kit[value]:
            if row['type'] == 2:
                attachment(row['effect'])
            else:
                other_kit_types[row['type']] += 1

    def visual(value):
        if not value or value in visited_visuals:
            return
        visited_visuals.add(value)
        if value not in visual_rows:
            errors.append(['missing_visual', value])
            return
        row = visual_rows[value]
        for field in ('hostile', 'caster', 'low_violence'):
            visual(row[field])
        for k in visual_kits[value] | {row['targeting_kit']}:
            kit(k)
        missile_set = row['missile_set'] & 0xffff
        if missile_set:
            matches = [r for r in missiles.values() if r['parent'] == missile_set]
            if not matches:
                errors.append(['missing_missile_set', missile_set])
            for missile in matches:
                effect_name(missile['effect_name'])

    for root in roots:
        visited, queue = set(), [root['ID']]
        while queue:
            current = queue.pop()
            if current in visited:
                continue
            visited.add(current)
            if current not in names:
                errors.append(['missing_spell', root['ID'], current])
            queue.extend(edges[current] - visited)
            for v in spell_visuals[current]:
                visual(v)
        results.append(dict(**root, bindings=bindings[root['ID']], effects=by_spell[root['ID']],
                            presentation_paths=[dict(ID=s, name=names.get(s), visuals=sorted(spell_visuals[s])) for s in sorted(visited)]))
    output = dict(source_hashes={name: t.sha256 for name, t in tables.items()},
                  counts=dict(roots=len(roots), visuals=len(visited_visuals), kits=len(visited_kits),
                              attachments=len(visited_attachments), effect_names=len(visited_names), files=len(file_ids)),
                  errors=errors, other_kit_effect_types=dict(other_kit_types),
                  referenced_file_ids=sorted(file_ids), spells=results,
                  limitations='Presentation paths include reviewed parent effects; not all edges are casts. Sound/camera/procedural kit effects and CASC file contents are not verified.')
    args.output.write_text(json.dumps(output, indent=2), encoding='utf-8')
    print(json.dumps(output['counts']))
    print('Errors:', errors)
    raise SystemExit(bool(errors))


if __name__ == '__main__':
    main()
