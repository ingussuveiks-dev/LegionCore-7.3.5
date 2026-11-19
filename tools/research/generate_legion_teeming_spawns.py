"""Render reviewed reconstructed 7.3.5 content; never infer new placements.

The catalog records historical addon provenance and native geometry receipts.
Output stays outside the live updater until its matching executable is ready.
"""
import argparse
import json
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CATALOG = ROOT / 'docs/research/legion-teeming-reconstruction-2026-11-19.json'


def check_script_header(data):
    header = (ROOT/'src/server/scripts/Legion/MawofSouls/MawSkjalCrew.h').read_text(encoding='utf-8')
    block = header.split('Spawns={{', 1)[1].split('}};', 1)[0]
    actual = [(int(entry), [float(x.strip().rstrip('fF')) for x in pose.split(',')]) for entry, pose in re.findall(r'\{(\d+)u,\{([^}]+)\}\}', block)]
    scripted = [e for e in data['entries'] if e['storage'] == 'instance_script']
    packed = lambda values: struct.pack('<'+'f'*len(values), *values)
    assert len(actual) == 23 and len(scripted) == 7
    for (entry, pose), expected in zip(actual[16:], scripted):
        assert entry == expected['npc'] and packed(pose) == packed(expected['pose']+[expected['orientation']]), expected['candidate_id']
    source = (ROOT/'src/server/scripts/Legion/MawofSouls/boss_helya.cpp').read_text(encoding='utf-8')
    block = source.split('addsPos[18]', 1)[1].split('};', 1)[0]
    original = [[float(x.strip().rstrip('fF')) for x in pose.split(',')] for pose in re.findall(r'\{\s*([-\d.fF, ]+)\}', block)]
    assert len(original) == 18
    for (_, pose), expected in zip(actual[:16], original[2:]):
        assert packed(pose) == packed(expected), 'Original normal Skjal home changed'


def literal(value):
    if isinstance(value, str):
        return "'" + value.replace('\\', '\\\\').replace("'", "''") + "'"
    if isinstance(value, bool):
        return str(int(value))
    return repr(value)


def insert(table, rows):
    keys = list(rows[0])
    assert all(list(row) == keys for row in rows), table
    return 'INSERT INTO `' + table + '` (' + ','.join('`'+k+'`' for k in keys) + ') VALUES ' + ','.join('('+','.join(literal(row[k]) for k in keys)+')' for row in rows) + ';'


