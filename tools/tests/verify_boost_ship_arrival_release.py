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
    ("?SendToSelf@VisibleNotifier@Trinity@@", "?GetTransport@Map@@"),
    # MSVC inlines the placement helper into the wrapper; verify its actual
    # native collision call there rather than requiring a non-inlined helper.
    ("?SummonPassenger" + script, "?getHeight@GameObjectModel@@"),
    # PreparePassengerPosition can be fully inlined/eliminated. The selected
    # SummonPassenger wrapper above must still call the actual collision query.
]
for caller, target in checks:
    assert native["symbol"](target) in native["calls"](caller), f"Missing linked {caller} -> {target}"
print("PASS: linked Release queries moving collision models, prepares initial and NPC deck poses, defers NPCs and reconciles transport visibility")
