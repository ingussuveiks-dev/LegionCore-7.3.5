"""Verify the weighted draw's half-open bound in the actual linked Release method."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM
p = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n = p['native']
method = '?GetRandomBreedID@BattlePetDataStoreMgr@@'
body = p['instructions'](method)
random = n['symbol']('?urand@@')
draw = next(i for i, ins in enumerate(body) if ins.mnemonic == 'call' and ins.operands[0].type == CS_OP_IMM and n['resolve'](ins.operands[0].imm) == random)
assert any(ins.mnemonic == 'lea' and ins.op_str.startswith('edx,') and '- 1]' in ins.op_str for ins in body[max(0, draw-8):draw]), 'Missing sum-1 inclusive upper bound'
assert any(ins.mnemonic in ('je', 'jz') for ins in body[:draw]), 'Missing empty/zero-weight guard'
assert n['symbol'](method) in n['calls']('?AddBattlePetWithSpeciesId@Player@@'), 'Journal creation does not use checked weighted selector'
for caller in ('?ReplaceCreature@WildBattlePetMgr@@', '?EnableWildBattle@WildBattlePetMgr@@'):
    assert n['symbol'](method) in n['calls'](caller), (caller, 'Wild-pet selector dispatch absent')
print('PASS linked Release weighted selector uses guarded sum-1 range and real journal/wild-pet callers; DB2/SQL/native distribution checks are separate.')
