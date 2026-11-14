"""Strict chronological composition of the five later command repair groups.

Historical artifact source hashes are unchanged. Only complete declared edits
are inverted; partial/foreign edits survive for the historical full-file check.
The actual current production files and every intervening Git baseline are
certified independently before any supplied source is normalized.
"""
from pathlib import Path
import functools
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]
NAMES = ('FiveTen20261114Repairs.json', 'NextFifty20261114Repairs.json',
         'MoreFifty20261114Repairs.json', 'FourthFifty20261114Repairs.json',
         'FifthFifty20261114Repairs.json')
PLANS = tuple(json.loads((ROOT / 'tools/tests' / name).read_text(encoding='utf-8')) for name in NAMES)
PATHS = frozenset(path for plan in PLANS for path in plan['files'])


def _sha(text):
    return hashlib.sha256(text.encode('utf-8')).hexdigest()


def _edits(plan):
    return plan['changes'] + plan['extra_edits']


@functools.lru_cache(None)
def verify():
    for path in sorted(PATHS):
        source = (ROOT / path).read_text(encoding='utf-8-sig')
        for plan in reversed(PLANS):
            if path not in plan['files']:
                continue
            record = plan['files'][path]
            assert _sha(source) == record['after_sha256'], ('Undeclared later command source', path, plan['baseline'])
            original = '' if record.get('added') else subprocess.check_output(
                ['git', '-C', str(ROOT), 'show', plan['baseline'] + ':' + path]
            ).decode('utf-8-sig').replace('\r\n', '\n')
            assert _sha(original) == record['before_sha256']
            if record.get('added'):
                source = ''  # The complete added file was certified above.
            for row in reversed(_edits(plan)):
                if row['path'] == path:
                    assert source.count(row['new']) == 1, ('Partial later command source', path, row.get('key', row.get('owner_key')))
                    source = source.replace(row['new'], row['old'], 1)
            assert source == original, ('Unrelated later command amendment', path)
    return True


def normalize(path, source):
    if path not in PATHS:
        return source
    verify()
    for plan in reversed(PLANS):
        record = plan['files'].get(path)
        if record and record.get('added'):
            # Added files have no replacement rows. Only the exact declared
            # complete file may disappear from the historical source view.
            if _sha(source) == record['after_sha256']:
                source = ''
            continue
        for row in reversed(_edits(plan)):
            if row['path'] != path:
                continue
            count = source.count(row['new'])
            assert count <= 1, ('Ambiguous later command amendment', path, row.get('key', row.get('owner_key')))
            if count:
                source = source.replace(row['new'], row['old'], 1)
    return source
