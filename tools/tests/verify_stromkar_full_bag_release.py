"""Check the scoped loot/equip/mail consumers in the linked Release PE."""
from pathlib import Path
import bisect
import runpy
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM

native = runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True


def instructions(prefix):
    address = native['symbol'](prefix)
    index = bisect.bisect_right(native['starts'], address - native['base']) - 1
    begin, end, _ = native['functions'][index]
    assert begin <= address - native['base'] < end
    end += native['base']
    return list(decoder.disasm(native['binary'][native['offset'](address):native['offset'](end)], address))


def requires(prefix, dependencies):
    actual = native['calls'](prefix)
    for dependency in dependencies:
        assert native['symbol'](dependency) in actual, (prefix, dependency)


requires('?StoreLootItem@Player@@', ('?CanAutoEquipLoot@ScriptMgr@@', '?StoreNewItem@Player@@'))
requires('?StoreNewItem@Player@@', ('?CanEquipItem@Player@@', '?CanUnequipItem@Player@@',
    '?CanStoreItem@Player@@QEBA?AW4InventoryResult@@EEAEAV?$vector@UItemPosCount@@V?$allocator@UItemPosCount@@@std@@@std@@IIPEAVItem@@_NPEAI2@Z', '?StoreItem@Player@@', '?EquipItem@Player@@',
    # Release inlines MoveItemFromInventory and SaveInventoryAndGoldToDB.
    # Check their actual remove/quest/update-queue and persistent save sinks.
    '?RemoveItem@Player@@QEAAXEE_N@Z', '?ItemRemovedQuestCheck@Player@@',
    '?RemoveFromUpdateQueueOf@Item@@', '?DeleteFromInventoryDB@Item@@QEAAX',
    '?SendMailTo@MailDraft@@', '?_SaveInventory@Player@@', '?_SaveCurrency@Player@@'))
requires('?EffectForceEquipItem@Spell@@', ('?CanEquipItem@Player@@', '?SwapItem@Player@@'))
requires('?SwapItem@Player@@', ('?CanEquipItem@Player@@', '?AutoUnequipOffhandIfNeed@Player@@'))
policy = '?CanAutoEquipLoot@player_stromkar_loot_equipment@@'
requires(policy, ('?GetQuestStatus@Player@@', '?GetQuestRewardStatus@Player@@', '?HasItemCount@Player@@', '?GetGameObject@Map@@',
    '?GetScenario@ScenarioMgr@@', '?GetCurrentStep@Scenario@@'))
values = {operand.imm for instruction in instructions(policy)
          for operand in instruction.operands if operand.type == CS_OP_IMM}
assert {128910, 247877, 41105, 1539, 1037, 9}.issubset(values), values
requires('?AddSC_artifact_acquisition@@', ('??0player_stromkar_loot_equipment@@',))
assert b'player_stromkar_loot_equipment' in native['binary']
print('PASS linked scoped Stromkar loot policy, shared metadata/storage/equipment/mail/save consumers and native force-equip/swap/offhand path; actual DB transactions and client delivery remain separate checks')
