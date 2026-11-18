"""Verify repaired Dargal required counters at actual native reward admission."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM
p=runpy.run_path(str(Path(__file__).with_name('verify_dargal_progression_release.py')))
n,ins=p['n'],p['ins']
base='?CanRewardQuest@Player@@QEAA_NPEBVQuest@@_N@Z'
choice='?CanRewardQuest@Player@@QEAA_NPEBVQuest@@I_N@Z'
assert n['symbol'](base) in n['calls'](choice)
values={o.imm for i in ins(base) for o in i.operands if o.type==CS_OP_IMM}
assert {42771,108815,110635}<=values
assert values & {8,9}
assert p['anycall'](base,'?GetQuestObjectiveData@Player@@')
for forbidden in ('?SetQuestObjectiveData@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?RewardQuest@Player@@','?AddItem@Player@@'):
 assert not p['anycall'](base,forbidden),forbidden
print('PASS selected native reward-choice overload calls the real base42771/108815/110635 counter gate; gate mutates/grants nothing. Actual legacy/status/inventory outcomes are extracted-native ASAN evidence, not client reward traversal.')