"""Check retained truncated capture against a complete native-file export."""
import argparse
import json
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--failed-file',type=Path,required=True)
p.add_argument('--repaired-directory',type=Path,required=True)
p.add_argument('--require-count-trailer',action='store_true')
a=p.parse_args()
old=a.failed_file.read_text(encoding='utf-8-sig').splitlines()
assert old and old[-1]=='{','Expected the retained incomplete native capture, not arbitrary bad JSON'
try:
    json.loads(old[-1])
except json.JSONDecodeError:
    pass
else:
    raise AssertionError('Original incomplete row unexpectedly parsed')
new_file=a.repaired_directory/a.failed_file.name
new=new_file.read_text(encoding='utf-8-sig').splitlines()
assert len(old)==len(new)==931
assert {tuple(sorted(json.loads(s).items())) for s in old[:-1]} <= {tuple(sorted(json.loads(s).items())) for s in new}
assert isinstance(json.loads(new[-1]),dict)
files=list(a.repaired_directory.glob('*.jsonl'))
assert len(files)==20
records=0
for file in files:
    native=a.repaired_directory/('.query-'+file.stem+'.stdout')
    raw=native.read_bytes()
    marker=b'__ARTIFACT_EXPORT_ROWS__'
    if raw.splitlines()[-1].startswith(marker):
        trailer=raw.splitlines(keepends=True)[-1]
        count=int(trailer.strip()[len(marker):])
        assert count==len(file.read_bytes().splitlines()),file.name
        assert raw[:-len(trailer)]==file.read_bytes(),file.name
    else:
        assert not a.require_count_trailer,file.name
        assert raw==file.read_bytes(),file.name
    stderr=a.repaired_directory/('.query-'+file.stem+'.stderr')
    assert stderr.read_bytes()==b'',file.name
    rows=file.read_text(encoding='utf-8-sig').splitlines()
    assert rows,file.name
    for row in rows:
        assert isinstance(json.loads(row),dict),file.name
    records+=len(rows)
print(f'PASS expected truncated-row counterfactual paired with complete931-row native query; all{len(files)} exports/{records} objects parse and match native stdout bytes with empty stderr')
