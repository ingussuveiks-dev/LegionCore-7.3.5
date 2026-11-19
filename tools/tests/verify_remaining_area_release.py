"""Check the linked area admission, delayed cast and group credit in Release."""
from pathlib import Path
import bisect
import re
import runpy
import struct

native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
mapping = (native["args"].release / "worldserver.map").read_text(errors="replace")
stamp = re.search(r"Timestamp is ([0-9a-fA-F]+)", mapping)
assert stamp and int(stamp[1], 16) == struct.unpack_from("<I", native["binary"], native["pe"] + 8)[0]

groups = "?RewardStagedAreaCredit@"
schedule = "?ScheduleStoneDeliveryButton@"
entry = "?IsTriggerMeets@SmartEventObject@@"
callback = "?_Do_call@?$_Func_impl_no_alloc@V<lambda_1>@?1??ScheduleStoneDeliveryButton@"
condition = "?IsObjectMeetingSmartEventConditions@ConditionMgr@@"
edges = {
    "?ProcessAction@SmartScript@@": [groups, schedule],
    entry: [condition],
    groups: ["?IsAtGroupRewardDistance@Player@@", condition, "?KilledMonsterCredit@Player@@"],
    schedule: ["?AddQuestDelayedEvent@Player@@"],
    callback: [condition, "?GetEventObject@ObjectAccessor@@", "?ActivatedForPlayer@EventObject@@"],
}
for caller, targets in edges.items():
    actual = native["calls"](caller)
    for target in targets:
        assert native["symbol"](target) in actual, f"Missing Release call {caller} -> {target}"

selector = "?SelectImplicitNearbyTargets@Spell@@"
address = native["symbol"](selector)
index = bisect.bisect_right(native["starts"], address - native["base"]) - 1
begin, end, _ = native["functions"][index]
assert begin <= address - native["base"] < end
offset = native["offset"](address)
code = native["binary"][offset: offset + native["base"] + end - address]
assert all(struct.pack("<I", value) in code for value in (206157, 104208)), "Missing scoped delivery target identifiers"
assert native["symbol"]("?AddUnitTarget@Spell@@") in native["calls"](selector)

print("PASS Release links scoped SmartEvent admission, per-player group stages, quest-owned delayed delivery and the original receiver hit.")
