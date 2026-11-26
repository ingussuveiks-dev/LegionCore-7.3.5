"""Check that the linked yak waits for and records actual carriage arrival."""

from pathlib import Path
import runpy


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
symbols, resolve, calls = (native[key] for key in ("symbols", "resolve", "calls"))

actor = "@npc_nourished_yakAI@npc_nourished_yak@@"
update = "?UpdateAI" + actor
arrival = "?RecordArrival" + actor

for caller, target in (
    (update, "?GetPointMoveList@SystemMgr@@"),
    (update, "?GetQuestCancellationReceipt@Player@@"),
    (update, arrival),
    (arrival, "?SetQuestObjectiveData@Player@@"),
    (arrival, "?SendQuestUpdateAddCredit@Player@@"),
):
    addresses = {resolve(address) for name, address in symbols.items() if name.startswith(target)}
    assert addresses & calls(caller), f"Missing linked {caller} -> {target}"

print("PASS: linked cart checks its native route endpoint and records the optional objective for the boarded attempt")
