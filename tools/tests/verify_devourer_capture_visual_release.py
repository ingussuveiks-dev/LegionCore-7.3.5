"""Check the selected opening Akaari capture state machine in the linked Release PE."""
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





update='?UpdateAI@npc_akaari_shadowgoreAI@npc_akaari_shadowgore@@'
body=instructions(update)
targets=native['calls'](update)
# Helpers may be outlined or inlined; inspect the code actually reached by this Update.
reached=[update]
for prefix in ('?CanCapture@npc_akaari_shadowgoreAI@npc_akaari_shadowgore@@',
               '?PresentCapture@npc_akaari_shadowgoreAI@npc_akaari_shadowgore@@',
               '?FinishCapture@npc_akaari_shadowgoreAI@npc_akaari_shadowgore@@'):
 matches=[name for name in native['symbols'] if name.startswith(prefix)]
 if matches:
  assert len(matches)==1 and native['symbol'](matches[0]) in targets, f'Update does not use {prefix}'
  reached.extend(matches)
can_prefix='?CanCapture@npc_akaari_shadowgoreAI@npc_akaari_shadowgore@@'
finish_prefix='?FinishCapture@npc_akaari_shadowgoreAI@npc_akaari_shadowgore@@'
can=native['symbol'](can_prefix);finish=native['symbol'](finish_prefix)
assert can in native['calls'](finish_prefix), 'Final transfer must revalidate after optional presentation/reentrant callbacks'
finish_body=instructions(finish_prefix)
ordered=[native['resolve'](ins.operands[0].imm) for ins in finish_body if ins.mnemonic=='call' and ins.operands[0].type==CS_OP_IMM]
assert ordered.index(can)<ordered.index(native['symbol']('?TeleportTo@Player@@QEAA_NIMMMMII@Z')), 'Admission must precede native teleport'
reached.extend(['?EnterCombat@npc_akaari_shadowgoreAI@npc_akaari_shadowgore@@UEAAXPEAVUnit@@@Z','?Reset@npc_akaari_shadowgoreAI@npc_akaari_shadowgore@@UEAAXXZ'])
all_body=[ins for name in reached for ins in instructions(name)]
all_targets=set().union(*(native['calls'](name) for name in reached))
values={op.imm for ins in all_body for op in ins.operands if op.type==CS_OP_IMM}
assert {105540,209264,209238,5000,10000,12000,11000,41924,1078,211686,211684,50261,1607} <= values, values
for prefix in ('?GetPlayer@ObjectAccessor@@','?GetCreature@ObjectAccessor@@','?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@',
               '?TeleportTo@Player@@','?AddQuestDelayedEvent@Player@@','?getQuestStatus@Player@@','?UpdateAchievementCriteria@Player@@','?ScheduleEvent@EventMap@@','?Reset@EventMap@@'):
 matches={native['resolve'](address) for name,address in native['symbols'].items() if name.startswith(prefix)}
 assert matches & all_targets, f'Missing native capture operation {prefix}'
for prefix in ('?AddDelayedEvent@Unit@@','?AddDelayedEvent@FunctionProcessor@@','?FindNearestPlayer@WorldObject@@'):
 matches={native['resolve'](address) for name,address in native['symbols'].items() if name.startswith(prefix)}
 assert not matches & all_targets, f'Capture still borrows delayed callbacks or nearest stranger: {prefix}'
selected=[]
for ins in instructions('?GetAI@npc_akaari_shadowgore@@'):
 for op in ins.operands:
  if ins.mnemonic=='lea' and op.type==CS_OP_MEM and op.mem.base==X86_REG_RIP:
   at=native['offset'](ins.address+ins.size+op.mem.disp)
   if struct.unpack_from('<Q',native['binary'],at+0x18)[0]==native['symbol'](update):selected.append(at)
assert len(selected)==1, 'Factory does not select verified capture update'
slots=struct.unpack_from('<80Q',native['binary'],selected[0])
for name in reached[-2:]:assert native['symbol'](name) in slots, f'Factory vtable omits {name}'
print('PASS selected opening Akaari Update18 native event-driven capture: actual5s presentation/10s transfer and12s+11s combat, real map/quest/scenario/owner/registry calls, native teleport/credit, no delayed player/helper/AI callback or nearest-stranger selection. Full extracted/native host owns cancellation/reset/failure/identity tests; actual client arrival/VFX pending.')
