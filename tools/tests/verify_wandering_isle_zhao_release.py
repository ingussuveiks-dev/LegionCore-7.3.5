"""Check the linked Zhao-Ren damage, launcher, and landing paths."""

from pathlib import Path
import runpy
import struct


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
symbols, resolve, calls = (native[key] for key in ("symbols", "resolve", "calls"))

for caller, target in (
    ("?DealDamage@Unit@@", "?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@"),
    ("?SpellHit@boss_zhao_renAI@boss_zhao_ren@@", "?MoveFall@MotionMaster@@"),
    ("?MovementInform@boss_zhao_renAI@boss_zhao_ren@@", "?ComputeProgress@MoveSpline@Movement@@"),
    ("?OnSpellClick@npc_rocket_launcherAI@npc_rocket_launcher@@", "?IsWithinLOSInMap@WorldObject@@"),
    ("?OnSpellClick@npc_rocket_launcherAI@npc_rocket_launcher@@", "?CastSpell@Unit@@"),
):
    targets = {resolve(address) for name, address in symbols.items() if name.startswith(target)}
    assert targets & calls(caller), f"Missing linked {caller} -> {target}"

deal_damage = native["symbol"]("?DealDamage@Unit@@")
start = native["offset"](deal_damage)
code = native["binary"][start:start + 768]
for value in (55786, 245042, 29786, 104855, 125992):
    assert struct.pack("<I", value) in code, f"Missing Zhao-Ren damage gate constant {value}"

print("PASS: linked Zhao-Ren damage admission, launcher, and native fall acknowledgement")