def render(data):
    entries = data['entries']
    db = [e for e in entries if e['storage'] == 'creature']
    assert len(entries) == 222 and len(db) == 215
    assert len({(e['route'], e['candidate_id']) for e in entries}) == 222
    assert {e['creature']['guid'] for e in db} == set(range(900902000, 900902215))
    assert all(e['creature']['spawnMask'] == 256 and e['creature']['isTeemingSpawn'] == 1 and e['creature']['phaseMask'] == 1 and not e['creature']['PhaseId'] for e in db)
    tables = {'creature': [e['creature'] for e in db], 'creature_addon': [e['addon'] for e in db], 'waypoint_data': data['waypoints'], 'creature_formations': data['formations']}
    primary = {'creature': ['guid'], 'creature_addon': ['guid'], 'waypoint_data': ['id', 'point'], 'creature_formations': ['memberGUID']}
    lines = ['-- Reconstructed Legion7.3.5 Teeming content:215 DB spawns plus7 instance-owned Skjal additions.',
             '-- Historical MDT aaabf806caac2cc519b692bce68c9ce38e934974;native26972 floor/model/path receipts in the catalog.',
             '-- These are reviewed reconstructions,not recovered original server GUIDs or client traversal evidence.',
             'START TRANSACTION;',
             'CREATE TEMPORARY TABLE legion_teeming_guard (valid TINYINT NOT NULL CHECK(valid=1));']
    for table, rows in tables.items():
        expected = 'legion_teeming_expected_' + table
        lines += [f'CREATE TEMPORARY TABLE `{expected}` LIKE `{table}`;', insert(expected, rows)]
        join = ' AND '.join(f'a.`{k}`=e.`{k}`' for k in primary[table])
        same = ' AND '.join(f'(a.`{k}` <=> e.`{k}`)' for k in rows[0])
        # A collision or locally edited row fails before any persistent write.
        lines += [f'INSERT INTO legion_teeming_guard SELECT COUNT(*)=0 FROM `{table}` a JOIN `{expected}` e ON {join} WHERE NOT({same});']
    guards = data['existing_formation_guards']
    expected = 'legion_teeming_original_formations'
    lines += [f'CREATE TEMPORARY TABLE `{expected}` LIKE creature_formations;', insert(expected, guards)]
    same = ' AND '.join(f'(a.`{k}` <=> e.`{k}`)' for k in guards[0])
    lines += [f'INSERT INTO legion_teeming_guard SELECT COUNT(*)=0 FROM `{expected}` e LEFT JOIN creature_formations a ON a.memberGUID=e.memberGUID WHERE a.memberGUID IS NULL OR NOT({same});']
    # A reserved path cannot contain unrelated additional points.
    lines += ['INSERT INTO legion_teeming_guard SELECT COUNT(*)=0 FROM waypoint_data a LEFT JOIN legion_teeming_expected_waypoint_data e ON a.id=e.id AND a.point=e.point WHERE a.id BETWEEN 1467062000 AND 1467062009 AND e.id IS NULL;']
    for table in tables:
        expected = 'legion_teeming_expected_' + table
        join = ' AND '.join(f'a.`{k}`=e.`{k}`' for k in primary[table])
        first = primary[table][0]
        lines += [f'INSERT INTO `{table}` SELECT e.* FROM `{expected}` e LEFT JOIN `{table}` a ON {join} WHERE a.`{first}` IS NULL;']
    lines += ['COMMIT;']
    lines += [f'DROP TEMPORARY TABLE `legion_teeming_expected_{table}`;' for table in tables]
    lines += ['DROP TEMPORARY TABLE legion_teeming_original_formations;', 'DROP TEMPORARY TABLE legion_teeming_guard;']
    return '\n'.join(lines)+'\n'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--catalog', type=Path, default=CATALOG)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--fixture-directory', type=Path)
    parser.add_argument('--verify-live-sql', type=Path)
    args = parser.parse_args()
    data = json.loads(args.catalog.read_text(encoding='utf-8'))
    check_script_header(data)
    sql = render(data)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(sql, encoding='utf-8', newline='\n')
    if args.verify_live_sql:
        db = [e for e in data['entries'] if e['storage'] == 'creature']
        tables = {'creature': [e['creature'] for e in db], 'creature_addon': [e['addon'] for e in db], 'waypoint_data': data['waypoints'], 'creature_formations': data['formations']}
        primary = {'creature': ['guid'], 'creature_addon': ['guid'], 'waypoint_data': ['id', 'point'], 'creature_formations': ['memberGUID']}
        verify = ['SET SESSION TRANSACTION READ WRITE;']
        for table in tables:
            verify.append(f'CREATE TEMPORARY TABLE `legion_verify_{table}` LIKE `{table}`;')
        verify.append('SET SESSION TRANSACTION READ ONLY;')
        for table, rows in tables.items():
            expected = 'legion_verify_'+table
            verify.append(insert(expected, rows))
            join = ' AND '.join(f'a.`{k}`=e.`{k}`' for k in primary[table])
            same = ' AND '.join(f'(a.`{k}` <=> e.`{k}`)' for k in rows[0])
            first = primary[table][0]
            verify.append(f"SELECT JSON_OBJECT('table','{table}','expected',{len(rows)},'joined',COUNT(*),'matched',SUM(a.`{first}` IS NOT NULL AND {same})) FROM `{expected}` e LEFT JOIN `{table}` a ON {join};")
        args.verify_live_sql.write_text('\n'.join(verify)+'\n', encoding='utf-8', newline='\n')
    if args.fixture_directory:
        args.fixture_directory.mkdir(parents=True, exist_ok=True)
        prelude = []
        for table in ('creature', 'creature_addon', 'waypoint_data', 'creature_formations'):
            seed = 'reconstruction_seed_' + table
            if table == 'creature_formations':
                ids = ','.join(str(x['memberGUID']) for x in data['existing_formation_guards'])
                prelude.append(f'CREATE TEMPORARY TABLE `{seed}` AS SELECT * FROM `{table}` WHERE memberGUID IN({ids});')
            else:
                prelude.append(f'CREATE TEMPORARY TABLE `{seed}` LIKE `{table}`;')
            prelude += [f'CREATE TEMPORARY TABLE `{table}` LIKE `{seed}`;', f'INSERT INTO `{table}` SELECT * FROM `{seed}`;']
        (args.fixture_directory/'shadow.sql').write_text('\n'.join(prelude)+'\n', encoding='utf-8')
        records = []
        for e in data['entries']:
            if e['storage'] != 'creature':
                continue
            records.append(' '.join(map(str, [e['candidate_id'], e['route'], e['creature']['guid'], e['npc'], e['map'], *e['pose'], e['orientation']])))
        (args.fixture_directory/'native-rows.tsv').write_text('\n'.join(records)+'\n', encoding='utf-8')
    print('Rendered215 guarded native-profile DB spawns/10 paths;seven Skjal additions remain instance-owned.')
