"""Check the meditation progress and cleanup calls in the final worldserver PE."""

from pathlib import Path
import runpy


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
calls = native["calls"]
symbol = native["symbol"]

participant = "?AdvanceParticipant@mob_aysaAI@mob_aysa@@"
cleanup = "?FinishMeditation@mob_aysaAI@mob_aysa@@"
for caller, target in (
    (participant, "?GetQuestCancellationReceipt@Player@@"),
    (participant, "?KilledMonsterCredit@Player@@QEAAXIVObjectGuid@@@Z"),
    (cleanup, "?DespawnAll@SummonList@@"),
    ("?OnBeforeLogout@player_wandering_isle_meditation@@", "?RemoveAura@Unit@@QEAAXIVObjectGuid@@I"),
):
    if symbol(target) not in calls(caller):
        raise AssertionError(f"Linked worldserver lacks {caller} -> {target}")

print("PASS: linked PE contains accepted-attempt meditation credit, wave cleanup and logout aura removal")
