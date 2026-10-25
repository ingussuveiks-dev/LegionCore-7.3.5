"""Verify native completed-loot claim, without script-side weapon creation."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

admission = runpy.run_path(str(Path(__file__).with_name('verify_truthguard_pickup_admission_release.py')))
native = admission['native']
instructions = admission['instructions']
callback = '?OnLootStateChanged@go_sr_truthguard@@'
gossip = '?OnGossipHello@go_sr_truthguard@@'
body = instructions(callback)
state = next(index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.op_str == 'r8d, 3')
assert body[state + 1].mnemonic == 'jne', 'Native activation is still treated as a completed pickup'
targets = native['calls'](callback)
assert native['symbol']('?AddItem@Player@@') not in targets, 'Weapon still granted outside native loot'
for dependency in ('?getLootState@GameObject@@', '?HasItemCount@Player@@'):
    assert native['symbol'](dependency) in targets, f'Completed claim lost live state/ownership: {dependency}'
calls = [(index, native['resolve'](entry.operands[0].imm)) for index, entry in enumerate(body)
         if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM]
owned = next(index for index, target in calls if target == native['symbol']('?HasItemCount@Player@@'))
credit = next(index for index, target in calls if target == native['symbol']('?DoUpdateAchievementCriteria@InstanceScript@@'))
assert owned < credit and any(entry.mnemonic == 'je' for entry in body[owned + 1:credit]), 'Progression precedes ownership validation'
assert native['symbol']('?SetLootState@GameObject@@') in native['calls'](gossip), 'Owned-copy replay lost native state dispatcher'
assert native['symbol']('?OnGameObjectLootStateChanged@ScriptMgr@@') in native['calls']('?SetLootState@GameObject@@')
store = '?StoreLootItem@Player@@'
store_targets = native['calls'](store)
for dependency in ('?LootItemInSlot@Loot@@', '?StoreNewItem@Player@@', '?SendEquipError@Player@@'):
    assert native['symbol'](dependency) in store_targets, f'Native real inventory path missing: {dependency}'
inventory_checks = {native['resolve'](address) for name, address in native['symbols'].items()
                    if name.startswith('?CanStoreItem@Player@@')} & store_targets
assert len(inventory_checks) == 1, 'Native item-space admission missing or ambiguous'
assert native['symbol']('?IsObjectMeetToConditions@ConditionMgr@@QEAA_NPEAVWorldObject@@AEBV') in native['calls']('?AllowedForPlayer@LootItem@@')
assert native['symbol']('?SetLootState@GameObject@@') in native['calls']('?DoLootRelease@WorldSession@@'), 'Real loot release no longer dispatches completed state'
print('PASS: installed Truthguard completed-state/carried-item claim, native slot/condition/storage/error/release dispatch retained with no AddItem grant. Native paired storage/equip, event acknowledgment and client remain separate.')
