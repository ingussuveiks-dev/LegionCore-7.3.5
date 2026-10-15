"""Check native display existence; do not invent metadata for removed displays."""
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
repo, output = map(Path, sys.argv[1:])
sys.path.insert(0, str(repo / 'tools/research'))
from audit_spell_mechanics import Data

# Native base data plus the relevant installed hotfix rows, if any.
data = Data(repo / 'build-extractors/bin/Release/dbc/enUS', output)
displays = data.rows('CreatureDisplayInfo')
assert 26101 not in displays, 'Re-evaluate the scoped obsolete-model repair: display 26101 now exists'
models = {int(line.split()[0]) for line in (output / 'models.tsv').read_text().splitlines()}
assert models == {25517, 33997}
for id in models:
    assert id in displays and displays[id]['Gender'] == 1
(output / 'native.json').write_text(json.dumps({
    'invalid_display': 26101,
    'invalid_display_present': 26101 in displays,
    'valid_displays': {id: displays[id] for id in sorted(models)},
    'db2_sha256': data.hashes,
}, indent=2))
print('PASS: installed 7.3.5 CreatureDisplayInfo contains both template displays and no display 26101.')
