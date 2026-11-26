"""Verify the linked forest gate publication and custom escort movement."""

from pathlib import Path
import runpy


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
symbols, resolve, calls = (native[key] for key in ("symbols", "resolve", "calls"))
movement = "?MovementInform@mob_mandori_escortAI@mob_mandori_escort@@"
gate = "?EffectSummonObject@Spell@@"

for caller, target in (
    ("?SummonCreature@Map@@", "?BindQuestObject@Player@@"),
    (movement, "?ComputeProgress@MoveSpline@Movement@@"),
    (movement, "?MovementInform@npc_escortAI@@"),
):
    targets = {resolve(address) for name, address in symbols.items() if name.startswith(target)}
    assert targets & calls(caller), f"Missing linked {caller} -> {target}"

ordered = calls(gate, ordered=True)
bind = native["symbol"]("?BindQuestObject@Player@@")
collision = native["symbol"]("?EnableQuestScopedCollision@GameObject@@")
publish = native["symbol"]("??$AddToMap@VGameObject@@@Map@@")
assert ordered.index(bind) < ordered.index(collision) < ordered.index(publish, ordered.index(collision))

print("PASS: linked personal gates bind scope and collision before publication, while scripted movement retains ordinary escort waypoints")
