"""Inspect Dyala's selector in the linked x64 PE (requires capstone)."""
import bisect
from pathlib import Path
import runpy
import sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM

reader = Path(__file__).with_name("verify_scenario_lfg_release.py")
sys.argv[0] = str(reader)
n = runpy.run_path(str(reader))
prefix = "?selectShipment@GarrShipment@@"
address = n["symbol"](prefix)
index = bisect.bisect_right(n["starts"], address - n["base"]) - 1
begin, end, unwind = n["functions"][index]
origin = n["root"](begin, unwind)
for tail_begin, tail_end, tail_unwind in n["functions"][index + 1:]:
    if tail_begin != end or n["root"](tail_begin, tail_unwind) != origin:
        break
    end = tail_end
at = n["offset"](address)
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True
instructions = list(decoder.disasm(n["binary"][at:at + n["base"] + end - address], address))
immediates = lambda mn: {operand.imm for ins in instructions if ins.mnemonic == mn for operand in ins.operands if operand.type == CS_OP_IMM}
assert {129, 41797, 9}.issubset(immediates("cmp")), "Missing container/quest/class gates"
assert {321, 300, 255, 362}.issubset(immediates("mov")), "Missing tutorial/normal/upgrade selection"
for target in ("?GetQuestStatus@Player@@", "?hasTallent@Garrison@@"):
    assert n["symbol"](target) in n["calls"](prefix), f"Missing native selector call: {target}"
for caller in ("?canAddShipmentOrder@Garrison@@", "?OnGossipSelect@Garrison@@", "?CreateShipment@Garrison@@", "?SendShipmentInfo@Garrison@@"):
    assert address in n["calls"](caller), f"Missing selector dispatch: {caller}"
print("PASS: linked Dyala selector contains native quest/class/container gates, 321/255/300 choices, talent 362 and all four service dispatches")
