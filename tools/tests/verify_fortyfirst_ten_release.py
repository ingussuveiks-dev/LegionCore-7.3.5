from pathlib import Path
import json,runpy
from verify_ai_lifetime_guard import verify_plan
p=runpy.run_path(str(Path(__file__).with_name('verify_fortieth_ten_release.py')))
loader=p
while 'callbacks'not in loader:loader=loader['p']
p=loader|p
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values'];callbacks=p['callbacks'];expanded=p['expanded']
plan=json.loads(Path(__file__).with_name('FortyFirstTenRepairs.json').read_text(encoding='utf8'));seen=verify_plan(p,plan)
assert len(seen)==10
print('PASS linked FortyFirst active cases:10 real production closure lifetime gates before payload;complete prior Meljarak/Twin/health/367 chain retained. Native/source/ASAN projections and client/full encounters remain distinct.')
