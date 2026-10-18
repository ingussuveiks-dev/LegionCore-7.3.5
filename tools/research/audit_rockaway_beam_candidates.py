"""Trace candidate beam assets in the local 7.3.5 WDC1 catalog, read-only.

An asset relationship is not evidence of which retail NPC cast the spell.
SpellVisualKitEffect type 1 references SpellProceduralEffect; procedural
type 26 carries a SpellChainEffects ID in its first FLOAT value. This is
separate from kit-effect type 13 (BeamEffect). Preserve that distinction.
"""
import argparse
import json
from pathlib import Path
import re
import struct

from audit_spell_visuals import Table


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dbc', type=Path, default=Path('build-extractors/bin/Release/dbc/enUS'))
    parser.add_argument('--spell', type=int, action='append', help='Repeat to select candidate IDs')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    selected = set(args.spell or [231328, 231333, 231433, 228021, 228854,
                                 231203, 229905, 229911, 195062, 211670])
    metadata = Path('src/server/game/DataStores/DB2Metadata.h').read_text()
    hashes = {}

    def read(name):
        block = metadata.split('struct ' + name + 'Meta\n', 1)[1].split('\n};', 1)[0]
        index, layout = re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[0-9A-Fa-f]+)', block).groups()
        table = Table(args.dbc / (name + '.db2'), int(index), int(layout, 16))
        hashes[name] = table.sha256
        rows = {table.ids[i]: dict(id=table.ids[i], parent=table.parents.get(i),
                                  values=[table.value(i, f) for f in range(table.fields)])
                for i in range(table.count)}
        for dest, source in table.copy_pairs:
            rows[dest] = dict(rows[source], id=dest)
        return table, rows

    def array(value, count, bits=32):
        if isinstance(value, list):
            assert len(value) == count
            return value
        return [(value >> (i * bits)) & ((1 << bits) - 1) for i in range(count)]

    def floats(value):
        return [struct.unpack('<f', struct.pack('<I', v))[0] for v in array(value, 4)]

    st, spells = read('Spell')
    names = {st.ids[i]: st.name(i) for i in range(st.count)}
    for dest, source in st.copy_pairs:
        names[dest] = names[source]
    _, links = read('SpellXSpellVisual')
    _, visuals = read('SpellVisual')
    _, events = read('SpellVisualEvent')
    _, effects = read('SpellVisualKitEffect')
    _, procedures = read('SpellProceduralEffect')
    _, chains = read('SpellChainEffects')
    _, attachments = read('SpellVisualKitModelAttach')
    _, effect_names = read('SpellVisualEffectName')
    _, missiles = read('SpellVisualMissile')
    # Physical layouts pinned by the checked metadata / WDC1 header.
    assert len(next(iter(procedures.values()))['values']) == 3
    assert len(next(iter(chains.values()))['values']) == 60

    def chain_record(chain_id):
        row = chains[chain_id]['values']
        return dict(id=chain_id, rgba=[row[51], row[52], row[53], row[50]],
                    texture_file_ids=array(row[59], 3),
                    child_chain_ids=array(row[46], 11, 16),
                    start_width=struct.unpack('<f', struct.pack('<I', row[40]))[0],
                    end_width=struct.unpack('<f', struct.pack('<I', row[41]))[0])

    result = []
    for spell_id in sorted(selected):
        assert spell_id in spells, f'No native spell {spell_id}'
        visual_ids = {r['values'][0] for r in links.values() if r['parent'] == spell_id} - {0}
        spell_events = [r for r in events.values() if r['parent'] in visual_ids]
        kit_ids = {r['values'][7] for r in spell_events}
        kit_effects = [r for r in effects.values() if r['parent'] in kit_ids]
        procedure_rows = []
        chain_ids = set()
        for effect in kit_effects:
            if effect['values'][0] != 1:
                continue
            row = procedures[effect['values'][1]]
            values = floats(row['values'][0])
            procedure_rows.append(dict(id=row['id'], kit=effect['parent'],
                                       type=row['values'][1], values=values))
            if row['values'][1] == 26:
                assert values[0].is_integer()
                if values[0]:
                    chain_ids.add(int(values[0]))
        visited = set()
        pending = list(chain_ids)
        while pending:
            chain_id = pending.pop()
            if chain_id in visited:
                continue
            visited.add(chain_id)
            pending.extend(i for i in chain_record(chain_id)['child_chain_ids'] if i and i not in visited)
        attach_rows = [attachments[r['values'][1]] for r in kit_effects if r['values'][0] == 2]
        model_ids = {effect_names[r['values'][3]]['values'][10] for r in attach_rows}
        missile_sets = {visuals[v]['values'][3] & 0xffff for v in visual_ids} - {0}
        missile_rows = [r for r in missiles.values() if r['parent'] in missile_sets]
        missile_models = {effect_names[r['values'][5] & 0xffff]['values'][10] for r in missile_rows}
        result.append(dict(spell=spell_id, name=names[spell_id], visuals=sorted(visual_ids),
                           visual_attachments=[dict(visual=v, caster=visuals[v]['values'][5],
                                                    destination=visuals[v]['values'][4])
                                               for v in sorted(visual_ids)],
                           kit_effect_types=sorted({r['values'][0] for r in kit_effects}),
                           beam_effect_ids=sorted({r['values'][1] for r in kit_effects if r['values'][0] == 13}),
                           procedures=procedure_rows,
                           chains=[chain_record(i) for i in sorted(visited)],
                           attached_model_file_ids=sorted(model_ids),
                           missile_model_file_ids=sorted(missile_models),
                           retail_actor_binding_established=False))
    output = dict(build=26972, db2_sha256=hashes, candidates=result,
                  references=[
                      'https://github.com/wowdev/WoWDBDefs/blob/master/definitions/SpellVisualKitEffect.dbd',
                      'https://github.com/wowdev/WoWDBDefs/blob/master/definitions/SpellProceduralEffect.dbd',
                      'https://github.com/wowdev/WoWDBDefs/blob/master/definitions/SpellChainEffects.dbd',
                      'https://github.com/Marlamin/wow.tools.local/blob/5b2f85db477bd42205fde912ed72b3adadda218a/wwwroot/js/meta.js'],
                  boundary='Read-only native asset graph. No spell/caster/target binding is recovered from a sniff; '
                           'no client playback, visual match, original spell ID or actor assignment is verified.')
    args.output.write_text(json.dumps(output, indent=2) + '\n', encoding='utf-8')
    print(f'PASS: {len(result)} candidate asset graphs; original Rockaway beam binding remains unresolved.')


if __name__ == '__main__':
    main()
