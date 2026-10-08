"""Read-only 26972 talent, equipment, proc and supporting DB2 reference audit."""
import argparse
import collections
import csv
import hashlib
import json
from pathlib import Path
import re
import struct
import sys
sys.dont_write_bytecode = True
from audit_spell_visuals import Table


class Data:
    def __init__(self, dbc, sql):
        self.dbc, self.sql = dbc, sql
        self.meta = Path('src/server/game/DataStores/DB2Metadata.h').read_text()
        self.load = Path('src/server/game/DataStores/DB2LoadInfo.h').read_text()
        self.cache, self.native, self.overlaid, self.hashes, self.sql_hashes = {}, {}, {}, {}, {}
        self.deleted = {}

    def export(self, name):
        path = self.sql / name
        self.sql_hashes[name] = hashlib.sha256(path.read_bytes()).hexdigest()
        with path.open(encoding='utf-8-sig', newline='') as stream:
            return list(csv.DictReader(stream, delimiter='\t'))

    def rows(self, name):
        if name in self.cache:
            return self.cache[name]
        meta = re.split('struct ' + name + 'Meta\n', self.meta, flags=re.IGNORECASE)[1].split('\n};')[0]
        index, layout, parent = re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[\dA-Fa-f]+), types, arraySizes, (-?\d+)', meta).groups()
        index, parent = int(index), int(parent)
        types = re.search(r'types = "([^"]+)"', meta)[1]
        sizes = [int(v.strip()) for v in re.search(r'arraySizes\[\d+\] = \{([^}]+)', meta)[1].split(',')]
        info = re.split('struct ' + name + 'LoadInfo\n', self.load, flags=re.IGNORECASE)[1].split('\n};')[0]
        fields = re.findall(r'\{ (true|false), FT_\w+, "([^"]+)" \}', info)
        if index < 0:
            fields = [f for f in fields if f[1] != 'ID']
        assert len(fields) == sum(sizes), (name, len(fields), sizes)
        table = Table(self.dbc / (name + '.db2'), index, int(layout, 16))
        self.hashes[name] = table.sha256
        result = {}
        for row in range(table.count):
            record, offset = {'ID': table.ids[row]}, 0
            for col, (kind, count) in enumerate(zip(types, sizes)):
                value = table.value(row, col) if col < table.fields else table.parents.get(row, 0)
                assert col < table.fields or col == parent, (name, col)
                width = {'b': 8, 'h': 16, 'i': 32, 'f': 32, 's': 32, 'l': 64}[kind]
                values = value if isinstance(value, list) else [(value >> (n * width)) & ((1 << width) - 1) for n in range(count)]
                for (signed, field), value in zip(fields[offset:offset + count], values):
                    if kind == 'f':
                        value = struct.unpack('<f', struct.pack('<I', value))[0]
                    elif signed == 'true' and kind != 's' and value & (1 << (width - 1)):
                        value -= 1 << width
                    record[field] = value
                offset += count
            result[table.ids[row]] = record
        for dest, source in table.copy_pairs:
            result[dest] = dict(result[source], ID=dest)
        self.native[name] = result.copy()
        snake = re.sub(r'(?<!^)(?=[A-Z])', '_', name).lower()
        overlay = self.sql / ('mechanics-' + snake + '.tsv')
        if overlay.exists():
            sql_types = dict((field, kind) for kind, field in re.findall(r'\{ (?:true|false), FT_(\w+), "([^"]+)" \}', info))
            for row in self.export(overlay.name):
                self.overlaid.setdefault(name, set()).add(int(row['ID']))
                result[int(row['ID'])] = {k: v if sql_types[k].startswith('STRING') else float(v) if sql_types[k] == 'FLOAT' else int(v)
                                          for k, v in row.items() if k != 'VerifiedBuild'}
        tombstones = self.sql / 'mechanics-link-hotfixes.tsv'
        if tombstones.exists():
            table_hash = struct.unpack_from('<I', table.data, 20)[0]
            latest = {}
            for row in sorted(self.export(tombstones.name), key=lambda r: int(r['Id'])):
                if int(row['TableHash']) == table_hash:
                    latest[int(row['RecordID'])] = int(row['Deleted'])
            self.deleted[name] = [id for id, deleted in latest.items() if deleted]
            for id in self.deleted[name]:
                result.pop(id, None)
        self.cache[name] = result
        return result


