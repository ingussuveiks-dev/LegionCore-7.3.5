"""Verify the actual canonical per-item condition and inventory receipt consumers."""
from pathlib import Path
import runpy

native = runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
for consumer, dependency in (
    ('?AllowedForPlayer@LootItem@@', '?IsObjectMeetToConditions@ConditionMgr@@QEAA_NPEAVWorldObject@@AEBV'),
    ('?StoreLootItem@Player@@', '?LootItemInSlot@Loot@@'),
    ('?StoreLootItem@Player@@', '?CanStoreItem@Player@@QEBA?AW4InventoryResult@@EEAEAV?$vector@UItemPosCount@@V?$allocator@UItemPosCount@@@std@@@std@@II'),
    ('?StoreLootItem@Player@@', '?StoreNewItem@Player@@'),
):
    assert native['symbol'](dependency) in native['calls'](consumer), f'Missing real loot consumer {consumer} -> {dependency}'
print('PASS: deployed real loot checks native per-item conditions, rechecks the slot and validates storage before StoreNewItem; this is not client acquisition/equip proof')
