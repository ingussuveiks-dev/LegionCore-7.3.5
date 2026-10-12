"""Read the 7.3.5 client spell definitions used by the pet-healing regression.

This is a native DB2 snapshot, not a replacement for querying live hotfix rows
or verifying healing/visibility in the client.
"""
import argparse
import json
import sys
from pathlib import Path

sys.dont_write_bytecode = True
from audit_spell_mechanics import Data
from audit_spell_visuals import Table


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dbc', type=Path, default=Path('build-extractors/bin/Release/dbc/enUS'))
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    data = Data(args.dbc, Path('__native_db2_only__'))
    table = Table(args.dbc / 'Spell.db2')

    def string(offset):
        if not offset:
            return ''
        start = table.strings + offset
        return table.data[start:table.data.index(0, start)].decode('utf8')

    ids = {755, 217979, 108366, 108370, 108415, 108446, 108447,
           198590, 219272, 228974, 234153}
    spells = [{key: string(row[key]) if key != 'ID' else row[key]
               for key in ('ID', 'Name', 'Description', 'AuraDescription')}
              for row in data.rows('Spell').values() if row['ID'] in ids]
    effects = [row for row in data.rows('SpellEffect').values() if row['SpellID'] in ids]
    misc = [row for row in data.rows('SpellMisc').values() if row['SpellID'] in ids]
    specs = [row for row in data.rows('SpecializationSpells').values() if row['SpellID'] in ids]
    index = {(row['SpellID'], row['EffectIndex']): row for row in effects if row['DifficultyID'] == 0}
    assert index[217979, 0]['EffectBasePoints'] == 4
    assert index[217979, 1]['EffectBasePoints'] == 8
    assert index[198590, 0]['EffectAmplitude'] == 2
    assert index[234153, 0]['EffectAmplitude'] == 6
    assert index[108370, 0]['EffectBasePoints'] == 10
    assert index[108370, 1]['EffectBasePoints'] == 15
    assert index[108446, 1]['EffectBasePoints'] == 25
    assert index[108446, 2]['EffectBasePoints'] == 50
    assert index[219272, 0]['EffectBasePoints'] == 1
    assert index[219272, 0]['EffectAuraPeriod'] == 1000
    assert index[219272, 1]['EffectBasePoints'] == 20
    assert {row['SpecID'] for row in specs if row['SpellID'] == 108415} == {266}
    result = {'build': 26972, 'source': 'native runtime DB2; no SQL overlays',
              'spells': spells, 'effects': effects, 'misc': misc,
              'specializations': specs, 'sha256': data.hashes}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf8')
    print(f'PASS: native pet-healing definitions, {len(spells)} spells; {args.output}')


if __name__ == '__main__':
    main()
