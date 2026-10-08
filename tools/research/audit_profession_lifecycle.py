"""Read-only profession source, crafting and glyph-reference audit for build 26972."""
import argparse
import collections
import csv
import hashlib
import json
from pathlib import Path
import re
import sys
sys.dont_write_bytecode = True
from audit_spell_visuals import Table
from audit_professions import PROFESSIONS


def audit(dbc, sql_directory):
    metadata = Path('src/server/game/DataStores/DB2Metadata.h').read_text()
    tables = {}
    sql_hashes = {}

    def table(name):
        if name not in tables:
            block = metadata.split('struct ' + name + 'Meta\n')[1].split('\n};')[0]
            index, layout = re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[0-9A-Fa-f]+)', block).groups()
            tables[name] = Table(dbc / (name + '.db2'), int(index), int(layout, 16))
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
        path = sql_directory / name
        sql_hashes[name] = hashlib.sha256(path.read_bytes()).hexdigest()
        with path.open(encoding='utf-8-sig', newline='') as stream:
            return list(csv.DictReader(stream, delimiter='\t'))

    for r in sql('profession-lifecycle-hotfix-counts.tsv') + sql('profession-hotfix-counts.tsv'):
        if r['table_name'] not in ('item', 'item_effect') and int(r['rows_count']):
            raise ValueError('Unimplemented hotfix overlay: ' + r['table_name'])
    spells = table('Spell').all_ids | {int(r['ID']) for r in sql('visual-hotfix-spells.tsv')}
    skills = rows('SkillLine', {'parent': 7})
    families = PROFESSIONS | {k for k, r in skills.items() if r['parent'] in PROFESSIONS}
    abilities = rows('SkillLineAbility', {'spell': 2, 'skill': 4})
    catalog = {r['spell'] for r in abilities.values() if r['skill'] & 0xffff in families}
    live = catalog & spells
    missing_catalog = catalog - spells
    items = rows('Item', {'class': 1, 'subclass': 2})
    for r in sql('profession-hotfix-items.tsv'):
        items[int(r['ID'])] = {'class': int(r['ClassID']), 'subclass': int(r['SubclassID'])}
    uses = rows('ItemEffect', {'spell': 0, 'trigger': 7})
    for r in sql('profession-hotfix-item-effects.tsv'):
        uses[int(r['ID'])] = dict(parent=int(r['ItemID']), spell=int(r['SpellID']), trigger=int(r['TriggerType']))
    effects = rows('SpellEffect', {'effect': 1, 'item': 12, 'trigger': 16, 'misc': 26})
    for r in sql('chain-hotfix-effects.tsv'):
        effects[int(r['ID'])] = dict(parent=int(r['SpellID']), effect=int(r['Effect']),
                                    item=int(r['EffectItemType']), trigger=int(r['EffectTriggerSpell']),
                                    misc=[int(r['EffectMiscValue1']), int(r['EffectMiscValue2'])])
    errors = []
    creates = {e['item'] for e in effects.values() if e.get('parent') in live and e['effect'] in (24, 157) and e['item']}
    for item in sorted(creates - items.keys()):
        errors.append(['missing_crafted_item', item])
    reagents = rows('SpellReagents', {'spell': 0, 'items': 1, 'counts': 2})
    reagent_ids = set()

    def array(value, width, count):
        return value if isinstance(value, list) else [(value >> (i * width)) & ((1 << width) - 1) for i in range(count)]

    reagent_rows = 0
    for r in reagents.values():
        if r['spell'] not in live:
            continue
        reagent_rows += 1
        for item, count in zip(array(r['items'], 32, 8), array(r['counts'], 16, 8)):
            if 0 < item < 0x80000000:
                reagent_ids.add(item)
                if item not in items:
                    errors.append(['missing_reagent_item', r['spell'], item])
                if not count:
                    errors.append(['zero_reagent_count', r['spell'], item])
    vendors = sql('profession-vendors.tsv')
    vendor_items = {int(r['item']) for r in vendors if int(r['type']) == 1}
    invalid_techniques = {u['parent']: u['spell'] for u in uses.values()
                          if u.get('parent') in vendor_items and u['spell'] in missing_catalog}
    for item, spell in sorted(invalid_techniques.items()):
        errors.append(['vendor_teaches_missing_profession_spell', item, spell])
    invalid_trainers = [r for r in sql('profession-trainer.tsv') if int(r['spell']) in missing_catalog]
    errors.extend(['trainer_teaches_missing_profession_spell', int(r['entry']), int(r['spell'])] for r in invalid_trainers)
    invalid_rewards = [r for r in sql('profession-quest-rewards.tsv') if int(r['RewardSpell']) in missing_catalog]
    errors.extend(['quest_teaches_missing_profession_spell', int(r['ID']), int(r['RewardSpell'])] for r in invalid_rewards)
    learn = rows('SpellLearnSpell', {'spell': 0, 'learn': 1})
    learn_edges = collections.defaultdict(set)
    for r in learn.values():
        learn_edges[r['spell']].add(r['learn'])
    for r in sql('profession-world-learn.tsv'):
        learn_edges[int(r['entry'])].add(int(r['SpellID']))
    checked = set()

    def walk(spell, path):
        if spell in path:
            errors.append(['learn_cycle', path[path.index(spell):] + [spell]])
            return
        if spell in checked:
            return
        checked.add(spell)
        for child in learn_edges[spell]:
            if child not in spells:
                errors.append(['missing_learn_target', spell, child])
            else:
                walk(child, path + [spell])

    for spell in sorted(live):
        walk(spell, [])
    crafted_uses = {u['spell'] for u in uses.values() if u.get('parent') in creates}
    glyphs = rows('GlyphProperties', {'spell': 0, 'category': 3})
    bind_rows = rows('GlyphBindableSpell', {'spell': 0})
    bindings = collections.defaultdict(set)
    for b in bind_rows.values():
        bindings[b['parent']].add(b['spell'])
    used_glyphs = set()
    for e in effects.values():
        if e.get('parent') not in crafted_uses or e['effect'] != 74:
            continue
        glyph = array(e['misc'], 32, 2)[0]
        if glyph:
            used_glyphs.add(glyph)
    for glyph in sorted(used_glyphs):
        if glyph not in glyphs:
            errors.append(['missing_glyph_properties', glyph])
        elif glyphs[glyph]['spell'] not in spells:
            errors.append(['missing_glyph_spell', glyph, glyphs[glyph]['spell']])
        if not bindings[glyph]:
            errors.append(['missing_glyph_bindings', glyph])
        for spell in bindings[glyph] - spells:
            errors.append(['missing_glyph_bindable_spell', glyph, spell])
    # Empty character export means there are currently no persisted glyph rows.
    saved_path = sql_directory / 'profession-saved-glyphs.tsv'
    saved = sql(saved_path.name) if saved_path.exists() and saved_path.stat().st_size else []
    for r in saved:
        glyph = int(r['glyphId'])
        if glyph not in glyphs or glyphs[glyph]['spell'] not in spells:
            errors.append(['invalid_saved_glyph', glyph])
    return dict(build='7.3.5.26972', errors=errors,
                counts=dict(profession_spells=len(live), crafted_items=len(creates),
                            reagent_rows=reagent_rows, reagent_items=len(reagent_ids),
                            vendor_rows=len(vendors), sold_profession_techniques=len({u['parent'] for u in uses.values() if u.get('parent') in vendor_items and u['spell'] in live}),
                            learned_spells_checked=len(checked), crafted_glyphs=len(used_glyphs),
                            crafted_zero_category_glyphs=sum(glyphs[g]['category'] & 255 == 0 for g in used_glyphs if g in glyphs),
                            saved_glyph_rows=sum(int(r['n']) for r in saved)),
                source_hashes={n: t.sha256 for n, t in tables.items()},
                sql_export_hashes=sql_hashes,
                limitations='Checks direct profession catalog recipes and direct trainer/vendor/quest sources, not every scripted or loot source. Native glyph tables/reagents require empty SQL overlays. Does not simulate crafting or client visuals.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dbc', type=Path, default=Path('build-extractors/bin/Release/dbc/enUS'))
    parser.add_argument('--sql-directory', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = audit(args.dbc, args.sql_directory)
    args.output.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result['counts']))
    print('Errors:', result['errors'])
    raise SystemExit(bool(result['errors']))
