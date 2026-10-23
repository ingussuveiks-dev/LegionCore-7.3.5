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
assert native['symbol']('?SetData@SmartAI@@') in native['calls']('?SetData@npc_titanstrike_prustaga@@')
assert native['symbol']('?SetData@SmartAI@@') in native['calls']('?UpdateAI@npc_titanstrike_prustaga@@')
native['symbol']('?Spawn@npc_titanstrike_prustaga@@')
print('PASS: actual Release Grif uses map1495/scenario1068/quest41574, Theodore106289 and Shoot6660; Prustaga preserves native SmartAI data dispatch')
