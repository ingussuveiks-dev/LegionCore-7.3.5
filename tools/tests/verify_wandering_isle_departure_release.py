"""Verify the linked Pandaren farewell gossip and capital choice."""

from pathlib import Path
import runpy


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
symbols, resolve, calls = (native[key] for key in ("symbols", "resolve", "calls"))
choice = "?HandleNeutralPlayerSelectFaction@WorldSession@@"

for caller, target in (
    ("?sGossipSelect@npc_shang_xi_choose_factionAI@", "?ShowNeutralPlayerFactionSelectUI@Player@@"),
    (choice, "?GetNPCIfCanInteractWith@Player@@"),
    (choice, "?TeleportTo@Player@@"),
    (choice, "?KilledMonsterCredit@Player@@"),
    (choice, "?CompleteQuest@Player@@"),
    (choice, "?SetHomebind@Player@@"),
    (choice, "?SendMovieStart@Player@@"),
    (choice, "?SaveToDB@Player@@"),
):
    targets = {resolve(address) for name, address in symbols.items() if name.startswith(target)}
    assert targets & calls(caller), f"Missing linked {caller} -> {target}"

print("PASS: linked farewell option, faction objective and capital transfer setup")
