"""Verify the reconstructed 192962 reward against the installed 7.3.5 DB2s.

This establishes data compatibility, not Blizzard's original server-only mapping.
"""
import argparse
import csv
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
from audit_spell_visuals import Table

CLASSES = ['WARRIOR', 'PALADIN', 'HUNTER', 'ROGUE', 'PRIEST', 'DEATH_KNIGHT',
           'SHAMAN', 'MAGE', 'WARLOCK', 'MONK', 'DRUID', 'DEMON_HUNTER']
MISSING_TRIGGERS = {192969, 192975, 192970, 192971, 192972, 192973,
                    192974, 192976, 192978, 192979, 192980, 192981}
SCRIPT = 'spell_gen_inscription_class_glyph'


def audit(dbc, sql_directory):
    metadata = Path('src/server/game/DataStores/DB2Metadata.h').read_text()
    tables = {}

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
        with (sql_directory / name).open(encoding='utf-8-sig', newline='') as stream:
            return list(csv.DictReader(stream, delimiter='\t'))

    source = Path('src/server/scripts/Spells/spell_generic.cpp').read_text(encoding='utf-8')
    body = source.split('class ' + SCRIPT + ' :')[1].split('\n};')[0]
    mapping = dict(re.findall(r'case CLASS_(\w+):\s+return (\d+);', body))
    errors = []
    if set(mapping) != set(CLASSES):
        errors.append('Class selector must cover exactly the twelve Legion classes')
    bindings = sql('chain-script-bindings.tsv')
    if sum(r['ScriptName'] == SCRIPT and int(r['spell_id']) == 192962 for r in bindings) != 1:
        errors.append('Missing or duplicate 192962 script binding')
    if 'RegisterSpellScript(' + SCRIPT + ');' not in source:
        errors.append('Script is not registered')
    for phase in ('OnEffectLaunch', 'OnEffectLaunchTarget'):
        if not re.search(phase + r' \+= SpellEffectFn\(' + SCRIPT + r'::PreventMissingTrigger, EFFECT_ALL, SPELL_EFFECT_TRIGGER_SPELL\)', body):
            errors.append('Missing all-effect suppression in ' + phase)
    spells = table('Spell')
    names = {spells.ids[i]: spells.name(i) for i in range(spells.count)}
    for dest, origin in spells.copy_pairs:
        names[dest] = names[origin]
    effects = rows('SpellEffect', {'effect': 1, 'index': 3, 'item': 12, 'trigger': 16, 'misc': 26})
    reward_effects = [e for e in effects.values() if e.get('parent') == 192962]
    if (len(reward_effects) != 12 or {e['index'] for e in reward_effects} != set(range(12))
            or {e['effect'] for e in reward_effects} != {64}
            or {e['trigger'] for e in reward_effects} != MISSING_TRIGGERS):
        errors.append('Unexpected native 192962 trigger layout')
    abilities = rows('SkillLineAbility', {'spell': 2, 'skill': 4})
    items = rows('Item', {'class': 1, 'subclass': 2})
    uses = rows('ItemEffect', {'spell': 0})
    glyphs = rows('GlyphProperties', {'spell': 0})
    binds = rows('GlyphBindableSpell', {'spell': 0})
    result = []
    relevant_spells = {192962}
    for class_id, class_name in enumerate(CLASSES, 1):
        spell = int(mapping.get(class_name, 0))
        relevant_spells.add(spell)
        if spell not in names or not any(a['spell'] == spell and a['skill'] & 0xffff == 773 for a in abilities.values()):
            errors.append(f'{class_name}: recipe {spell} is not a native Inscription ability')
        creates = [e['item'] for e in effects.values() if e.get('parent') == spell and e['effect'] == 24]
        if len(creates) != 1:
            errors.append(f'{class_name}: recipe must create exactly one glyph type')
            continue
        item = creates[0]
        if item not in items or items[item]['class'] & 255 != 16 or items[item]['subclass'] & 255 != class_id:
            errors.append(f'{class_name}: item {item} has the wrong glyph class')
        use_spells = {u['spell'] for u in uses.values() if u.get('parent') == item}
        relevant_spells.update(use_spells)
        properties = set()
        for e in effects.values():
            if e.get('parent') in use_spells and e['effect'] == 74:
                properties.add(e['misc'][0] if isinstance(e['misc'], list) else e['misc'] & 0xffffffff)
        if len(properties) != 1:
            errors.append(f'{class_name}: glyph item needs one valid APPLY_GLYPH effect')
        glyph_spells, bindable = set(), set()
        for prop in properties:
            if prop not in glyphs or glyphs[prop]['spell'] not in names:
                errors.append(f'{class_name}: missing glyph properties or aura {prop}')
            else:
                glyph_spells.add(glyphs[prop]['spell'])
            bindable.update(b['spell'] for b in binds.values() if b.get('parent') == prop)
        if not bindable or not bindable <= names.keys():
            errors.append(f'{class_name}: missing bindable player spell')
        relevant_spells.update(glyph_spells | bindable)
        result.append(dict(class_id=class_id, class_name=class_name, recipe=spell, name=names.get(spell),
                           item=item, item_use_spells=sorted(use_spells), glyph_properties=sorted(properties),
                           glyph_spells=sorted(glyph_spells), bindable_spells=sorted(bindable)))
    # Fail closed: this audit does not decode arbitrary hotfix overlays on these chains.
    if any(int(r['SpellID']) in relevant_spells for r in sql('chain-hotfix-effects.tsv')):
        errors.append('Relevant spell_effect SQL overrides require manual overlay review')
    return dict(build='7.3.5.26972', mapping_status='Reconstruction; original server-only mapping not fully confirmed',
                recipes=result, suppressed_trigger_ids=sorted(MISSING_TRIGGERS), errors=errors,
                source_hashes={name: t.sha256 for name, t in tables.items()})


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dbc', type=Path, default=Path('build-extractors/bin/Release/dbc/enUS'))
    parser.add_argument('--sql-directory', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = audit(args.dbc, args.sql_directory)
    args.output.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(dict(classes=len(result['recipes']), errors=result['errors'])))
    raise SystemExit(bool(result['errors']))
