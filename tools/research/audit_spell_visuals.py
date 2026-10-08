"""Read regular Legion WDC1 tables and audit SpellXSpellVisual references.

No client data is modified. Python 3, standard library only.
"""
import argparse
import collections
import csv
import hashlib
import json
from pathlib import Path
import re
import struct


class Table:
    def __init__(self, path, index_field=-1, layout=None):
        self.data = b = path.read_bytes()
        self.sha256 = hashlib.sha256(b).hexdigest()
        self.u32 = lambda p: struct.unpack_from('<I', b, p)[0]
        u = self.u32
        if b[:4] != b'WDC1' or struct.unpack_from('<H', b, 44)[0] & 1:
            raise ValueError(f'{path}: requires regular WDC1')
        if layout is not None and u(24) != layout:
            raise ValueError(f'{path}: unexpected layout hash')
        self.count, self.fields, self.size = u(4), u(8), u(12)
        self.start = 84 + 4 * self.fields
        self.strings = self.start + self.count * self.size
        ids = self.strings + u(16)
        copies = ids + u(64)
        meta = copies + u(40)
        pallet = meta + u(68)
        common = pallet + u(76)
        relation = common + u(72)
        if relation + u(80) != len(b) or u(68) != self.fields * 24:
            raise ValueError(f'{path}: unexpected section sizes')
        self.columns = []
        for f in range(self.fields):
            bit, width, extra, kind, default, packed_width, array = struct.unpack_from('<HHIIIII', b, meta + f * 24)
            if kind in (1, 3, 4):
                bit, width = u(52) * 8 + default, packed_width
            values = {}
            if kind == 2:
                values = dict(struct.iter_unpack('<II', b[common:common + extra]))
                common += extra
            self.columns.append((bit, width, kind, default, array, pallet, values))
            if kind in (3, 4):
                pallet += extra
        if pallet != meta + u(68) + u(76) or common != relation:
            raise ValueError(f'{path}: invalid auxiliary sections')
        self.ids = [u(ids + 4 * i) for i in range(self.count)] if u(64) else []
        if not self.ids:
            if index_field < 0:
                raise ValueError(f'{path}: no ID field')
            self.ids = [self.value(i, index_field) for i in range(self.count)]
        self.copy_pairs = list(struct.iter_unpack('<II', b[copies:meta]))
        self.parents = {}
        if u(80):
            if 12 + u(relation) * 8 != u(80):
                raise ValueError(f'{path}: invalid relationship section')
            self.parents = {i: parent for parent, i in struct.iter_unpack('<II', b[relation + 12:])}
        self.all_ids = set(self.ids) | {dest for dest, _ in self.copy_pairs}

    def value(self, i, f):
        bit, width, kind, default, array, pallet, common = self.columns[f]
        if kind == 2:
            return common.get(self.ids[i], default)
        if bit + width > self.size * 8:
            raise ValueError('Column outside record')
        p = self.start + i * self.size + bit // 8
        value = int.from_bytes(self.data[p:p + (bit % 8 + width + 7) // 8], 'little')
        value = (value >> (bit % 8)) & ((1 << width) - 1)
        if kind == 3:
            return self.u32(pallet + 4 * value)
        if kind == 4:
            return [self.u32(pallet + 4 * (value * array + a)) for a in range(array)]
        if kind not in (0, 1):
            raise ValueError(f'Unsupported compression {kind}')
        return value

    def name(self, i):
        p = self.strings + self.value(i, 0)
        return self.data[p:self.data.index(0, p)].decode('utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dbc', type=Path, default=Path('build-extractors/bin/Release/dbc/enUS'))
    parser.add_argument('--hotfix-visuals', type=Path)
    parser.add_argument('--hotfix-spells', type=Path, help='TSV with an ID column')
    parser.add_argument('--character-spells', type=Path)
    parser.add_argument('--world-visuals', type=Path, help='TSV: spellId, SpellVisualID')
    parser.add_argument('--world-kits', type=Path, help='TSV: spellId, KitRecID')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--strict', action='store_true', help='Fail on conflicting hotfix ownership or missing visual assets')
    args = parser.parse_args()
    tables = {}
    def table(name, index=-1, layout=None):
        tables[name] = Table(args.dbc / (name + '.db2'), index, layout)
        return tables[name]
    spells = table('Spell', layout=0x2273DFFF)
    names = dict(zip(spells.ids, (spells.name(i) for i in range(spells.count))))
    for dest, source in spells.copy_pairs:
        names[dest] = names[source]
    if args.hotfix_spells:
        with args.hotfix_spells.open(encoding='utf-8-sig') as stream:
            for row in csv.DictReader(stream, delimiter='\t'):
                names.setdefault(int(row['ID']), '(hotfix-only spell)')
    links = table('SpellXSpellVisual', 1, 0x4F4B8A2A)
    visuals = table('SpellVisual', layout=0x1C1301D2)
    kits = table('SpellVisualKit', layout=0xDC04F488)
    fields = ['SpellVisualID', 'ID', 'Probability', 'CasterPlayerConditionID', 'CasterUnitConditionID',
              'ViewerPlayerConditionID', 'ViewerUnitConditionID', 'SpellIconFileID', 'ActiveIconFileID',
              'Flags', 'DifficultyID', 'Priority']
    def link_value(i, f):
        value = links.value(i, f)
        if f == 2:
            return struct.unpack('<f', struct.pack('<I', value))[0]
        if f in (3, 4, 5, 6):
            return value & 0xffff
        if f in (9, 10, 11):
            return value & 0xff
        return value
    raw = {links.ids[i]: dict(zip(fields, (link_value(i, f) for f in range(links.fields))),
                            SpellID=links.parents.get(i, 0)) for i in range(links.count)}
    effective = dict(raw)
    collisions = []
    if args.hotfix_visuals:
        with args.hotfix_visuals.open(encoding='utf-8-sig') as stream:
            for row in csv.DictReader(stream, delimiter='\t'):
                row = {k: float(v) if k == 'Probability' else int(v) for k, v in row.items()}
                old = raw.get(row['ID'])
                if old and old['SpellID'] != row['SpellID']:
                    collisions.append(dict(ID=row['ID'], original=old, replacement=row,
                                           original_name=names.get(old['SpellID'], '')))
                effective[row['ID']] = row
    by_spell = collections.defaultdict(list)
    for row in effective.values():
        by_spell[row['SpellID']].append(row)
    # All skill-line abilities (including professions, racial and pet skills),
    # specialization abilities and talent spells, plus persisted character spells.
    skills = table('SkillLineAbility', 1, 0x97B5A653)
    specs = table('SpecializationSpells', 5, 0xAE3436F3)
    talents = table('Talent', layout=0xE8850B48)
    candidates = {skills.value(i, 2) for i in range(skills.count)}
    candidates |= {specs.value(i, 1) for i in range(specs.count)}
    candidates |= {talents.value(i, 1) for i in range(talents.count)}
    characters = set()
    if args.character_spells:
        with args.character_spells.open(encoding='utf-8-sig') as stream:
            characters = {int(row['spell']) for row in csv.DictReader(stream, delimiter='\t')}
        candidates |= characters
    candidates.discard(0)
    class_candidates = {skills.value(i, 2) for i in range(skills.count) if skills.value(i, 10)}
    class_candidates |= {specs.value(i, 1) for i in range(specs.count)}
    class_candidates |= {talents.value(i, 1) for i in range(talents.count)}
    class_candidates.discard(0)
    current_spec_spells = {specs.value(i, 1) for i in range(specs.count)}
    current_talent_spells = {talents.value(i, 1) for i in range(talents.count)}
    misc = table('SpellMisc', layout=0xCDC114D5)
    passive = {misc.parents[i] for i in range(misc.count)
               if i in misc.parents and misc.value(i, 9)[0] & 0x40}
    world_errors = []
    world_count = 0
    for path, field, assets in ((args.world_visuals, 'SpellVisualID', visuals.all_ids),
                                (args.world_kits, 'KitRecID', kits.all_ids)):
        if path:
            with path.open(encoding='utf-8-sig') as stream:
                for row in csv.DictReader(stream, delimiter='\t'):
                    world_count += 1
                    if int(row[field]) not in assets:
                        world_errors.append(row)
    script_kits = []
    for path in sorted(Path('src/server/scripts/Spells').glob('*.cpp')):
        for number, line in enumerate(path.read_text(encoding='utf-8').splitlines(), 1):
            match = re.search(r'(?:SendPlay|Cancel)SpellVisualKit\((\d+)', line)
            if match:
                kit = int(match[1])
                script_kits.append(dict(file=path.name, line=number, kit=kit, exists=kit in kits.all_ids))
    missing_assets = [r for r in effective.values() if r['SpellVisualID'] and r['SpellVisualID'] not in visuals.all_ids]
    result = dict(
        source_hashes={n: t.sha256 for n, t in tables.items()},
        counts=dict(spells=len(names), visual_links=len(effective), visual_assets=len(visuals.all_ids),
                    visual_kits=len(kits.all_ids), candidate_spells=len(candidates), class_spells=len(class_candidates),
                    character_spells=len(characters), world_visual_rows=world_count),
        hotfix_collisions=collisions,
        missing_visual_assets=missing_assets,
        candidate_missing_spells=sorted(candidates - names.keys()),
        candidate_without_direct_visual=[dict(ID=s, name=names.get(s, '')) for s in sorted(candidates) if not by_spell[s]],
        candidate_broken_visuals=[r for r in missing_assets if r['SpellID'] in candidates],
        class_missing_spells=sorted(class_candidates - names.keys()),
        specialization_missing_spells=sorted(current_spec_spells - names.keys() - {0}),
        talent_missing_spells=sorted(current_talent_spells - names.keys() - {0}),
        class_active_without_direct_visual=[dict(ID=s, name=names.get(s, '')) for s in sorted(class_candidates & names.keys() - passive) if not by_spell[s]],
        world_missing_visual_assets=world_errors,
        script_visual_kits=script_kits,
        character_spells=[dict(ID=s, name=names.get(s, ''), passive=s in passive, visuals=by_spell[s]) for s in sorted(characters)],
    )
    args.output.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result['counts']))
    for key in ('hotfix_collisions', 'missing_visual_assets', 'candidate_missing_spells', 'candidate_without_direct_visual', 'candidate_broken_visuals'):
        print(f'{key}: {len(result[key])}')
    if args.strict and (collisions or missing_assets or world_errors or any(not row['exists'] for row in script_kits)):
        raise SystemExit(1)


if __name__ == '__main__':
    main()
