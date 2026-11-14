"""Positive and refusal controls; no production or historical hash mutation."""
import hashlib
import subprocess
from artifact_later_command_scope import ROOT, PATHS, PLANS, normalize, verify

historical = [ROOT / 'tools/tests/WarswordsSceneExpiryRepair.json', ROOT / 'tools/tests/FiveTen20261113Repairs.json']
before = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in historical}
verify()
cases = 0
for path in sorted(PATHS):
    current = (ROOT / path).read_text(encoding='utf-8-sig')
    added = next((record.get('added') for plan in PLANS for name, record in plan['files'].items() if name == path), False)
    pinned = '' if added else subprocess.check_output(
        ['git', '-C', str(ROOT), 'show', PLANS[0]['baseline'] + ':' + path]
    ).decode('utf-8-sig').replace('\r\n', '\n')
    assert normalize(path, current) == pinned, path
    assert normalize(path, pinned) == pinned, path
    assert normalize(path, current + '\n// undeclared source addition\n') != pinned
    cases += 3
    # Exercise one latest complete-function edit per touched file. A damaged
    # approved body must remain detectable by the historical full-file check.
    row = next((row for plan in reversed(PLANS) for row in reversed(plan['changes'] + plan['extra_edits']) if row['path'] == path and row['new'] in current), None)
    if row is None and added:
        row = {'new': current}
    assert row, path
    damaged = row['new'] + '\n// foreign amendment inside replacement'
    mutation = current.replace(row['new'], damaged, 1)
    assert normalize(path, mutation) != pinned
    cases += 1
    try:
        assert normalize(path, current + '\n' + row['new']) != pinned
    except AssertionError as error:
        assert isinstance(error.args[0], tuple), error
    cases += 1
assert before == {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in historical}
print(f'PASS {cases} chronological command/artifact scope controls; original hashes and production bytes unchanged')