def audit(data):
    counts = data.export('mechanics-hotfix-counts.tsv')
    for row in counts:
        name = 'mechanics-' + row['table_name'] + '.tsv'
        if int(row['rows_count']) and not (data.sql / name).exists():
            raise ValueError('Missing nonempty SQL overlay: ' + row['table_name'])
        if (data.sql / name).exists() and len(data.export(name)) != int(row['rows_count']):
            raise ValueError('Stale/incomplete SQL overlay: ' + row['table_name'])
    spells = set(data.rows('Spell')) | {int(r['ID']) for r in data.export('visual-hotfix-spells.tsv')}
    issues = collections.defaultdict(list)
    def ref(group, record, field, target, optional=True):
        value = record[field]
        if (value or not optional) and value not in target:
            issues[group].append({'row': record['ID'], 'field': field, 'target': value})
    specs = data.rows('ChrSpecialization')
    for name in ('Talent', 'PvpTalent', 'SpecializationSpells'):
        for r in data.rows(name).values():
            ref(name, r, 'SpellID', spells, False)
            ref(name, r, 'OverridesSpellID' if name != 'PvpTalent' else 'OverrideSpellID', spells)
            ref(name, r, 'SpecID', specs)
            if name != 'SpecializationSpells':
                spec = specs.get(r['SpecID'])
                if spec and spec['ClassID'] != r['ClassID']:
                    issues[name].append({'row': r['ID'], 'error': 'class/spec mismatch'})
                if not 0 <= r['ColumnIndex'] < 3 or not 0 <= r['TierID'] < (7 if name == 'Talent' else 6):
                    issues[name].append({'row': r['ID'], 'error': 'slot out of range'})
    for r in specs.values():
        for field in ('MasterySpellID1', 'MasterySpellID2'):
            ref('mastery', r, field, spells)
    ppm = data.rows('SpellProcsPerMinute')
    aura = data.rows('SpellAuraOptions')
    for r in aura.values():
        if r['SpellID'] in spells:
            ref('aura_ppm', r, 'SpellProcsPerMinuteID', ppm)
    for r in data.rows('SpellProcsPerMinuteMod').values():
        ref('ppm_mod', r, 'SpellProcsPerMinuteID', ppm, False)
    # Presence and references, not a claim that every DB2 value is an intended
    # balance rule. Server-side scripted corrections are intentionally separate.
    for name in ('SpellEquippedItems', 'SpellCooldowns', 'SpellPower', 'SpellReagentsCurrency', 'SpellTotems', 'SkillRaceClassInfo'):
        data.rows(name)
    for r in data.rows('SpellReagentsCurrency').values():
        ref('currency_reagent', r, 'CurrencyTypesID', data.rows('CurrencyTypes'), False)
    for r in data.rows('SpellTotems').values():
        if r['SpellID'] in spells:
            for field in ('RequiredTotemCategoryID1', 'RequiredTotemCategoryID2'):
                ref('totem_category', r, field, data.rows('TotemCategory'))
    criteria = data.rows('Criteria')
    tree = data.rows('CriteriaTree')
    for r in tree.values():
        ref('criteria_tree', r, 'CriteriaID', criteria)
        ref('criteria_parent', r, 'Parent', tree)
    for r in data.rows('ScenarioStep').values():
        ref('scenario_tree', r, 'Criteriatreeid', tree)
        ref('scenario', r, 'ScenarioID', data.rows('Scenario'), False)
    origins = {'SpecializationSpells': 'SpecializationSpells', 'Talent': 'Talent', 'PvpTalent': 'PvpTalent',
               'criteria_tree': 'CriteriaTree', 'criteria_parent': 'CriteriaTree',
               'scenario_tree': 'ScenarioStep', 'scenario': 'ScenarioStep', 'mastery': 'ChrSpecialization',
               'aura_ppm': 'SpellAuraOptions', 'ppm_mod': 'SpellProcsPerMinuteMod',
               'currency_reagent': 'SpellReagentsCurrency', 'totem_category': 'SpellTotems'}
    for group, entries in issues.items():
        for entry in entries:
            entry['source'] = 'SQL overlay' if entry['row'] in data.overlaid.get(origins[group], set()) else 'native DB2'
    proc = data.export('mechanics-proc.tsv')
    aura_by_spell = collections.defaultdict(list)
    for r in aura.values():
        aura_by_spell[r['SpellID']].append(r)
    modcharges = []
    for r in proc:
        if int(r['modcharges']):
            sid = abs(int(r['spellId']))
            modcharges.append({'spell': sid, 'charges': int(r['charges']), 'modcharges': int(r['modcharges']),
                               'effective_stack_limits': sorted({a['CumulativeAura'] for a in aura_by_spell[sid]})})
    return {'build': '7.3.5.26972', 'counts': {n: len(v) for n, v in data.cache.items()},
            'reference_findings': dict(issues), 'proc_modcharges': modcharges,
            'retired_links': {name: ids for name, ids in data.deleted.items() if ids},
            'exodar_tree_matches_native': all(tree[7898][f] == data.native['CriteriaTree'][7898][f]
                                              for f in ('CriteriaID', 'Parent', 'OrderIndex', 'Amount', 'Operator')),
            'source_hashes': data.hashes, 'sql_export_hashes': data.sql_hashes,
            'limitations': 'Reference findings need classification: native obsolete rows and deliberate server overrides are not automatically errors. Does not verify rendering, balance, all scripts or live transitions.'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dbc', type=Path, default=Path('build-extractors/bin/Release/dbc/enUS'))
    parser.add_argument('--sql-directory', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = audit(Data(args.dbc, args.sql_directory))
    args.output.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result['counts']))
    print('Reference findings:', {k: len(v) for k, v in result['reference_findings'].items()})
