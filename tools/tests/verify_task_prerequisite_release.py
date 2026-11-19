"""Resolve the two task branches and native metadata evaluator in the final PE."""
from pathlib import Path
import bisect
import re
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM

n = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True


def instructions(prefix):
    address = n["symbol"](prefix)
    index = bisect.bisect_right(n["starts"], address - n["base"]) - 1
    begin, end, unwind = n["functions"][index]
    assert begin <= address - n["base"] < end
    origin = n["root"](begin, unwind)
    for tail_begin, tail_end, tail_unwind in n["functions"][index + 1:]:
        if tail_begin != end or n["root"](tail_begin, tail_unwind) != origin:
            break
        end = tail_end
    start = n["offset"](address)
    return list(decoder.disasm(n["binary"][start:start + n["base"] + end - address], address))


stamp = re.search(r"Timestamp is ([0-9a-fA-F]+)", (n["args"].release / "worldserver.map").read_text(errors="replace"))
assert stamp and int(stamp[1], 16) == struct.unpack_from("<I", n["binary"], n["pe"] + 8)[0]
helper = "?MeetsQuestTaskPrerequisites@@"
calls = [i for i in instructions("?UpdateAreaQuestTasks@Player@@")
         if i.mnemonic == "call" and i.operands[0].type == CS_OP_IMM
         and n["resolve"](i.operands[0].imm) == n["symbol"](helper)]
assert len(calls) == 2, "Both bonus and world-task branches must use native prerequisite logic"
assert n["symbol"]("?GetQuestStatus@Player@@") in n["calls"](helper)
# Packed26972 QuestV2CliTask:completed quests at34,logic byte at45.
# The native fixture prints these actual offsetof values before its cases.
assert any(i.mnemonic == "movzx" and any(o.type == CS_OP_MEM and o.size == 1 and o.mem.disp == 45 for o in i.operands)
           for i in instructions(helper)), "Linked evaluator must read the authored task logic byte"
print("PASS final PE/map:both task branches call native prerequisite evaluator;actual quest status and packed26972 logic byte45 retained. Native behavioral cases and client traversal remain separate.")
