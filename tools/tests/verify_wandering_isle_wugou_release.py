"""Check the linked Shu menu, movement, splash acknowledgement and credit path."""

from pathlib import Path
import runpy


native = runpy.run_path(str(Path(__file__).with_name("verify_scenario_lfg_release.py")))
symbols, resolve, calls = (native[key] for key in ("symbols", "resolve", "calls"))

actor = "@npc_water_spirit_dailoAI@npc_water_spirit_dailo@@"
select = "?sGossipSelect" + actor
splash_hit = "?SpellHitTarget" + actor
update = "?UpdateAI" + actor


def linked_calls(caller, target):
    matches = {resolve(address) for name, address in symbols.items() if name.startswith(target)}
    return bool(matches & calls(caller))


for caller, target in (
    (select, "?GetQuestCancellationReceipt@Player@@"),
    (select, "?KilledMonsterCredit@Player@@"),
    (splash_hit, "?ScheduleEvent@EventMap@@"),
    (update, "?MovePoint@MotionMaster@@"),
    (update, "?CastSpell@Unit@@"),
):
    assert linked_calls(caller, target), f"Missing linked {caller} -> {target}"

assert not linked_calls(update, "?KilledMonsterCredit@Player@@"), "Legacy direct Wugou credit is still linked"
print("PASS: the linked Shu route admits the quest attempt, schedules after the spell hit and uses native spells for Wugou")
