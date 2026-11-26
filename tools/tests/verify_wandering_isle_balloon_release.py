"""Verify the linked dock interaction and scoped Shen-zin Su balloon."""

from pathlib import Path
import runpy
import struct


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
symbols, resolve, calls = (native[key] for key in ("symbols", "resolve", "calls"))
actor = "mop_air_balloonAI@mop_air_balloon@@"
dock_click = "?HandleSpellClick@Unit@@"
summon = "?SummonCreature@Map@@"

for caller, target in (
    (dock_click, "?IsWithinLOSInMap@WorldObject@@"),
    (summon, "?BindQuestObject@Player@@"),
    ("?PassengerBoarded@" + actor, "?CanTargetQuestObject@WorldObject@@"),
    ("?PassengerBoarded@" + actor, "?RemoveFromExtraLook@Player@@"),
    ("?IsSummonedBy@" + actor, "?EnterVehicle@Unit@@"),
    ("?WaypointReached@" + actor, "?GetPassenger@Vehicle@@"),
):
    targets = {resolve(address) for name, address in symbols.items() if name.startswith(target)}
    assert targets & calls(caller), f"Missing linked {caller} -> {target}"

for caller, constants in (
    (dock_click, (55918, 245079)),
    (summon, (55649, 105002, 56661, 106636, 29791)),
):
    address = native["symbol"](caller)
    index = native["bisect"].bisect_right(native["starts"], address - native["base"]) - 1
    begin, end, unwind = native["functions"][index]
    assert begin <= address - native["base"] < end, f"Missing body for {caller}"
    origin = native["root"](begin, unwind)
    for tail_begin, tail_end, tail_unwind in native["functions"][index + 1:]:
        if tail_begin != end or native["root"](tail_begin, tail_unwind) != origin:
            break
        end = tail_end
    start = native["offset"](address)
    body = native["binary"][start:start + native["base"] + end - address]
    for value in constants:
        assert struct.pack("<I", value) in body, f"Missing {value} in {caller}"

print("PASS: linked original balloon click range, private passengers, and native route credit")
