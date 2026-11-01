"""Check the actual native controller call and ordinary-summon ownership guard."""
import bisect
from pathlib import Path
import re
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM

n = runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
stamp = re.search(r'Timestamp is ([0-9a-fA-F]+)',
                  (n['args'].release / 'worldserver.map').read_text(errors='replace'))
assert stamp and int(stamp[1], 16) == struct.unpack_from('<I', n['binary'], n['pe'] + 8)[0], 'PE/map timestamps differ'
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True


def instructions(prefix):
    address = n['symbol'](prefix)
    index = bisect.bisect_right(n['starts'], address - n['base']) - 1
    begin, end, unwind = n['functions'][index]
    assert begin <= address - n['base'] < end, 'Missing native function bounds'
    origin = n['root'](begin, unwind)
    for tail_begin, tail_end, tail_unwind in n['functions'][index + 1:]:
        if tail_begin != end or n['root'](tail_begin, tail_unwind) != origin:
            break
        end = tail_end
    at = n['offset'](address)
    return list(decoder.disasm(n['binary'][at:at + n['base'] + end - address], address))


cleanup = '?RemoveAllMinionsByFilter@Unit@@'
aura = '?HandleSummonController@AuraEffect@@'
assert n['symbol'](cleanup) in n['calls'](aura), 'Native aura removal does not invoke cleanup'
assert n['symbol']('?RemoveGameObject@Unit@@QEAAXI_N') in n['calls'](aura), 'Native GO cleanup was lost'
body = instructions(cleanup)
# These are the matching x64 production layouts. A layout/codegen change must
# be reviewed with the native regression, rather than accepting an old body.
controlled = next(i for i, entry in enumerate(body) if entry.mnemonic == 'lea'
                  and any(o.type == CS_OP_MEM and o.mem.disp == 0x4C8 for o in entry.operands))
entry_lists = next(i for i, entry in enumerate(body) if entry.mnemonic == 'mov'
                  and any(o.type == CS_OP_MEM and o.mem.disp == 0x848 for o in entry.operands))
assert controlled < entry_lists, 'Controlled snapshot/ordinary entry-list traversal missing'
owner = next(i for i, entry in enumerate(body) if entry.mnemonic == 'movups'
             and any(o.type == CS_OP_MEM and o.mem.disp == 0x3B64 for o in entry.operands))
comparison = n['symbol']('??9ObjectGuid@@QEBA_NAEBV0@@Z')
compare_at = next(i for i in range(owner + 1, min(owner + 12, len(body)))
                  if body[i].mnemonic == 'call' and body[i].operands[0].type == CS_OP_IMM
                  and n['resolve'](body[i].operands[0].imm) == comparison)
branch = next(entry for entry in body[compare_at + 1:compare_at + 4] if entry.mnemonic == 'jne')
remove = next(entry for entry in body[compare_at + 1:compare_at + 9]
              if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM
              and entry.operands[0].mem.disp == 0x2B8)
assert branch.operands[0].imm > remove.address, 'Foreign-owner branch does not skip native UnSummon'
for forbidden in ('?KilledMonsterCredit@Player@@', '?QuestObjectiveSatisfy@Player@@', '?CompleteQuest@Player@@'):
    assert n['symbol'](forbidden) not in n['calls'](cleanup), 'Cleanup unexpectedly grants quest progress'
print('PASS linked native Aura428 -> cleanup: GUID snapshots, ordinary entry lists, exact summoner guard, retained GO cleanup and no quest grants')
