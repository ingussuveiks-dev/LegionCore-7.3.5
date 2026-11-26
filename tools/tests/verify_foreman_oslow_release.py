"""Verify the linked owner selection, boulder ride and native group credit."""

from pathlib import Path
import runpy


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
symbols, resolve, calls = (native[key] for key in ("symbols", "resolve", "calls"))
boulder = "@npc_redridge_huge_boulderAI@"
ettin = "@npc_redridge_subdued_ettinAI@"

for caller, target in (
    ("?FindRescueEttin@", "?GetCreature@ObjectAccessor@@"),
    ("?FindRescueEttin@", "?GetGuidValue@Object@@"),
    ("?SpellHit" + boulder, "?FindRescueEttin@"),
    ("?ChooseAssignedEttin@spell_redridge_lift_huge_boulder@@", "?GetCreature@ObjectAccessor@@"),
    ("?PassengerBoarded" + ettin, "?ScheduleEvent@EventMap@@"),
    ("?UpdateAI" + ettin, "?GetPassenger@Vehicle@@"),
    ("?UpdateAI" + boulder, "?IsAtGroupRewardDistance@Player@@"),
    ("?UpdateAI" + boulder, "?RewardPlayerAndGroupAtEvent@Player@@"),
    ("?UpdateAI" + boulder, "?ExitVehicle@Unit@@"),
):
    targets = {resolve(address) for name, address in symbols.items() if name.startswith(target)}
    assert targets & calls(caller), f"Missing linked {caller} -> {target}"

print("PASS: linked actual Ettin ownership, assigned boulder seat, group credit and cleanup")
