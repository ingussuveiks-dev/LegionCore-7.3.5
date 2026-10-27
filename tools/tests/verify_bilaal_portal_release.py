"""Verify registered instance1583 actually consumes Azuregos arrival and retry."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP

p = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n, ins = p['native'], p['instructions']
def values(method):
    return {o.imm for x in ins(method) for o in x.operands if o.type == CS_OP_IMM}
def references(method):
    return {x.address + x.size + o.mem.disp for x in ins(method) for o in x.operands
            if o.type == CS_OP_MEM and o.mem.base == X86_REG_RIP}
def table(address, count):
    return struct.unpack_from('<' + str(count) + 'Q', n['binary'], n['offset'](address))
def needs(method, *dependencies):
    for dependency in dependencies:
        assert n['symbol'](dependency) in n['calls'](method), (method, dependency)
factory = '?GetInstanceScript@?$GenericInstanceMapScript@Uinstance_priest_mage_nexus_art'
arrival = '?SetGuidData@instance_priest_mage_nexus_art@@'
update = '?Update@instance_priest_mage_nexus_art@@'
assert any(n['symbol'](arrival) in table(a, 140) and n['symbol'](update) in table(a, 140) for a in references(factory))
assert 104546 in values(arrival)
needs(arrival, '?Arrived@State@BilaalPortal@@')
needs(update, '?Try@State@BilaalDeath@@', '?Try@State@BilaalPortal@@')
assert 2000 in values(update)
arrived = '?Arrived@State@BilaalPortal@@'
context = '?Context@State@BilaalPortal@@'
attempt = '?Try@State@BilaalPortal@@'
needs(arrived, context, '?GetCreature@Map@@', '?InSamePhaseId@WorldObject@@QEBA_NAEBV', attempt)
assert {104546, 1, 2000} <= values(arrived)
needs(context, '?Victory@BilaalDeath@@')
assert 5 in values(context)
needs(attempt, context, '??0GameObject@@', '??$AddToMap@VGameObject@@@Map@@',
      '?SetRespawnTime@GameObject@@', '?SetSpawnedByDefault@GameObject@@')
assert {248553, 100} <= values(attempt)
body = ins(attempt)
add = n['symbol']('??$AddToMap@VGameObject@@@Map@@')
site = next(i for i, x in enumerate(body) if x.mnemonic == 'call' and x.operands[0].type == CS_OP_IMM and n['resolve'](x.operands[0].imm) == add)
check = next(i for i in range(site + 1, site + 5) if body[i].mnemonic == 'test' and body[i].op_str == 'al, al')
assert body[check + 1].mnemonic in ('je', 'jne'), 'Map insertion result is unchecked'
# Resolve actual constructed GameObject virtual hooks; phase must precede
# insertion and rollback cleanup must remain reachable after a failed add.
mapping = (n['args'].release / 'worldserver.map').read_text(errors='replace')
match = re.search(re.escape('??_7GameObject@@6BWorldObject@@@') + r'\s+([0-9a-fA-F]{16})', mapping)
assert match
address = int(match[1], 16)
assert address in references('??0GameObject@@')
vtable = table(address, 60)
phase = vtable.index(n['symbol']('?SetPhaseId@WorldObject@@')) * 8
cleanup = vtable.index(n['symbol']('?CleanupsBeforeDelete@GameObject@@')) * 8
create = vtable.index(n['symbol']('?Create@GameObject@@')) * 8
def virtual_call(code, slot):
    for i, x in enumerate(code):
        if x.mnemonic != 'call':
            continue
        operand = x.operands[0]
        if operand.type == CS_OP_MEM and operand.mem.disp == slot:
            return True
        if operand.type == CS_OP_REG:
            writes = [q for q in code[:i] if operand.reg in q.regs_access()[1]]
            if writes and writes[-1].mnemonic == 'mov' and writes[-1].operands[1].type == CS_OP_MEM and writes[-1].operands[1].mem.disp == slot:
                return True
    return False
assert virtual_call(body[:site], create)
assert any(x.mnemonic == 'call' and x.operands[0].type == CS_OP_MEM and x.operands[0].mem.disp == phase for x in body[:site])
assert any(x.mnemonic == 'call' and x.operands[0].type == CS_OP_MEM and x.operands[0].mem.disp == cleanup for x in body[check + 2:])
# Native SmartAI waypoint callback and SET_INST_DATA64 use the selected virtual
# slot, rather than an unreferenced compiled helper.
needs('?MovementInform@SmartAI@@', '?ProcessEventsFor@SmartScript@@')
assert 40 in values('?MovementInform@SmartAI@@')  # inlined MovepointReached
selected = next(table(a, 140) for a in references(factory) if n['symbol'](arrival) in table(a, 140))
slot = selected.index(n['symbol'](arrival)) * 8
assert virtual_call(ins('?ProcessAction@SmartScript@@'), slot)
print('PASS registered instance GUID/Update hooks, actual SmartAI dispatch, earned native context, captured-phase-before-insert GameObject factory, checked insertion/cleanup and2s retry. SQL graph and client portal traversal are separate checks.')
