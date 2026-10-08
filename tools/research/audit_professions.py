"""Audit Legion 26972 profession DB2 references against exported runtime SQL.

Read-only. Export files and limitations are documented in the accompanying audit.
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
from audit_inscription_class_glyph import audit as audit_class_glyph

# The 11 primary and four secondary professions present in Legion.
PROFESSIONS = {164, 165, 171, 182, 197, 202, 333, 393, 755, 773, 186,
               129, 185, 356, 794}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dbc', type=Path, default=Path('build-extractors/bin/Release/dbc/enUS'))
    parser.add_argument('--sql-directory', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--visual-roots', type=Path, help='Input for audit_character_spell_chains.py')
    args = parser.parse_args()
    metadata = Path('src/server/game/DataStores/DB2Metadata.h').read_text()
    tables = {}

    def table(name):
        if name not in tables:
            block = metadata.split('struct ' + name + 'Meta\n')[1].split('\n};')[0]
            index, layout = re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[0-9A-Fa-f]+)', block).groups()
            tables[name] = Table(args.dbc / (name + '.db2'), int(index), int(layout, 16))
        return tables[name]

    def rows(name, fields):
        t = table(name)
        result = {t.ids[i]: {k: t.value(i, f) for k, f in fields.items()} for i in range(t.count)}
        for i, parent in t.parents.items():
            result[t.ids[i]]['parent'] = parent
        for dest, source in t.copy_pairs:
            result[dest] = dict(result[source])
        return result

    def sql(name):
        with (args.sql_directory / name).open(encoding='utf-8-sig', newline='') as stream:
            return list(csv.DictReader(stream, delimiter='\t'))

    # Do not silently audit native skill relationships if SQL overrides them.
    for row in sql('profession-hotfix-counts.tsv'):
        if int(row['rows_count']):
            raise ValueError('Audit requires overlay support for nonempty ' + row['table_name'])
    skill_table = table('SkillLine')
    skill_names = {skill_table.ids[i]: skill_table.name(i) for i in range(skill_table.count)}
    skills = rows('SkillLine', {'parent': 7})
    selected = PROFESSIONS | {sid for sid, row in skills.items() if row['parent'] in PROFESSIONS}
    spell_table = table('Spell')
    names = {spell_table.ids[i]: spell_table.name(i) for i in range(spell_table.count)}
    for dest, source in spell_table.copy_pairs:
        names[dest] = names[source]
    for row in sql('visual-hotfix-spells.tsv'):
        names.setdefault(int(row['ID']), '(SQL spell)')
    abilities = rows('SkillLineAbility', {'spell': 2, 'skill': 4, 'rank': 11, 'acquire': 12})
    selected_abilities = {rid: row for rid, row in abilities.items() if row['skill'] & 0xffff in selected}
    roots = {row['spell'] for row in selected_abilities.values()}
    missing_catalog = sorted(roots - names.keys())
    effects = rows('SpellEffect', {'effect': 1, 'index': 3, 'aura': 4, 'trigger': 16, 'misc': 26})
    for row in sql('chain-hotfix-effects.tsv'):
        effects[int(row['ID'])] = dict(parent=int(row['SpellID']), effect=int(row['Effect']), index=int(row['EffectIndex']),
                                      aura=int(row['EffectAura']), trigger=int(row['EffectTriggerSpell']),
                                      misc=[int(row['EffectMiscValue1']), int(row['EffectMiscValue2'])])
    graph = collections.defaultdict(set)
    learn = rows('SpellLearnSpell', {'spell': 0, 'learn': 1, 'override': 2})
    for row in learn.values():
        graph[row['spell']].add(row['learn'])
    for row in sql('profession-world-learn.tsv'):
        graph[int(row['entry'])].add(int(row['SpellID']))
    for row in effects.values():
        if row['trigger']:
            graph[row['parent']].add(row['trigger'])

    # Relearning scripts are not fully represented by DB2 trigger fields.
    source = Path('src/server/scripts/Spells/spell_generic.cpp').read_text(encoding='utf-8')
    relearn = {}
    for script, body in re.findall(r'class (spell_gen_relearn_\w+_quests) : public SpellScript\s*\{(.*?)\n\};', source, re.S):
        relearn[script] = sorted({int(v) for v in re.findall(r'CastSpell\(player, (\d+), true\)', body)})
    bindings = sql('chain-script-bindings.tsv')
    for row in bindings:
        if row['ScriptName'] in relearn:
            root = abs(int(row['spell_id']))
            roots.add(root)
            graph[root].update(relearn[row['ScriptName']])
    class_glyph = audit_class_glyph(args.dbc, args.sql_directory)
    if not class_glyph['errors']:
        for recipe in class_glyph['recipes']:
            graph[192962].update([recipe['recipe'], *recipe['item_use_spells'],
                                 *recipe['glyph_spells'], *recipe['bindable_spells']])
    visited, queue = set(), list(roots & names.keys())
    missing_edges = set()
    while queue:
        current = queue.pop()
        if current in visited:
            continue
        visited.add(current)
        for child in graph[current]:
            if child not in names:
                missing_edges.add((current, child))
            else:
                queue.append(child)
    # REMOVE_AURA (140) is cleanup, not an attempt to cast/learn a deleted ID.
    missing_removals = sorted({(row['parent'], row['trigger']) for row in effects.values()
                               if row['parent'] in visited and row['effect'] == 140 and row['trigger'] not in names})
    # HandlePeriodicTriggerSpellAuraTick has a native summon fallback for Spellcloth.
    scripted_fallbacks = {(31373, 31374)} & missing_edges
    if not class_glyph['errors']:
        scripted_fallbacks.update({(192962, child) for child in class_glyph['suppressed_trigger_ids']} & missing_edges)
    unresolved = missing_edges - set(missing_removals) - scripted_fallbacks
    trainer_rows = sql('profession-trainer.tsv')
    invalid_trainers = [r for r in trainer_rows if int(r['spell']) in missing_catalog]
    enchantments = rows('SpellItemEnchantment', {'skill': 8, 'rank': 9, 'visual': 6})
    profession_enchants = {rid: row for rid, row in enchantments.items() if row['skill'] & 0xffff in PROFESSIONS}
    errors = [['inscription_class_glyph', error] for error in class_glyph['errors']]
    referenced_enchants = set(profession_enchants)
    for row in effects.values():
        if row['parent'] in visited and row['effect'] in (53, 54, 92, 156):
            enchant = row['misc'][0] if isinstance(row['misc'], list) else row['misc'] & 0xffffffff
            if enchant:
                referenced_enchants.add(enchant)
    item_visuals = rows('ItemVisuals', {'models': 0})
    item_files, visited_item_visuals = set(), set()
    for enchant in sorted(referenced_enchants):
        if enchant not in enchantments:
            errors.append(['missing_enchantment', enchant])
            continue
        visual = enchantments[enchant]['visual'] & 0xffff
        if not visual:
            continue
        visited_item_visuals.add(visual)
        if visual not in item_visuals:
            errors.append(['missing_item_visual', enchant, visual])
            continue
        models = item_visuals[visual]['models']
        if not isinstance(models, list):
            models = [(models >> (32 * i)) & 0xffffffff for i in range(5)]
        item_files.update(model for model in models if model)
    for script, targets in relearn.items():
        if not any(row['ScriptName'] == script for row in bindings):
            errors.append(['unbound_relearn_script', script])
        for binding in bindings:
            if binding['ScriptName'] == script:
                root = abs(int(binding['spell_id']))
                # All eleven production scripts register EFFECT_0 / SPELL_EFFECT_DUMMY.
                if not any(e['parent'] == root and e['index'] == 0 and e['effect'] == 3 for e in effects.values()):
                    errors.append(['incompatible_relearn_effect', script, root])
        for target in targets:
            if target not in names:
                errors.append(['missing_relearn_spell', script, target])
    result = dict(
        build='7.3.5.26972', source_hashes={n: t.sha256 for n, t in tables.items()},
        counts=dict(skills=len(selected), catalog_spells=len({r['spell'] for r in selected_abilities.values()}),
                    reachable_spells=len(visited), missing_catalog_spells=len(missing_catalog),
                    profession_enchantments=len(profession_enchants), relearn_scripts=len(relearn),
                    referenced_enchantments=len(referenced_enchants), item_visuals=len(visited_item_visuals),
                    item_visual_files=len(item_files)),
        professions=[dict(ID=sid, name=skill_names[sid], parent=skills[sid]['parent'],
                          spells=len({r['spell'] for r in selected_abilities.values() if r['skill'] & 0xffff == sid})) for sid in sorted(selected)],
        missing_catalog_spells=missing_catalog, missing_effect_or_learn_targets=sorted(missing_edges),
        missing_aura_removal_targets=missing_removals, script_handled_missing_targets=sorted(scripted_fallbacks),
        unresolved_trigger_targets=sorted(unresolved),
        inscription_class_glyph=class_glyph,
        trainer_rows_using_missing_catalog_spells=invalid_trainers,
        relearn_scripts=relearn, errors=errors, item_visual_file_ids=sorted(item_files),
        limitations='Native catalog tombstones are not automatically server defects. Does not simulate crafting, all scripts, items, quests, racial bonuses or client rendering.')
    args.output.write_text(json.dumps(result, indent=2), encoding='utf-8')
    if args.visual_roots:
        args.visual_roots.write_text(json.dumps({'class_active_without_direct_visual': [dict(ID=s, name=names[s]) for s in sorted(visited)]}), encoding='utf-8')
    print(json.dumps(result['counts']))
    print('Missing aura cleanup targets:', len(missing_removals))
    print('Script-handled missing targets:', sorted(scripted_fallbacks))
    print('Unresolved trigger targets:', sorted(unresolved))
    print('Trainer rows using missing catalog spells:', invalid_trainers)
    print('Errors:', errors)
    raise SystemExit(bool(errors or unresolved or invalid_trainers))


if __name__ == '__main__':
    main()
