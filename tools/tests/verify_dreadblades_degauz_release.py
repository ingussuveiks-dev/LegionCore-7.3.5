"""Check the linked final Vigfus death callback, owner bridge and selected vtable."""
from pathlib import Path
import bisect
import re
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP

native = runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
stamp = re.search(r'Timestamp is ([0-9a-fA-F]+)', (native['args'].release / 'worldserver.map').read_text(errors='replace'))
assert stamp and int(stamp.group(1), 16) == struct.unpack_from('<I', native['binary'], native['pe'] + 8)[0], 'Canonical PE/map link timestamps differ'
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True


def instructions(prefix):
    address = native['symbol'](prefix)
    index = bisect.bisect_right(native['starts'], address - native['base']) - 1
    begin, end, unwind = native['functions'][index]
    assert begin <= address - native['base'] < end, f'Missing runtime bounds: {prefix}'
    origin = native['root'](begin, unwind)
    for tail_begin, tail_end, tail_unwind in native['functions'][index + 1:]:
        if tail_begin != end or native['root'](tail_begin, tail_unwind) != origin:
            break
        end = tail_end
    start = native['offset'](address)
    return list(decoder.disasm(native['binary'][start:start + native['base'] + end - address], address))




method = '?DamageTakenAfterScaling@npc_first_helper_degauzAI@npc_first_helper_degauz@@'
body = instructions(method)
targets = native['calls'](method)
for prefix in ('?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@','?GetHealth@Unit@@QEBA_KPEAV1@@','?CountPctFromMaxHealth@Unit@@','?UpdateAchievementCriteria@Player@@','?StopAttack@Creature@@','?Reset@EventMap@@'):
    assert native['symbol'](prefix) in targets, f'Missing actual Degauz native dependency: {prefix}'
values = {op.imm for ins in body for op in ins.operands if op.type == CS_OP_IMM}
assert {40849,1545,1012,48449,208007,102212,102181,102180,10000,30,60,90} <= values, 'Missing stage/admission/threshold/retreat payload'
post = native['symbol'](method)
tables = []
for ins in instructions('?GetAI@npc_first_helper_degauz@@'):
    for op in ins.operands:
        if ins.mnemonic == 'lea' and op.type == CS_OP_MEM and op.mem.base == X86_REG_RIP:
            at = native['offset'](ins.address + ins.size + op.mem.disp)
            slots = struct.unpack_from('<80Q',native['binary'],at)
            if slots[0xa8//8] == post:
                tables.append(slots)
assert len(tables)==1 and tables[0][0xa0//8]==native['symbol']('?DamageTaken@UnitAI@@'), 'Wrong selected pre/post scaling vtable slots'
owner = native['symbol']('?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')
health = native['symbol']('?GetHealth@Unit@@QEBA_KPEAV1@@')
credit = native['symbol']('?UpdateAchievementCriteria@Player@@')
call_sites = lambda address: [i for i,ins in enumerate(body) if ins.mnemonic=='call' and ins.operands[0].type==CS_OP_IMM and native['resolve'](ins.operands[0].imm)==address]
owner_site, = call_sites(owner); health_site, = call_sites(health); credit_site, = call_sites(credit)
assert owner_site < health_site < credit_site
owner_reg = body[owner_site+1].op_str.split(',')[0]
assert body[owner_site+1].mnemonic=='mov' and body[owner_site+1].op_str.endswith(', rax')
assert any(ins.mnemonic=='test' and ins.op_str==f'{owner_reg}, {owner_reg}' and body[i+1].mnemonic=='je' and body[i+1].operands[0].imm>body[credit_site].address for i,ins in enumerate(body[:health_site])), 'Unresolved owner guard missing'
assert native['symbol']('?GetMaxHealth@Unit@@QEBA_KPEAV1@@') in native['calls']('?CountPctFromMaxHealth@Unit@@'), 'Threshold helper lost attacker-relative maximum health'
zero_addresses={ins.address for ins in body if ins.mnemonic=='mov' and ins.op_str=='dword ptr [r12], esi'}
assert any(ins.mnemonic=='cmp' and ins.op_str=='dword ptr [rcx + 0x60], esi' and body[i+1].mnemonic=='je' and body[i+1].operands[0].imm in zero_addresses for i,ins in enumerate(body[:owner_site])), 'Terminal damage-zero guard missing'
# Prove the shared native death dispatcher reaches the actual post-scaling slot.
runpy.run_path(str(Path(__file__).with_name('damage_scaling_release_check.py')))['check'](native, instructions)
print('PASS selected Degauz factory uses final-damage A8 and empty raw A0; target-relative health/max, owner, stage0 admission, original retreat/summons/type92 event48449 and native primary/suspended dispatch retained. Semantic/reentry cases are native-host checks; no client proof.')
