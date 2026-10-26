"""Check real linked Charge admission, native deck collision and spline path."""
from pathlib import Path
import runpy
import re
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p=runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n, instructions=p['native'],p['instructions']
def require(caller,callee):
    targets={n['resolve'](address) for name,address in n['symbols'].items() if name.startswith(callee)}
    assert targets.intersection(n['calls'](caller)), f'Missing selected {caller} -> {callee}'
target='?CalculatePathToTarget@PathGenerator@@'
geometry='?BuildTransportGroundPath@Movement@@'
check='?CheckCast@spell_warr_charge_check_cast_SpellScript@spell_warr_charge_check_cast@@'
require(check,target)
require('?CheckCast@Spell@@',target)
require('?CheckCast@Spell@@','?ShortenPathUntilDist@PathGenerator@@')
require(target,geometry)
require(geometry,'?getHeight@GameObjectModel@@')
require(geometry,'?isInLineOfSight@GameObjectModel@@')
require('?EffectCharge@Spell@@','?MoveCharge@MotionMaster@@')
require('?MoveCharge@MotionMaster@@QEAAXAEBVPathGenerator@@','?MovebyPath@MoveSplineInit@Movement@@')
require('?MoveCharge@MotionMaster@@QEAAXAEBVPathGenerator@@','?Launch@MoveSplineInit@Movement@@')
assert b'spell_warr_charge_check_cast' in n['binary']
def references(method):
    for ins in instructions(method):
        for op in ins.operands:
            if op.type == CS_OP_IMM:
                yield op.imm
            elif op.type == CS_OP_MEM and op.mem.base == X86_REG_RIP:
                yield ins.address + ins.size + op.mem.disp
register='?Register@spell_warr_charge_check_cast_SpellScript@spell_warr_charge_check_cast@@'
assert n['symbol'](check) in references(register), 'Register does not select repaired cast callback'
mapping=(n['args'].release/'worldserver.map').read_text(errors='replace')
table=re.search(r'\s[0-9a-f]+:[0-9a-f]+\s+'+re.escape('??_7spell_warr_charge_check_cast_SpellScript@spell_warr_charge_check_cast@@6B@')+r'\s+([0-9a-fA-F]{16})',mapping)
assert table
address=int(table[1],16)
assert address in references('?GetSpellScript@spell_warr_charge_check_cast@@')
assert n['symbol'](register) in struct.unpack_from('<12Q',n['binary'],n['offset'](address))
print('PASS selected Charge script/core use target-aware path; actual deck height/body-LOS checks linked, contact shortening and native charge path/spline launch retained. Actual client movement remains pending.')
