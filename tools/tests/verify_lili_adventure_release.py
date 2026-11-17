"""Inspect the actual linked scene callbacks, privacy routing and registrations."""
from pathlib import Path
import bisect
import re
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM

n = runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
stamp = re.search(r'Timestamp is ([0-9a-fA-F]+)', (n['args'].release / 'worldserver.map').read_text(errors='replace'))
assert stamp and int(stamp.group(1), 16) == struct.unpack_from('<I', n['binary'], n['pe'] + 8)[0]
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True


def body(prefix):
    address = n['symbol'](prefix)
    index = bisect.bisect_right(n['starts'], address - n['base']) - 1
    begin, end, unwind = n['functions'][index]
    assert begin <= address - n['base'] < end, prefix
    origin = n['root'](begin, unwind)
    for b, e, u in n['functions'][index + 1:]:
        if b != end or n['root'](b, u) != origin:
            break
        end = e
    at = n['offset'](address)
    return list(decoder.disasm(n['binary'][at:at + n['base'] + end - address], address))


for caller, callee in [
    ('?AddLegionScripts@@', '?AddSC_lili_adventure@@'),
    ('?sQuestAccept@AdventureAI@LiliAdventure@@', '?Start@AdventureAI@LiliAdventure@@'),
    ('?Start@AdventureAI@LiliAdventure@@', '?BindQuestObject@Player@@'),
    ('?Start@AdventureAI@LiliAdventure@@', '?SummonCreature@Map@@'),
    ('?UpdateAI@AdventureAI@LiliAdventure@@', '?EnterVehicle@Unit@@'),
    ('?UpdateAI@AdventureAI@LiliAdventure@@', '?MoveTakeoff@MotionMaster@@'),
    ('?Speak@AdventureAI@LiliAdventure@@', '?SendChat@CreatureTextMgr@@'),
    ('?Finish@AdventureAI@LiliAdventure@@', '?Release@AdventureAI@LiliAdventure@@'),
]:
    assert n['symbol'](callee) in n['calls'](caller), (caller, callee)
for prefix, required in [
    ('?Start@AdventureAI@LiliAdventure@@', {40633, 100475, 100481, 3545, 240000}),
    ('?UpdateAI@AdventureAI@LiliAdventure@@', {100972, 198964, 40000}),
    ('?SendNonChatPacket@CreatureTextMgr@@', {5}),
]:
    values = {operand.imm for instruction in body(prefix) for operand in instruction.operands if operand.type == CS_OP_IMM}
    assert required <= values, (prefix, required - values)
for registration in (b'npc_lili_adventure\0', b'npc_lili_adventure_crane\0'):
    assert registration in n['binary'], registration
print('PASS linked Li Li acceptance/private-scope/summon, actual boarding/takeoff, native dialogue/privacy and generation cleanup paths; actual client presentation still pending.')
