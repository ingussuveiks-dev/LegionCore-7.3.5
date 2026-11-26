"""Verify the linked Shen-zin Su defense and quest cleanup."""

from pathlib import Path
import runpy


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
symbols, resolve, calls = (native[key] for key in ("symbols", "resolve", "calls"))
controller = "npc_ji_end_eventAI@npc_ji_end_event@@"

for caller, target in (
    ("?MoveInLineOfSight@" + controller, "?GetQuestCancellationReceipt@Player@@"),
    ("?UpdateAI@" + controller, "?RemoveIneligibleParticipants@" + controller),
    ("?UpdatePower@" + controller, "?GetLivingHealers@" + controller),
    ("?UpdatePower@" + controller, "?KilledMonsterCredit@Player@@"),
    ("?Reset@" + controller, "?DespawnAll@SummonList@@"),
    ("?CancelQuestObjects@Player@@", "?RemoveAurasDueToSpell@Unit@@"),
):
    targets = {resolve(address) for name, address in symbols.items() if name.startswith(target)}
    assert targets & calls(caller), f"Missing linked {caller} -> {target}"

print("PASS: linked quest attempt, active healers, defense completion and power cleanup")
