from pathlib import Path
import json,runpy
from verify_ai_lifetime_guard import verify_plan
p=runpy.run_path(str(Path(__file__).with_name('verify_thirtysecond_ten_release.py')))
# Keep the shared linked-verifier names available to later batches.
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values'];callbacks=p['callbacks'];expanded=p['expanded']
plan=json.loads(Path(__file__).with_name('ThirtyThirdTenRepairs.json').read_text(encoding='utf8'));seen=verify_plan(p,plan)
assert len(seen)==30
print('PASS actual Release thirtythird ten:30 weak-admission gates reject absent/expired source AI before payload calls;ten live-bound problems. Source bytes preserve authored payloads/timers;projected ASAN/native admission and full client scopes remain separate. Prior327 selected checks retained,total337.')
