"""Inspect the deployed callbacks, resolving incremental-link entry thunks."""
from pathlib import Path
import bisect
import runpy
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM

native = runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True

def immediates(prefix):
    address = native['symbol'](prefix)
    index = bisect.bisect_right(native['starts'], address-native['base'])-1
    begin, end, unwind = native['functions'][index]
    if not begin <= address-native['base'] < end:
        following = min(value for value in native['symbols'].values() if value > address)
        assert following-address <= 128, f'Unbounded leaf callback: {prefix}'
        end = following-native['base']
    else:
        origin = native['root'](begin, unwind)
        for first, last, data in native['functions'][index+1:]:
            if first != end or native['root'](first, data) != origin:
                break
            end = last
    at = native['offset'](address)
    code = native['binary'][at:at+native['base']+end-address]
    return {op.imm for ins in decoder.disasm(code, address) for op in ins.operands if op.type == CS_OP_IMM}

grif = immediates('?UpdateAI@npc_titanstrike_grif@@')
assert {1495,1068,41574,106289,6660}.issubset(grif), 'Wrong deployed Grif scenario, quest, Theodore or Shoot'
assert 194584 not in grif, 'Grif still tries a melee Crippling Slash instead of Shoot'
follow = '?Follow@npc_titanstrike_grif@@'
assert native['symbol'](follow) in native['calls']('?UpdateAI@npc_titanstrike_grif@@')
for callee in ('?SetSpeed@Unit@@', '?GetCurrentMovementGeneratorType@MotionMaster@@', '?MoveFollow@MotionMaster@@'):
    assert native['symbol'](callee) in native['calls'](follow), f'Deployed Theodore follow lacks {callee}'
assert 14 in immediates(follow), 'Deployed Theodore follow lacks FOLLOW_MOTION_TYPE guard'
assert native['symbol']('?SetData@SmartAI@@') in native['calls']('?SetData@npc_titanstrike_prustaga@@')
assert native['symbol']('?SetData@SmartAI@@') in native['calls']('?UpdateAI@npc_titanstrike_prustaga@@')
native['symbol']('?Spawn@npc_titanstrike_prustaga@@')
print('PASS: actual Release Grif uses native scenario/Theodore/Shoot, guarded follow and speed synchronization; Prustaga preserves native SmartAI data dispatch')
