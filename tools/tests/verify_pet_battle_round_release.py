"""Check the real three round senders select the typed target converter."""
from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n=p['native']
converter='?BuildPetBattleRoundEffect@@'
for caller in ('?SendPetBattleFirstRound@WorldSession@@','?SendPetBattleRoundResult@WorldSession@@','?SendPetBattleReplacementMade@WorldSession@@'):
    assert n['symbol'](converter) in n['calls'](caller), (caller,'Missing typed converter')
    assert n['symbol']('?Write@BattleRound@BattlePet@WorldPackets@@') in n['calls'](caller), (caller,'Native packet writer missing')
assert n['symbol']('?SetPetState@PetBattle@@') in n['calls']('?Kill@PetBattle@@'), 'Native death state dispatch missing'
print('PASS selected three linked round senders -> typed converter/native735 BattleRound::Write; native Kill -> SetPetState retained. Actual payload bytes tested separately; animation remains client verification.')
