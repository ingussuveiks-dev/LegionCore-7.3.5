"""Verify the linked SmartEvent admission references all of its outdoor entries."""
import bisect
from pathlib import Path
import runpy
import struct

from capstone import CS_ARCH_X86, CS_MODE_64, CS_OP_MEM, Cs
from capstone.x86_const import X86_REG_RIP

native = runpy.run_path(str(Path(__file__).with_name("verify_remaining_area_release.py")))["native"]
address = native["symbol"]("?IsTriggerMeets@SmartEventObject@@")
index = bisect.bisect_right(native["starts"], address - native["base"]) - 1
begin, end, _ = native["functions"][index]
assert begin <= address - native["base"] < end
offset = native["offset"](address)
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True
instructions = decoder.disasm(native["binary"][offset:offset + native["base"] + end - address], address)

entries = [
    197, 200, 203, 204, 205, 208, 209, 210, 211, 212, 213, 214,
    215, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227,
    229, 230, 231, 232, 233, 307, 308, 309, 323, 331, 332, 367, 501, 533,
]
table = struct.pack(f"<{len(entries)}I", *entries)
references = {
    instruction.address + instruction.size + operand.mem.disp
    for instruction in instructions
    for operand in instruction.operands
    if operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP
}
assert any(native["binary"].startswith(table, native["offset"](target)) for target in references)
print("PASS Release SmartEvent admission references the 33 world task roots and five existing quest roots.")
