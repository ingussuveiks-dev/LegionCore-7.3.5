"""Verify the actual flagged-spawn admission block before native creation."""
from pathlib import Path
import runpy

from sharasdal_erunak_entry_pe import sites

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
body = n['instructions']('?LoadCreatureFromDB@Creature@@')
scenario, = sites(n, body, '?GetScenario@ScenarioMgr@@')
challenge, = sites(n, body, '?GetChallenge@Scenario@@')
affix, = sites(n, body, '?HasAffix@Challenge@@')
create, = sites(n, body, '?Create@Creature@@')
difficulty, = sites(n, body, '?GetDifficultyID@Map@@')
assert difficulty < scenario < challenge < affix < create

# CreatureData's native packed marker branches around this entire block.
gate = body[difficulty - 3:difficulty]
assert gate[0].mnemonic == 'cmp' and gate[0].op_str == 'byte ptr [rax + 0x65], 0'
assert gate[1].mnemonic == 'je'
ordinary = gate[1].operands[0].imm
assert body[difficulty + 1].mnemonic == 'cmp' and body[difficulty + 1].op_str == 'al, 8'
deny = body[difficulty + 2].operands[0].imm
assert body[difficulty + 2].mnemonic == 'jne'

# Missing scenario, missing challenge and false HasAffix all take that same
# native false return. Instance-zero/GUID branching occurs only afterwards.
for call, register in ((scenario, 'rax'), (challenge, 'rax'), (affix, 'al')):
    test, jump = body[call + 1:call + 3]
    assert test.mnemonic == 'test' and test.op_str == f'{register}, {register}'
    assert jump.mnemonic == 'je' and jump.operands[0].imm == deny
assert body[affix - 2].mnemonic == 'mov' and body[affix - 2].op_str == 'edx, 5'
false = next(x for x in body if x.address == deny)
assert false.mnemonic == 'xor' and false.op_str == 'al, al'
after = next(i for i, x in enumerate(body) if x.address == ordinary)
assert affix < after < create
assert sites(n, body[after:create], '?GetInstanceId@Map@@')
assert sites(n, body[after:create], '?GetCreature@Map@@')
print('PASS linked Teeming admission: packed spawn marker,exact Keystone8,real scenario/challenge/affix5,false-return guards precede both GUID paths and native Create')
