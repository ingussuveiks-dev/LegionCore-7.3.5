from pathlib import Path
import json,runpy
from verify_ai_lifetime_guard import verify_plan
p=runpy.run_path(str(Path(__file__).with_name('verify_thirtyninth_ten_release.py')))
loader=p
while 'callbacks'not in loader:loader=loader['p']
p=loader|p
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values'];callbacks=p['callbacks'];expanded=p['expanded']
plan=json.loads(Path(__file__).with_name('FortiethTenRepairs.json').read_text(encoding='utf8'));seen=verify_plan(p,plan)
assert len(seen)==15
print('PASS linked Fortieth ten:15 real production closure lifetime gates before payload;complete prior Meljarak/Twin/health/367 chain retained. Native/source/ASAN projections and client/full encounters remain distinct.')
