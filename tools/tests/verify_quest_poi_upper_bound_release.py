"""Verify that the linked quest POI point loop includes its highest ID."""

import bisect
from pathlib import Path
import runpy

from capstone import CS_ARCH_X86, CS_MODE_64, Cs


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
address = native["symbol"]("?LoadQuestPOI@QuestDataStoreMgr@@")
rva = address - native["base"]
index = bisect.bisect_right(native["starts"], rva) - 1
begin, end, unwind = native["functions"][index]
assert begin <= rva < end
origin = native["root"](begin, unwind)
for tail_begin, tail_end, tail_unwind in native["functions"][index + 1:]:
    if tail_begin != end or native["root"](tail_begin, tail_unwind) != origin:
        break
    end = tail_end

offset = native["offset"](address)
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
body = list(decoder.disasm(native["binary"][offset:offset + end - rva], address))
guard = [
    compare for compare, jump, vector_index in zip(body, body[1:], body[2:])
    if compare.mnemonic == "cmp" and compare.op_str == "esi, r14d"
    and jump.mnemonic == "jg"
    and vector_index.mnemonic == "lea" and vector_index.op_str == "rdx, [rsi + rsi*2]"
]
assert len(guard) == 1, "The deployed POI point loop must skip only IDs strictly above its allocated maximum"
print("PASS: the actual loaded quest ID equal to the maximum reaches the point vector")
