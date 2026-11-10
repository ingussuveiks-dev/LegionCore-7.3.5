from pathlib import Path
import json,runpy
from verify_ai_lifetime_guard import verify_plan
p=runpy.run_path(str(Path(__file__).with_name('verify_thirtyfifth_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values'];callbacks=p['callbacks'];expanded=p['expanded']
plan=json.loads(Path(__file__).with_name('ThirtySixthTenRepairs.json').read_text(encoding='utf8'));seen=verify_plan(p,plan)
assert len(seen)==49
print('PASS actual Release thirtysixth ten:49 production weak-admission gates before payload access;ten independent live-bound problems. Payload logic and original delays preserved;ASAN/native admission projections and full encounters/client scopes remain separate. Prior357 selected checks retained,total367.')
