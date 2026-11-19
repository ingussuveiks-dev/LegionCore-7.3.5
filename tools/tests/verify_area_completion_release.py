"""Verify EO323 joins the scoped SmartEvent admission in the linked MSVC PE."""
import bisect
from pathlib import Path
import runpy

from capstone import CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_REG, Cs
from capstone.x86_const import X86_REG_EAX

native = runpy.run_path(str(Path(__file__).with_name("verify_remaining_area_release.py")))["native"]
address = native["symbol"]("?IsTriggerMeets@SmartEventObject@@")
index = bisect.bisect_right(native["starts"], address - native["base"]) - 1
begin, end, _ = native["functions"][index]
assert begin <= address - native["base"] < end
offset = native["offset"](address)
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True
instructions = list(decoder.disasm(native["binary"][offset:offset + native["base"] + end - address], address))

# MSVC compiles the sparse entry switch as a chain of subtracts and zero tests.
entry = 0
accepted = []
destinations = set()
for i, instruction in enumerate(instructions[:-1]):
    operands = instruction.operands
    if instruction.mnemonic not in ("cmp", "sub") or len(operands) != 2:
        continue
    if operands[0].type != CS_OP_REG or operands[0].reg != X86_REG_EAX or operands[1].type != CS_OP_IMM:
        continue
    branch = instructions[i + 1]
    if instruction.mnemonic == "sub" and branch.mnemonic == "je":
        entry += operands[1].imm
        accepted.append(entry)
        destinations.add(branch.operands[0].imm)
    elif instruction.mnemonic == "cmp" and branch.mnemonic == "jne" and accepted:
        accepted.append(entry + operands[1].imm)
        destinations.add(branch.address + branch.size)
        break

assert accepted == [197, 323, 367, 501, 533] and len(destinations) == 1
print("PASS Release EO323 joins the existing condition admission branch before entry consumption.")
