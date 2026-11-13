"""Keep older healing certificates strict after the finite Warrior scene repair."""
from pathlib import Path
import subprocess
from warswords_scene_expiry_scope import PLAN, normalize, verify
from second_ten_ten_20261112_scope import verify as verify_retained

ROOT = Path(__file__).resolve().parents[2]
verify()
verify_retained()
cases = 0
for path in PLAN['files']:
    current = (ROOT / path).read_text(encoding='utf-8-sig')
    pinned = subprocess.check_output(
        ['git', '-C', str(ROOT), 'show', PLAN['baseline'] + ':' + path]
    ).decode('utf-8-sig').replace('\r\n', '\n')
    assert normalize(path, current) == pinned
    assert normalize(path, pinned) == pinned
    # Undeclared content must reach the old whole-file comparison unchanged.
    assert normalize(path, current + '\n// undeclared sentinel\n') != pinned
    edit = next(row for row in PLAN['changes'] if row['path'] == path)
    changed = edit['new'].replace('\n', '\n// partial amendment\n', 1)
    assert changed != edit['new']
    mutated = current.replace(edit['new'], changed, 1)
    try:
        assert normalize(path, mutated) != pinned
    except AssertionError as error:
        # Refusal of the damaged declared block is also a valid negative result.
        assert error.args and isinstance(error.args[0], tuple), error
    cases += 4
print(f'PASS {cases} finite-scene/retained healing scope controls: exact restoration, idempotence, foreign addition and partial amendment remain detectable')
