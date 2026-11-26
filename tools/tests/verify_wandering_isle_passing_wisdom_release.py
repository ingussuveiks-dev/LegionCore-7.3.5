"""Verify the linked Passing Wisdom summon, arrival, and credit paths."""

from pathlib import Path
import runpy
import struct


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
symbols, resolve, calls = (native[key] for key in ("symbols", "resolve", "calls"))
actor = "mob_master_shang_xi_thousand_staffAI@mob_master_shang_xi_thousand_staff@@"
summon = "?SummonCreature@Map@@"

for caller, target in (
    (summon, "?BindQuestObject@Player@@"),
    ("?MovementInform@" + actor, "?ComputeProgress@MoveSpline@Movement@@"),
    ("?UpdateAI@" + actor, "?IsWithinLOSInMap@WorldObject@@"),
    ("?UpdateAI@" + actor, "?CastSpell@Unit@@"),
):
    targets = {resolve(address) for name, address in symbols.items() if name.startswith(target)}
    assert targets & calls(caller), f"Missing linked {caller} -> {target}"

for caller, constants in (
    (summon, (56686, 106623, 3283, 29790)),
    ("?UpdateAI@" + actor, (128850, 109336, 106625)),
    ("?JustSummoned@" + actor, (57874,)),
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

print("PASS: linked scoped Passing Wisdom summon, actual arrival, and staff credit")
