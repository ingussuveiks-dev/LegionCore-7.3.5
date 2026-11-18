"""Verify the native boss-state persistence is present in the deployed Violet AI."""
from pathlib import Path
import runpy
from sharasdal_erunak_entry_pe import sites, values

n = runpy.run_path(str(Path(__file__).with_name("verify_smart_linked_cast_retry_release.py")))
suffix = "@instance_violet_hold_legion_InstanceMapScript@instance_violet_hold_legion@@"
save = n["instructions"]("?GetSaveData" + suffix)
load = n["instructions"]("?Load" + suffix)
assert sites(n, save, "?WriteSaveDataBossStates@InstanceScript@@")
assert sites(n, load, "?SetBossState@InstanceScript@@")
assert sites(n, n["instructions"]("?IsEncounterInProgress" + suffix), "?IsEncounterInProgress@InstanceScript@@")
assert {66, 8, 5} <= values(load), "Missing B marker, eight-state loop or range guard"
for target in ("?KilledMonsterCredit@Player@@", "?UpdateEncounterState@InstanceScript@@", "?AddItem@Player@@"):
    assert not sites(n, load, target)
assert sites(n, n["instructions"]("?AddLegionScripts@@"), "?AddSC_instance_violet_hold_legion@@")
print("PASS final linked Violet Hold: registered production AI writes native boss states; validated B extension/eight-state restore uses qualified native state API without quest/item/encounter-credit dispatch. Native legacy/new-save replay is separate; client remains open.")
