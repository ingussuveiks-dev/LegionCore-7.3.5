"""Verify the linked Risking It All summon, escort, and movie handoff."""

from pathlib import Path
import runpy


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
symbols, resolve, calls = (native[key] for key in ("symbols", "resolve", "calls"))
escort = "mob_aysa_gunship_crash_escortAI@mob_aysa_gunship_crash_escort@@"

for caller, target in (
    ("?SummonCreature@Map@@", "?BindQuestObject@Player@@"),
    ("?IsSummonedBy@" + escort, "?CastSpell@Unit@@"),
    ("?UpdateAI@" + escort, "?AttachJi@" + escort),
    ("?UpdateAI@" + escort, "?Start@npc_escortAI@@"),
    ("?UpdateAI@" + escort, "?KilledMonsterCredit@Player@@"),
    ("?UpdateAI@" + escort, "?SendMovieStart@Player@@"),
    ("?UpdateAI@" + escort, "?NearTeleportTo@Unit@@"),
):
    targets = {resolve(address) for name, address in symbols.items() if name.startswith(target)}
    assert targets & calls(caller), f"Missing linked {caller} -> {target}"

print("PASS: linked Aysa and Ji ownership, native escort, credit and movie landing")
