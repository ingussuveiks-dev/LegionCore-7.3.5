"""Check real x64 controller calls, removal hooks and native form dispatch."""
from pathlib import Path
import bisect
import re
import runpy
import struct

native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))


def body(prefix):
    address = native["symbol"](prefix)
    index = bisect.bisect_right(native["starts"], address - native["base"]) - 1
    begin, end, unwind = native["functions"][index]
    assert begin <= address - native["base"] < end
    origin = native["root"](begin, unwind)
    for tail_begin, tail_end, tail_unwind in native["functions"][index + 1:]:
        if tail_begin != end or native["root"](tail_begin, tail_unwind) != origin:
            break
        end = tail_end
    at = native["offset"](address)
    return address, native["binary"][at:at + native["base"] + end - address]


def bound(prefix, callback):
    address, code = body(prefix)
    handler = native["symbol"](callback)
    for i in range(len(code) - 6):
        if code[i] in (0x48, 0x4C) and code[i + 1] == 0x8D and code[i + 2] & 0xC7 == 5:
            target = address + i + 7 + struct.unpack_from("<i", code, i + 3)[0]
            try:
                if native["resolve"](target) == handler:
                    return
            except ValueError:
                pass
    raise AssertionError(f"Missing bound callback {callback}")


helper_prefix = "?UpdateLegendaryTravelForm@@"
helper, code = body(helper_prefix)
remove = native["symbol"]("?RemoveAurasDueToSpell@Unit@@")
assert remove in native["calls"](helper_prefix)
# Both missing aura and fewer-than-five stacks must branch to the same cleanup,
# not simply return. Verify its actual call target, not just an immediate ID.
guard = re.search(rb"\x48\x85\xc0\x0f\x84(.{4})\x66\x83\xb8....\x05\x0f\x82(.{4})", code, re.S)
assert guard, "Missing production aura/stack guard"
missing = guard.start() + 9 + struct.unpack("<i", guard[1])[0]
low = guard.start() + 23 + struct.unpack("<i", guard[2])[0]
assert missing == low and 0 <= missing < len(code)
assert any(code[i] == 0xE8 and native["resolve"](helper + i + 5 + struct.unpack_from("<i", code, i + 1)[0]) == remove
           for i in range(missing, min(missing + 40, len(code) - 4))), "Invalid stacks do not call removal"
for script in ("spell_item_vigilance_perch", "spell_item_sentinels_eternal_refuge"):
    tick = native["calls"](f"?OnTick@{script}@@")
    assert helper in tick and native["symbol"]("?GetUnitOwner@AuraScript@@") in tick
    assert remove in native["calls"](f"?OnRemove@{script}@@")
    bound(f"?Register@{script}@@", f"?OnTick@{script}@@")
    bound(f"?Register@{script}@@", f"?OnRemove@{script}@@")

shape_prefix = "?HandleAuraModShapeshift@AuraEffect@@"
shape, code = body(shape_prefix)
# MSVC's first switch indexes form-1 through a byte selector and an RVA table.
# The four native travel forms must use the same no-resource-change branch as
# travel form 3; absent form 8 must retain the separate diagnostic branch.
dispatch = re.search(rb"\x0f\xb6\x84\x03(.{4})\x8b\x8c\x83(.{4})\x48\x03\xcb\xff\xe1", code[:512], re.S)
assert dispatch, "Unrecognized native shapeshift dispatch layout"
selectors = native["offset"](native["base"] + struct.unpack("<I", dispatch[1])[0])
table = native["offset"](native["base"] + struct.unpack("<I", dispatch[2])[0])


def branch(form):
    index = native["binary"][selectors + form - 1]
    return struct.unpack_from("<I", native["binary"], table + 4 * index)[0]


assert all(branch(form) == branch(3) for form in (37, 38, 39, 40)), "Travel form still reaches unknown-form/resource branch"
assert branch(8) != branch(3), "Unknown-form diagnostic was disabled"
assert native["symbol"]("?GetModelForForm@Unit@@") in native["calls"](shape_prefix)
assert native["symbol"]("?RestoreDisplayId@Unit@@") in native["calls"](shape_prefix)
print("PASS: actual Release controllers remove orphaned forms, bind unequip cleanup, and admit all four native travel forms without changing resources or disabling unknown-form diagnostics")
