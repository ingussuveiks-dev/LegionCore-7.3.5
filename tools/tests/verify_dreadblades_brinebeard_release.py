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





scope='npc_lord_brinebeardAI@npc_lord_brinebeard@@'
post='?DamageTakenAfterScaling@'+scope
body=instructions(post);targets=native['calls'](post)
for prefix in ('?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@','?GetHealth@Unit@@QEBA_KPEAV1@@','?CountPctFromMaxHealth@Unit@@','?UpdateAchievementCriteria@Player@@','?StopAttack@Creature@@','?Reset@EventMap@@','?ScheduleEvent@EventMap@@QEAAXIIGG'):
 assert native['symbol'](prefix) in targets, f'Missing actual Brinebeard dependency: {prefix}'
values={op.imm for ins in body for op in ins.operands if op.type==CS_OP_IMM}
assert {40849,1545,1012,48531,202211,95464,3000,30} <= values
selected=[]
for ins in instructions('?GetAI@npc_lord_brinebeard@@'):
 for op in ins.operands:
  if ins.mnemonic=='lea' and op.type==CS_OP_MEM and op.mem.base==X86_REG_RIP:
   slots=struct.unpack_from('<80Q',native['binary'],native['offset'](ins.address+ins.size+op.mem.disp))
   if slots[0xa8//8]==native['symbol'](post):selected.append(slots)
assert len(selected)==1 and selected[0][0xa0//8]==native['symbol']('?DamageTaken@UnitAI@@')
assert selected[0][0x130//8]==native['symbol']('?JustDied@'+scope)
update='?UpdateAI@'+scope;reset='?Reset@'+scope
assert native['symbol'](update) in selected[0]
# The independent finale timer and native self-kill are in the selected update.
update_body=instructions(update);update_targets=native['calls'](update)
for prefix in ('?ExecuteEvent@EventMap@@','?Kill@Unit@@','?CreateConversation@Conversation@@'):
 matches={native['resolve'](a) for n,a in native['symbols'].items() if n.startswith(prefix)}
 assert matches & update_targets, f'Missing native timed finale: {prefix}'
assert selected[0][0x18//8]==native['symbol'](update), 'Factory misses actual UpdateAI slot18'
# EventMap::Update is inlined: the native timer advances before ExecuteEvent.
gate=next(i for i,x in enumerate(update_body) if x.mnemonic=='cmp' and x.op_str=='byte ptr [rcx + 0x39], 0')
assert update_body[gate+1].mnemonic=='je'
normal=update_body[gate+1].operands[0].imm
assert [x.op_str for x in update_body[gate+2:gate+4]]==['rcx, 0x48','dword ptr [rcx], edx']
assert update_body[gate+4].mnemonic=='call' and native['resolve'](update_body[gate+4].operands[0].imm)==native['symbol']('?ExecuteEvent@EventMap@@')
assert update_body[gate+5].op_str=='eax, 3' and update_body[gate+6].mnemonic=='jne'
kill=native['symbol']('?Kill@Unit@@')
kill_site=next(i for i,x in enumerate(update_body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and native['resolve'](x.operands[0].imm)==kill)
assert update_body[kill_site].address < normal < update_body[gate+6].operands[0].imm, 'Finale incorrectly passes through normal victim gate'
claim=next(i for i,x in enumerate(body) if x.mnemonic=='mov' and x.op_str in ('byte ptr [r15 + 0x39], 1','word ptr [r15 + 0x38], 0x100'))
credit=native['symbol']('?UpdateAchievementCriteria@Player@@')
credit_site=next(i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and native['resolve'](x.operands[0].imm)==credit)
schedule=native['symbol']('?ScheduleEvent@EventMap@@QEAAXIIGG')
schedule_site=next(i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and native['resolve'](x.operands[0].imm)==schedule)
assert claim < schedule_site < credit_site
assert any(x.op_str=='dword ptr [r13], esi' for x in body[:claim]), 'Damage not stopped before claim'
assert any(x.op_str=='r8d, 0xbb8' for x in body[claim:schedule_site]), 'Native3000ms delay changed'
# Reset is a bounded native leaf wrapper, lacking its own x64 unwind entry.
address=native['symbol'](reset);at=native['offset'](address)
leaf=list(decoder.disasm(native['binary'][at:at+32],address))
assert leaf[0].mnemonic=='cmp' and leaf[0].op_str=='byte ptr [rcx + 0x39], 0'
assert leaf[1].mnemonic=='jne' and leaf[1].operands[0].imm==leaf[4].address and leaf[4].mnemonic=='ret'
assert leaf[2].op_str=='rcx, 0x48' and leaf[3].mnemonic=='jmp' and native['resolve'](leaf[3].operands[0].imm)==native['symbol']('?Reset@EventMap@@')
assert native['symbol']('?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@') in native['calls']('?JustDied@'+scope)
assert native['symbol']('?Reset@EventMap@@') in native['calls'](reset)
print('PASS selected Brinebeard postA8/rawA0/death130/update18, attacker-relative owner/health/native stage2, zero-before-claim, unconditional3000ms event before credit, and timed self-kill before normal victim gate. Native host covers missing/rejected visual, reset/reentry/null/owned death; client presentation remains pending.')
