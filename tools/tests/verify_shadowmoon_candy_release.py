"""Verify the Shadowmoon reward and event reset in the linked worldserver."""
import bisect
from pathlib import Path
import runpy

from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True


def immediates(name):
    address = native["symbol"](name)
    index = bisect.bisect_right(native["starts"], address - native["base"]) - 1
    begin, end, _ = native["functions"][index]
    assert begin <= address - native["base"] < end
    offset = native["offset"](address)
    size = native["base"] + end - address
    return {
        operand.imm
        for instruction in decoder.disasm(native["binary"][offset:offset + size], address)
        for operand in instruction.operands
        if operand.type == CS_OP_IMM
    }


reward = "?CanRewardQuest@Player@@QEAA_NPEBVQuest@@_N@Z"
choice = "?CanRewardQuest@Player@@QEAA_NPEBVQuest@@I_N@Z"
reset = "?ResetEventSeasonalQuests@World@@"
assert 12409 in immediates(reward)
assert native["symbol"](reward) in native["calls"](choice)
assert native["symbol"]("?SatisfyQuestConditions@Player@@") in native["calls"](reward)
assert native["symbol"]("?GetQuestStatus@Player@@") in native["calls"](reward)
assert {12, 75, 20027} <= immediates(reset)
assert b"Hallow''s End occurrence\0" in native["binary"]
assert native["symbol"](reset) in native["calls"]("?ApplyNewEvent@GameEventMgr@@")
assert native["symbol"]("?_SaveSeasonalQuestStatus@Player@@") in native["calls"]("?RewardQuest@Player@@")
print("PASS linked Shadowmoon reward conditions and Hallow's End occurrence reset")
