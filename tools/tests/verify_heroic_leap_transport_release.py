"""Select the registered Leap callback and real collision/native jump consumers."""
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
check='?CheckElevation@spell_warr_heroic_leap_SpellScript@spell_warr_heroic_leap@@'
cast='?HandleDummy@spell_warr_heroic_leap_SpellScript@spell_warr_heroic_leap@@'
geometry='?CheckTransportJumpPath@Movement@@'
require(check,geometry)
require(check,'?CalculatePath@PathGenerator@@')
require(geometry,'?getHeight@GameObjectModel@@')
require(geometry,'?isInLineOfSight@GameObjectModel@@')
require(geometry,'?computeFallElevation@Movement@@')
height_targets={n['resolve'](address) for name,address in n['symbols'].items() if name.startswith('?getHeight@GameObjectModel@@')}
geometry_body=instructions(geometry)
height_calls=[ins for ins in geometry_body if ins.mnemonic=='call' and ins.operands
              and ins.operands[0].type==CS_OP_IMM and n['resolve'](ins.operands[0].imm) in height_targets]
assert len(height_calls)==1, 'Landing must check center support without a second radius-floor loop'
constants=set()
for ins in geometry_body:
    for op in ins.operands:
        if op.type==CS_OP_MEM and op.mem.base==X86_REG_RIP:
            address=ins.address+ins.size+op.mem.disp
            constants.add(n['binary'][n['offset'](address):n['offset'](address)+4])
assert struct.pack('<f',0.1) in constants, 'Missing native getHeight half-yard origin compensation'
require(cast,'?CastSpell@Unit@@')
require('?EffectJumpDest@Spell@@','?MoveJump@MotionMaster@@')
require('?MoveJump@MotionMaster@@QEAAXMMMM','?Launch@MoveSplineInit@Movement@@')
transform='??RTransportPathTransform@Movement@@'
require(transform,'?GetDirectTransport@Unit@@')
body=instructions(transform)
assert any(ins.mnemonic == 'mov' and any(op.type == CS_OP_MEM and op.mem.disp == 0x10 for op in ins.operands)
           for ins in body) and any(ins.mnemonic == 'call' and ins.op_str == 'r10' for ins in body), 'Missing native TransportBase::CalculatePassengerOffset virtual slot'
assert b'spell_warr_heroic_leap' in n['binary']
def references(method):
    for ins in instructions(method):
        for op in ins.operands:
            if op.type == CS_OP_IMM:
                yield op.imm
            elif op.type == CS_OP_MEM and op.mem.base == X86_REG_RIP:
                yield ins.address + ins.size + op.mem.disp
register='?Register@spell_warr_heroic_leap_SpellScript@spell_warr_heroic_leap@@'
assert {n['symbol'](check),n['symbol'](cast)} <= set(references(register))
mapping=(n['args'].release/'worldserver.map').read_text(errors='replace')
table=re.search(r'\s[0-9a-f]+:[0-9a-f]+\s+'+re.escape('??_7spell_warr_heroic_leap_SpellScript@spell_warr_heroic_leap@@6B@')+r'\s+([0-9a-fA-F]{16})',mapping)
assert table
address=int(table[1],16)
assert address in references('?GetSpellScript@spell_warr_heroic_leap@@')
assert n['symbol'](register) in struct.unpack_from('<12Q',n['binary'],n['offset'](address))
print('PASS selected registered Leap admission uses real deck landing/arc collision, ordinary terrain fallback, native94954 cast and transport-local jump launch. Actual client traversal remains pending.')
