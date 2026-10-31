"""Verify the actual linked quest-scope loot producers and receipt guards."""
from pathlib import Path
import runpy
from sharasdal_erunak_entry_pe import sites

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
ins = n['instructions']

def guards(body):
    return sorted(sites(n, body, '?CanLootQuestObject@Loot@@') +
                  sites(n, body, '?CanLootQuestObject@LootMgr@@'))

for name in ('CanLootQuestObject', 'BindQuestObjectScope'):
    body = ins('?' + name + '@Loot@@')
    target = '?CanLootQuestObject@LootMgr@@' if name == 'CanLootQuestObject' else '?BindQuestObjectLoot@LootMgr@@'
    assert sites(n, body, target), name

bind = ins('?BindQuestObjectScope@WorldObject@@')
assert sites(n, bind, '?BindQuestObjectLoots@LootMgr@@'), 'Late binding must seal registered source containers'
bulk = ins('?BindQuestObjectLoots@LootMgr@@')
assert sites(n, bulk, '?BindQuestObjectLoot@LootMgr@@'), 'Bulk source index must bind the actual weak scopes'
clear = ins('?clear@Loot@@')
unbind, = sites(n, clear, '?ClearQuestObjectLoot@LootMgr@@')
# This build inlines RemoveLoot's mutex/map lookup. Check the real tombstone
# store before the atomic isClear read, rather than requiring an absent call.
tombstone, = [i for i, x in enumerate(clear) if x.mnemonic == 'mov' and
              x.op_str == 'qword ptr [rdi + 0x30], r12']
is_clear, = [i for i, x in enumerate(clear) if x.mnemonic == 'movzx' and
             x.op_str == 'eax, byte ptr [rsi + 0x8f]']
assert unbind < tombstone < is_clear
assert clear[tombstone - 2].op_str == 'rdi, qword ptr [r15]'
policy = ins('?CanLootQuestObject@LootMgr@@')
assert sites(n, policy, '?GetGuidValue@Object@@') and sites(n, policy, '??8ObjectGuid@@')
assert any(x.mnemonic == 'lock cmpxchg' for x in policy), 'Actual weak lock must reject expired attempts'
assert any(x.mnemonic == 'movzx' and x.op_str == 'eax, byte ptr [rsi + 0x10]' for x in policy), 'Cancelled scope guard'

fill_names = [key for key in n['symbols'] if key.startswith('?FillLoot@Loot@@') and '?$set' in key]
assert len(fill_names) == 1, fill_names
fill = ins(fill_names[0])
assert sites(n, fill, '?BindQuestObjectLoot@LootMgr@@') or sites(n, fill, '?BindQuestObjectScope@Loot@@')
assert guards(fill), 'Native generation must admit the actual loot owner'

store = ins('?StoreLootItem@Player@@')
assert guards(store)[0] < sites(n, store, '?LootItemInSlot@Loot@@')[0], 'Guard before currency/item/storage side effects'
for method in ('AllowedForPlayer', 'BuildLootResponse', 'hasItemFor'):
    assert guards(ins('?' + method + '@Loot@@')), method
send = ins('?SendLoot@Player@@')
assert len(sites(n, send, '?CanTargetQuestObject@WorldObject@@')) >= 2
assert len(sites(n, send, '?BindQuestObjectScope@Loot@@')) >= 4
assert len(guards(send)) >= 2
assert guards(ins('?GetGoldFromLoot@Player@@')), 'Actual personal money loop must preserve scope admission'

auto = ins('?HandleAutostoreLootItemOpcode@WorldSession@@')
assert len(sites(n, auto, '?CanTargetQuestObject@WorldObject@@')) >= 2
assert guards(auto)[0] < sites(n, auto, '?StoreLootItem@Player@@')[0]
release = ins('?DoLootRelease@WorldSession@@')
assert len(sites(n, release, '?CanTargetQuestObject@WorldObject@@')) >= 2
corpse = ins('?LootCorps@WorldSession@@')
assert sites(n, corpse, '?CanTargetQuestObject@WorldObject@@')[0] < sites(n, corpse, '?SetOtherLootRecipient@Creature@@')[0]
master = ins('?HandleMasterLootItem@WorldSession@@')
assert len(sites(n, master, '?CanTargetQuestObject@WorldObject@@')) >= 4
assert len(guards(master)) >= 2
assert guards(master)[0] < sites(n, master, '?StoreNewItem@Player@@')[0]
roll = ins('?HandleDoMasterLootRoll@WorldSession@@')
assert guards(roll)[0] < sites(n, roll, '?DoRollForAllMembers@Group@@')[0]
print('PASS linked PE: actual source binding/index cleanup, generation, view, direct/native packet/currency/item/money receipt, master distribution, roll-start and private release guards; paired with native library regressions, not client traversal')
