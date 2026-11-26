"""Verify that the linked Vanessa finale uses native quest completion."""
import bisect
from pathlib import Path
import runpy

from capstone import CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, Cs


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
address = native["symbol"]("?ProcessAction@SmartScript@@")
rva = address - native["base"]
index = bisect.bisect_right(native["starts"], rva) - 1
begin, end, _ = native["functions"][index]
assert begin <= rva < end
offset = native["offset"](address)
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True
instructions = list(decoder.disasm(native["binary"][offset:offset + end - rva], address))


def immediates(instruction):
    return {operand.imm for operand in instruction.operands if operand.type == CS_OP_IMM}


start = next((i for i, op in enumerate(instructions) if op.mnemonic == "cmp" and 4237200 in immediates(op)), None)
assert start is not None, "Missing the original Vanessa timed-list dispatch"
finale = instructions[start:start + 100]
assert {42372, 26322, 48} <= set().union(*(immediates(op) for op in finale)), "Missing the original actor, quest or finale row"
calls = [native["resolve"](op.operands[0].imm) for op in finale if op.mnemonic == "call" and op.operands[0].type == CS_OP_IMM]
expected = [native["symbol"](name) for name in (
    "?GetTargets@SmartScript@@", "?FindQuestSlot@Player@@", "?CompleteQuest@Player@@", "?SendQuestComplete@Player@@",
)]
assert all(target in calls for target in expected), "Missing actual target selection, quest log, completion or notification call"
assert calls.index(expected[0]) < calls.index(expected[1]) < calls.index(expected[2]) < calls.index(expected[3])
assert native["symbol"]("?AreaExploredOrEventHappens@Player@@") not in calls, "The finale still uses the legacy event path"
print("PASS: linked Vanessa finale selects eligible witnesses and uses native quest completion and notification")
