"""Linked final intro protection in addition to every retained progression check."""
import runpy
from pathlib import Path
from capstone import CS_OP_IMM
from warswords_attackability_scope import verify
verify()
p=runpy.run_path(str(Path(__file__).with_name('verify_warswords_helya_ack_release.py')))
n,ins=p['n'],p['ins'];scope=p['scope']
for method in ('Reset','CancelIntro','AttackStart'):
 rows=ins('?'+method+scope)
 values={o.imm for x in rows for o in x.operands if o.type==CS_OP_IMM}
 assert {98602,1511}<=values,(method,'protection must keep exact actor/map scope')
 assert 2 in values,(method,'native NON_ATTACKABLE flag missing')
 assert any(x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==n['symbol']('?'+('RemoveFlag' if method=='AttackStart' else 'SetFlag')+'@Object@@') for x in rows),method
attack=ins('?AttackStart'+scope)
remove=p['sites'](attack,'?RemoveFlag@Object@@')[0];base=p['sites'](attack,'?AttackStart@ScriptedAI@@')[0]
assert remove<base,'Release native attack protection before original scripted engagement'
print('PASS linked final intro resets/cancellation publish native NON_ATTACKABLE; exact ready engagement removes it before ScriptedAI. Positive native Helya channel and player/pet combat remain separately exercised.')
