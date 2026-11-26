"""Check the linked Wind Temple entrance and matching escort cue path."""

from pathlib import Path
import runpy


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
symbols, resolve, calls = (native[key] for key in ("symbols", "resolve", "calls"))

for caller, target in (
    ("?OnTrigger@AreaTrigger_at_wind_temple_entrance@@", "?IsInAreaTriggerRadius@Player@@"),
    ("?UpdateAI@mob_frightened_windAI@mob_frightened_wind@@", "?GetAliveCreatureListWithEntryInGrid@WorldObject@@"),
    ("?DoAction@mob_aysa_wind_temple_escortAI@mob_aysa_wind_temple_escort@@", "?SetEscortPaused@npc_escortAI@@"),
    ("?IsSummonedBy@mob_aysa_wind_temple_escortAI@mob_aysa_wind_temple_escort@@", "?GetQuestCancellationReceipt@Player@@"),
):
    addresses = {resolve(address) for name, address in symbols.items() if name.startswith(target)}
    assert addresses & calls(caller), f"Missing linked {caller} -> {target}"

print("PASS: linked Wind Temple entrance checks the actual area and releases private escorts through the native waypoint controller")
