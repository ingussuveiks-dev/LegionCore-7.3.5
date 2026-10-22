"""Verify construct lesson lifecycle in the actual linked x64 executable."""
from pathlib import Path
import bisect
import runpy
import struct

native=runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
script="@instance_boost_experience_InstanceScript@instance_boost_experience@@"
def require_call(caller,target):
    targets={native["resolve"](address) for name,address in native["symbols"].items() if name.startswith(target)}
    assert targets.intersection(native["calls"](caller)),f"Missing linked {caller} -> {target}"
for caller,target in [
    ("?SetTrainingDummyForStep"+script,"?SetArcaneConstructsForStep"+script),
    ("?SetArcaneConstructsForStep"+script,"?SummonPassenger"+script),
    ("?SetArcaneConstructsForStep"+script,"?NeedsArcaneConstructs@BoostExperience@@"),
    ("?CreatureDies"+script,"?HandleConstructDeath"+script),
    ("?HandleConstructDeath"+script,"?UpdateAchievementCriteria@Player@@"),
]:require_call(caller,target)
# UpdateConstructLesson may be inlined; in either case Update must actually
# reach the actor creation path rather than merely contain an unused helper.
retry_symbols=[native["resolve"](address) for name,address in native["symbols"].items() if name.startswith("?UpdateConstructLesson"+script)]
if retry_symbols and retry_symbols[0] in native["calls"]("?Update"+script):
    require_call("?UpdateConstructLesson"+script,"?SetArcaneConstructsForStep"+script)
else:
    require_call("?Update"+script,"?SetArcaneConstructsForStep"+script)
for prefix,value in [("?SetArcaneConstructsForStep"+script,108478),("?HandleConstructDeath"+script,50571)]:
    address=native["symbol"](prefix)
    index=bisect.bisect_right(native["starts"],address-native["base"])-1
    begin,end,_=native["functions"][index]
    assert begin<=address-native["base"]<end
    at=native["offset"](address)
    code=native["binary"][at:at+native["base"]+end-address]
    assert struct.pack("<I",value) in code,f"Wrong linked actor/event in {prefix}"
print("PASS: linked construct actor spawn, current lesson selection, real death event and update/retry lifecycle")
