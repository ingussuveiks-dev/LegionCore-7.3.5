"""Pair a real truncated redirected result with a counted complete snapshot."""
import argparse,json
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--failed-stdout',type=Path,required=True)
p.add_argument('--repaired-directory',type=Path,required=True)
a=p.parse_args()
old=a.failed_stdout.read_bytes().splitlines()
new=(a.repaired_directory/'smart_scripts.jsonl').read_bytes().splitlines()
assert len(old)==71481 and len(new)==71485
try:json.loads(old[-1])
except (json.JSONDecodeError,UnicodeDecodeError):pass
else:raise AssertionError('Expected the retained truncated native row')
for before,after in zip(old[:-1],new):assert json.loads(before)==json.loads(after)
raw=(a.repaired_directory/'.query-smart_scripts.stdout').read_bytes().splitlines()
assert raw[-1]==b'__ARTIFACT_EXPORT_ROWS__71485' and raw[:-1]==new
print('PASS real redirected-native truncation counterfactual:71480 complete old rows preserved, partial71481 rejected; repaired71485 rows match native data and server count trailer')
