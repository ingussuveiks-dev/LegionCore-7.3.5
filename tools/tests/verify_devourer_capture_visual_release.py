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





update='?UpdateAI@npc_akaari_shadowgoreAI@npc_akaari_shadowgore@@'
lookup=native['symbol']('?GetCreature@ObjectAccessor@@')
candidates=[name for name in native['symbols'] if name.startswith('??R<lambda_') and 'npc_akaari_shadowgoreAI@npc_akaari_shadowgore@@' in name and lookup in native['calls'](name)]
assert len(candidates)==1, 'Expected one actual visual callback with fresh creature lookup'
visual=candidates[0];invokers=[name for name in native['symbols'] if name.startswith('?_Do_call@') and 'npc_akaari_shadowgoreAI@npc_akaari_shadowgore@@' in name and native['symbol'](visual) in native['calls'](name)]
assert len(invokers)==1, 'Native std::function must dispatch the verified lambda operator'
invoker=invokers[0];body=instructions(visual);targets=native['calls'](visual)
values={op.imm for ins in body for op in ins.operands if op.type==CS_OP_IMM}
assert {105536,105540,209264,3} <= values
assert native['symbol']('?SendChat@CreatureTextMgr@@') in targets
assert native['symbol']('?Talk@CreatureAI@@QEAAXE') not in targets, 'Delayed visual still depends on borrowed content AI'
for prefix in ('?TeleportTo@Player@@','?UpdateAchievementCriteria@Player@@'):
 matches={native['resolve'](address) for name,address in native['symbols'].items() if name.startswith(prefix)}
 assert not matches&targets, 'Optional visual must not advance capture'
tables=[]
for ins in instructions(update):
 for op in ins.operands:
  if ins.mnemonic=='lea' and op.type==CS_OP_MEM and op.mem.base==X86_REG_RIP:
   at=native['offset'](ins.address+ins.size+op.mem.disp)
   if struct.unpack_from('<Q',native['binary'],at+0x10)[0]==native['symbol'](invoker):tables.append(at)
assert len(tables)==1, 'Actual actor update does not install the verified std::function callback'
selected=[]
for ins in instructions('?GetAI@npc_akaari_shadowgore@@'):
 for op in ins.operands:
  if ins.mnemonic=='lea' and op.type==CS_OP_MEM and op.mem.base==X86_REG_RIP:
   at=native['offset'](ins.address+ins.size+op.mem.disp)
   if struct.unpack_from('<Q',native['binary'],at+0x18)[0]==native['symbol'](update):selected.append(at)
assert len(selected)==1, 'Factory must actually select the update that schedules this callback'

assert 5000 in {op.imm for ins in instructions(update) for op in ins.operands if op.type==CS_OP_IMM}
site=next(i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and native['resolve'](x.operands[0].imm)==lookup)
args=body[site-4:site]
assert [x.mnemonic for x in args]==['movups','movaps','lea','mov']
assert args[0].operands[1].type==CS_OP_MEM and args[0].operands[1].size==16
assert args[0].operands[1].mem.disp==8 and args[3].operands[1].type==CS_OP_MEM and args[3].operands[1].mem.disp==0
assert args[0].operands[1].mem.base==args[3].operands[1].mem.base
assert args[0].operands[0].reg==args[1].operands[1].reg and args[3].op_str.startswith('rcx,')
assert args[1].operands[0].type==CS_OP_MEM and args[2].operands[1].type==CS_OP_MEM
assert args[1].operands[0].mem.base==args[2].operands[1].mem.base and args[1].operands[0].mem.disp==args[2].operands[1].mem.disp
assert args[2].op_str.startswith('rdx,'), 'Fresh lookup must receive captured full128-bit GUID, not a helper pointer'
print('PASS registered opening Akaari update installs actual5s visual invoker: native fresh creature lookup, parent/helper identity constants, direct native Talk3 equivalent,209264 and no capture teleport/credit or borrowed AI call. Native ASAN host covers freed helper/AI/registry/fullGUID/phase/parent queue.10s transfer remains OPEN.')
