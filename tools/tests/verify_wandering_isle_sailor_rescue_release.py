"""Verify the linked sailor pickup, native delivery hit, and cancellation paths."""

from pathlib import Path
import runpy


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
symbols, resolve, calls = (native[key] for key in ("symbols", "resolve", "calls"))
controller = "npc_injured_sailor_rescue_controllerAI@npc_injured_sailor_rescue_controller@@"

for caller, target in (
    ("?HandleSpellClick@Unit@@", "?IsWithinLOSInMap@WorldObject@@"),
    ("?SummonCreature@Map@@", "?BindQuestObject@Player@@"),
    ("?MoveInLineOfSight@" + controller, "?GetQuestCancellationReceipt@Player@@"),
    ("?MoveInLineOfSight@" + controller, "?CastSpell@Unit@@"),
    ("?UpdateAI@" + controller, "?KilledMonsterCredit@Player@@"),
    ("?CancelQuestObjects@Player@@", "?RemoveAurasDueToSpell@Unit@@"),
):
    targets = {resolve(address) for name, address in symbols.items() if name.startswith(target)}
    assert targets & calls(caller), f"Missing linked {caller} -> {target}"

acknowledgement = "?SpellHitTarget@" + controller
assert any(name.startswith(acknowledgement) for name in symbols), "Missing native cancellation-hit callback"
credits = {resolve(address) for name, address in symbols.items() if name.startswith("?KilledMonsterCredit@Player@@")}
assert not credits & calls(acknowledgement)

print("PASS: linked sailor pickup scope, native cancellation hit, deferred delivery credit and quest cleanup")
