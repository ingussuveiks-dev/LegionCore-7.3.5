"""Check the new arrival call paths in the actual linked PE, including thunks."""
from pathlib import Path
import runpy

native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
script = "@instance_boost_experience_InstanceScript@instance_boost_experience@@"
checks = [
    ("?BuildPlayerArrivalObjects@Map@@", "?getHeight@GameObjectModel@@"),
    ("?SendInitSelf@Map@@", "?BuildPlayerArrivalObjects@Map@@"),
    ("?OnPlayerEnter" + script, "?PutPlayerOnTransport" + script),
    ("?Update" + script, "?ShouldDeferMapObjectArrivalVisibility@Player@@"),
]
for caller, target in checks:
    assert native["symbol"](target) in native["calls"](caller), f"Missing linked {caller} -> {target}"
print("PASS: linked Release queries moving collision models, prepares the deck before self-create and defers forced NPC visibility")
