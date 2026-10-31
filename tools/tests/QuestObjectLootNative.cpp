// Real native containers, quest scopes, packet decoders/handlers and inventory.
// The shared fixture loads World/Hotfix/Login read-only; no CharacterDB pool.
#define MASTER_LOOT_NATIVE_SUPPORT_ONLY
#include "MasterLootNative.cpp"
#include "GameObject.h"
#include <thread>

static unsigned checks;
static uint64 nextSource = 61000;

static void Expect(bool allowed, bool beforeFix, char const* label)
{
    std::cout << "CASE " << label << " fixed=" << allowed << " old=" << beforeFix << std::endl;
#ifdef QUEST_OBJECT_LOOT_BASELINE
    Require(beforeFix, label);
#else
    Require(allowed, label);
#endif
    ++checks;
}

static uint8 Seed(Loot& loot, LootPlayer& owner, WorldObject& source)
{
    loot.clear();
    Require(loot.FillLoot(116180, LootTemplates_Creature, &owner, true, false, &source), "Actual scoped native FillLoot failed");
    loot.AddItem(LootStoreItem(2589, 0, 0, 100.0f, false, 0, 0, 1, 1));
    for (uint8 i = 0; i < loot.items.size(); ++i)
        if (loot.items[i].item.ItemID == 2589) return i;
    throw std::runtime_error("Native ordinary receipt fixture missing");
}

static size_t View(Loot& loot, Player& player)
{
    WorldPackets::Loot::LootResponse packet;
    loot.BuildLootResponse(packet, &player, ALL_PERMISSION);
    Require(packet.Write()->size() > 0, "Actual native loot response serialization failed");
    return packet.Items.size() + packet.Currencies.size();
}

static void Request(LootPlayer& player, ObjectGuid guid, uint8 slot)
{
    WorldPacket wire(CMSG_LOOT_ITEM);
    wire << uint32(1) << guid << uint8(slot + 1);
    WorldPackets::Loot::AutoStoreLootItem packet(std::move(wire)); packet.Read();
    Require(packet.Loot.size() == 1 && packet.Loot[0].Object == guid && packet.Loot[0].LootListID == slot + 1,
        "Actual autostore decoder changed");
    player.GetSession()->HandleAutostoreLootItemOpcode(packet);
}

static void CheckSource(Map& map, LootPlayer& owner, LootPlayer& other)
{
    owner.Accept(); other.Accept();
    Actor source(&map, 116180, uint32(++nextSource), owner.GetPosition());
    owner.BindQuestObject(44923, &source);
    Require(map.AddToMap(static_cast<Creature*>(&source)), "Native source map admission failed");
    source.setDeathState(CORPSE);
    auto& loot = source.loot;
    auto slot = Seed(loot, owner, source);
    Require(View(loot, owner) >= 1, "Owner cannot view native scoped loot"); ++checks;
    Expect(View(loot, other) == 0, View(loot, other) >= 1, "Non-owner response admission");
    auto before = other.GetItemCount(2589, true);
    other.StoreLootItem(slot, &loot);
    Expect(other.GetItemCount(2589, true) == before && !loot.items[slot].is_looted,
        other.GetItemCount(2589, true) == before + 1, "Direct native StoreLootItem owner admission");
    slot = Seed(loot, owner, source); before = other.GetItemCount(2589, true);
    Request(other, source.GetGUID(), slot);
    Expect(other.GetItemCount(2589, true) == before && !loot.items[slot].is_looted,
        other.GetItemCount(2589, true) == before + 1, "Actual creature-GUID packet owner admission");
    slot = Seed(loot, owner, source); before = other.GetItemCount(2589, true);
    Request(other, loot.GetGUID(), slot);
    Expect(other.GetItemCount(2589, true) == before && !loot.items[slot].is_looted,
        other.GetItemCount(2589, true) == before + 1, "Actual Loot-GUID packet owner admission");
    slot = Seed(loot, owner, source); before = owner.GetItemCount(2589, true);
    Request(owner, loot.GetGUID(), slot); Request(owner, loot.GetGUID(), slot);
    Require(owner.GetItemCount(2589, true) == before + 1, "Owner native receipt missing or duplicated"); ++checks;
    // The source can leave the map while a personal/native Loot GUID survives.
    slot = Seed(loot, owner, source);
    map.RemoveFromMap(static_cast<Creature*>(&source), false);
    auto oldScope = owner.getQuestStatus(44923)->ScriptObjects;
    owner.SetQuestStatus(44923, QUEST_STATUS_NONE); owner.SetQuestSlot(0, 0);
    Require(oldScope && oldScope->Cancelled, "Actual quest removal did not cancel scope");
    before = owner.GetItemCount(2589, true);
    Request(owner, loot.GetGUID(), slot);
    Expect(owner.GetItemCount(2589, true) == before, owner.GetItemCount(2589, true) == before + 1,
        "Cancelled source-less Loot-GUID receipt");
    owner.Accept();
    bool refilled = loot.FillLoot(116180, LootTemplates_Creature, &owner, true, false, &source);
    Expect(!refilled, refilled, "Retaken quest cannot refill its old cancelled source");
}

static void CheckMoneyAndRetake(Map& map, LootPlayer& owner)
{
    owner.Accept();
    Actor source(&map, 116180, uint32(++nextSource), owner.GetPosition());
    owner.BindQuestObject(44923, &source); // Off-map source: native quest cleanup cannot delete this stack fixture.
    auto& loot = owner.personalLoot[source.GetGUID()];
    Seed(loot, owner, source); loot.gold = 17; loot.isOpen = true;
    Loot retained;
    Seed(retained, owner, source);
    auto scope = owner.getQuestStatus(44923)->ScriptObjects;
    auto money = owner.GetMoney();
    owner.SetQuestStatus(44923, QUEST_STATUS_NONE); owner.SetQuestSlot(0, 0);
    Require(scope->Cancelled, "Native abandoned scope not cancelled");
    auto received = owner.GetGoldFromLoot();
    auto remaining = owner.personalLoot.find(source.GetGUID());
    Expect(received == 0 && owner.GetMoney() == money &&
        (remaining == owner.personalLoot.end() || remaining->second.gold == 17),
        received == 17 && owner.GetMoney() == money + 17, "Cancelled native personal money receipt");
    owner.Accept();
#ifndef QUEST_OBJECT_LOOT_BASELINE
    Require(!retained.CanLootQuestObject(&owner), "Retaken quest revived the old container"); ++checks;
    Require(!retained.FillLoot(116180, LootTemplates_Creature, &owner, true, false, &source), "Cancelled native source refilled loot"); ++checks;
    scope.reset();
    Require(!retained.CanLootQuestObject(&owner), "Expired old scope admitted native loot"); ++checks;
    retained.clear();
    Require(retained.CanLootQuestObject(&owner), "Ordinary cleared container retained old scope"); ++checks;
    Require(retained.BindQuestObjectScope(&source) && !retained.CanLootQuestObject(&owner), "Expired source binding failed closed"); ++checks;
    retained.clear(); // Includes the already-isClear path after refused/empty fill.
    Require(retained.CanLootQuestObject(&owner), "Already-clear container leaked its weak registry binding"); ++checks;
#endif
    owner.personalLoot.erase(source.GetGUID());
}

struct ScopeLootGO : GameObject
{
    ~ScopeLootGO()
    {
        if (IsInWorld()) GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

static void CheckGameObject(Map& map, LootPlayer& owner, LootPlayer& other)
{
    owner.Accept(); other.Accept();
    ScopeLootGO object;
    Require(object.Create(++nextSource, 246318, &map, 1, owner.GetPosition(), G3D::Quat(0,0,0,1), 255, GO_STATE_READY),
        "Native GO branch fixture creation failed");
    owner.BindQuestObject(44923, &object);
    Require(map.AddToMap(static_cast<GameObject*>(&object)), "Native GO branch fixture map admission failed");
    auto& loot = object.loot;
    auto slot = Seed(loot, owner, object); object.SetLootState(GO_ACTIVATED, &owner);
    Require(map.GetGameObject(object.GetGUID()) == &object, "GO fixture not reachable through native map lookup");
    Require(object.IsWithinDistInMap(&other, INTERACTION_DISTANCE), "GO fixture not within native interaction range/phase");
    std::cout << "CASE GO baseline context personal=" << object.IsPersonal() << " targetSlot=" <<
        bool(loot.LootItemInSlot(slot, &other)) << " targetView=" << View(loot, other) << std::endl;
    auto before = other.GetItemCount(2589, true);
    Request(other, object.GetGUID(), slot);
    Expect(other.GetItemCount(2589, true) == before && !loot.items[slot].is_looted,
        other.GetItemCount(2589, true) == before + 1, "Actual GO-GUID packet source admission");
    Seed(loot, owner, object); object.SetLootState(GO_ACTIVATED, &owner);
    other.SendLoot(object.GetGUID(), LOOT_CORPSE);
    Expect(!other.personalLoot.count(object.GetGUID()), other.personalLoot.count(object.GetGUID()) != 0,
        "Native SendLoot rejects private GO before opening/caching");
    other.GetSession()->DoLootRelease(object.GetGUID());
    Expect(object.getLootState() == GO_ACTIVATED && !loot.items.empty(),
        object.getLootState() == GO_JUST_DEACTIVATED && loot.items.empty(), "Non-owner cannot deactivate/clear a private GO");
    map.RemoveFromMap(static_cast<GameObject*>(&object), false); object.ResetMap();
    other.personalLoot.erase(object.GetGUID()); owner.personalLoot.erase(object.GetGUID());
}

static void CheckMaster(Map& map, LootPlayer& master, LootPlayer& target)
{
    master.Accept(); target.Accept(); ProbeParty group(master, target);
    for (bool targetOwns : {false, true}) for (bool followRules : {false, true}) for (bool lootGuid : {false,true})
    {
        Actor source(&map, 116180, uint32(++nextSource), master.GetPosition());
        LootPlayer& owner = targetOwns ? target : master;
        owner.BindQuestObject(44923, &source);
        Require(map.AddToMap(static_cast<Creature*>(&source)), "Master-loot scoped source admission failed");
        auto slot = Seed(source.loot, owner, source);
        source.loot.items[slot].follow_loot_rules = followRules;
        auto before = target.GetItemCount(2589, true);
        Assign(*master.GetSession(), target.GetGUID(), lootGuid ? source.loot.GetGUID() : source.GetGUID(), uint8(slot + 1));
        Expect(target.GetItemCount(2589, true) == before && !source.loot.items[slot].is_looted,
            target.GetItemCount(2589, true) == before + 1, "Native master source and target must both own scoped loot");
    }
}

static void CheckCachedKinds(Map& map, LootPlayer& owner, LootPlayer& other)
{
    for (bool currency : {false,true})
    {
        owner.Accept(); other.Accept();
        Actor source(&map,116180,uint32(++nextSource),owner.GetPosition());
        Loot loot; loot.SetLootOwner(&owner); loot.isClear=false; loot.objGuid=source.GetGUID(); loot.objEntry=source.GetEntry();
        loot.AddItem(LootStoreItem(currency ? 0 : 2589,currency ? 1220 : 0,0,100.0f,false,0,0,1,1));
        Require(loot.items.size()==1,"Native cached branch fixture missing");
        loot.items[0].freeforall=!currency;
        (loot.*Access(FillCachesTag{}))(&owner,true); (loot.*Access(FillCachesTag{}))(&other,true);
        sLootMgr->AddLoot(&loot);
        owner.BindQuestObject(44923,&source);
        Expect(View(loot,other)==0,View(loot,other)==1,"Late scoped currency/FFA cached response admission");
        auto count=[currency](Player& p){return currency ? p.GetCurrency(1220) : p.GetItemCount(2589,true);};
        auto before=count(other); auto unlooted=loot.unlootedCount;
        other.StoreLootItem(0,&loot);
        Expect(count(other)==before && loot.unlootedCount==unlooted,count(other)==before+1,
            "Actual cached currency/FFA storage must retain source ownership");
        before=count(owner); owner.StoreLootItem(0,&loot); owner.StoreLootItem(0,&loot);
        Require(count(owner)==before+1,"Owner native currency/FFA receipt missing or duplicated"); ++checks;
    }
}

static void RollRequest(LootPlayer& player,ObjectGuid source,uint8 slot)
{
    WorldPacket wire(CMSG_DO_MASTER_LOOT_ROLL); wire<<source<<uint8(slot+1);
    WorldPackets::Loot::DoMasterLootRoll packet(std::move(wire)); packet.Read();
    player.GetSession()->HandleDoMasterLootRoll(packet);
}

static void CheckPacketEdges(Map& map,LootPlayer& owner,LootPlayer& other)
{
    owner.Accept(); other.Accept();
    Actor source(&map,116180,uint32(++nextSource),owner.GetPosition()); other.BindQuestObject(44923,&source);
    Require(map.AddToMap(static_cast<Creature*>(&source)),"Private corpse admission failed"); source.setDeathState(CORPSE);
    Seed(source.loot,other,source); source.SetOtherLootRecipient(other.GetGUID());
    source.LowerPlayerDamageReq(uint32(source.GetMaxHealth())); source.SetLootRecipient(&owner);
    source.AddToLootList(owner.GetGUID()); source.SetFlag(OBJECT_FIELD_DYNAMIC_FLAGS,UNIT_DYNFLAG_LOOTABLE); source.lootForBody=true;
    owner.GetSession()->LootCorps(source.GetGUID());
    Expect(source.GetOtherRecipient()==&other,source.GetOtherRecipient()==nullptr,
        "Non-owner corpse request must not replace native Fetch recipient");
    // The old SendLoot creates an empty personal cache for the rejected
    // recipient. Isolate the next packet case from that earlier side effect.
    owner.personalLoot.erase(source.GetGUID());
    Actor own(&map,116180,uint32(++nextSource),owner.GetPosition()); owner.BindQuestObject(44923,&own);
    Require(map.AddToMap(static_cast<Creature*>(&own)),"Owner corpse admission failed"); own.setDeathState(CORPSE);
    Seed(own.loot,owner,own); source.SetOtherLootRecipient(other.GetGUID());
    own.LowerPlayerDamageReq(uint32(own.GetMaxHealth())); own.SetLootRecipient(&owner);
    own.AddToLootList(owner.GetGUID()); own.SetFlag(OBJECT_FIELD_DYNAMIC_FLAGS,UNIT_DYNFLAG_LOOTABLE); own.lootForBody=true;
    std::list<Creature*> candidates; owner.GetCorpseCreatureInGrid(candidates,LOOT_DISTANCE);
    Require(std::find(candidates.begin(),candidates.end(),&source)!=candidates.end(),"Private corpse fixture not admitted by actual native pre-scope AoE check");
    owner.GetSession()->LootCorps(own.GetGUID());
    Expect(source.GetOtherRecipient()==&other,source.GetOtherRecipient()==nullptr,
        "AoE corpse enumeration must leave another player's private recipient alone");
    ProbeParty group(owner,other);
    auto slot=Seed(source.loot,other,source);
    RollRequest(owner,source.loot.GetGUID(),slot);
    auto& rolls=group.*Access(GroupRollsTag{});
    Expect(rolls.empty(),!rolls.empty(),
        "Private native container cannot be forced into another player's group roll");
    group.ClearRolls();
#ifndef QUEST_OBJECT_LOOT_BASELINE
    RollRequest(owner,ObjectGuid::Create<HighGuid::Creature>(1220,116180,++nextSource),0);
    RollRequest(owner,ObjectGuid::Empty,0);
    ++checks; // Missing/invalid sources must return before dereferencing Loot.
#endif
}

static void CheckOrdinaryAndFullBags(Map& map, LootPlayer& owner, LootPlayer& other)
{
    owner.Accept(); other.Accept();
    Actor ordinary(&map, 116180, uint32(++nextSource), owner.GetPosition());
    auto slot = Seed(ordinary.loot, owner, ordinary);
    auto before = other.GetItemCount(2589, true);
    Request(other, ordinary.loot.GetGUID(), slot);
    Require(other.GetItemCount(2589, true) == before + 1, "Ordinary unscoped native loot changed"); ++checks;
    owner.DestroyItemCount(2589, owner.GetItemCount(2589, true), true);
    for (uint8 bagSlot = INVENTORY_SLOT_ITEM_START; bagSlot < owner.GetInventoryEndSlot(); ++bagSlot)
    {
        Require(!owner.GetItemByPos(INVENTORY_SLOT_BAG_0, bagSlot), "Native full-bag fixture slot already occupied");
        ItemPosCountVec dest;
        Require(owner.CanStoreNewItem(INVENTORY_SLOT_BAG_0, bagSlot, dest, 2589, 200) == EQUIP_ERR_OK &&
            owner.StoreNewItem(dest, 2589, true), "Native full cloth stack fixture storage failed");
    }
    ItemPosCountVec dest;
    Require(owner.CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, 2589, 1) == EQUIP_ERR_INV_FULL, "Native inventory is not full");
    Actor source(&map, 116180, uint32(++nextSource), owner.GetPosition()); owner.BindQuestObject(44923, &source);
    slot = Seed(source.loot, owner, source); auto unlooted = source.loot.unlootedCount; before = owner.GetItemCount(2589, true);
    Request(owner, source.loot.GetGUID(), slot);
    Require(owner.GetItemCount(2589, true) == before && source.loot.unlootedCount == unlooted &&
        !source.loot.items[slot].is_looted, "Full bags consumed scoped native loot"); ++checks;
    owner.DestroyItem(INVENTORY_SLOT_BAG_0, INVENTORY_SLOT_ITEM_START, true); before = owner.GetItemCount(2589, true);
    Request(owner, source.loot.GetGUID(), slot); Request(owner, source.loot.GetGUID(), slot);
    Require(owner.GetItemCount(2589, true) == before + 1 && source.loot.items[slot].is_looted,
        "Native full-bag retry failed or duplicated item"); ++checks;
    owner.DestroyItemCount(2589, owner.GetItemCount(2589, true), true);
}

static void CheckLateBinding(Map& map, LootPlayer& owner, LootPlayer& other)
{
    owner.Accept(); other.Accept();
    Loot cached;
    {
        Actor source(&map, 116180, uint32(++nextSource), owner.GetPosition());
        auto slot = Seed(cached, owner, source);
        Require(View(cached, other) >= 1, "Ordinary pre-binding loot unexpectedly private"); ++checks;
        owner.BindQuestObject(44923, &source);
        auto before = other.GetItemCount(2589, true);
        Request(other, cached.GetGUID(), slot);
        Expect(other.GetItemCount(2589, true) == before, other.GetItemCount(2589, true) == before + 1,
            "Binding an existing source seals its already-generated personal container");
        slot = Seed(cached, owner, source);
    }
    auto before = owner.GetItemCount(2589, true);
    uint8 slot = 0;
    for (; slot < cached.items.size(); ++slot) if (cached.items[slot].item.ItemID == 2589) break;
    Require(slot < cached.items.size(), "Stored source-less fixture lost ordinary item");
    Request(owner, cached.GetGUID(), slot);
    Require(owner.GetItemCount(2589, true) == before + 1, "Valid owner lost earned loot after source destruction"); ++checks;
}

#ifndef QUEST_OBJECT_LOOT_BASELINE
struct LootScopeMapTag { using Type = std::map<Loot const*, std::weak_ptr<QuestObjectScope>> LootMgr::*; friend Type Access(LootScopeMapTag); };
template struct FixtureAccess<LootScopeMapTag, &LootMgr::m_QuestObjectLoot>;
struct LootSourceMapTag { using Type = std::map<Loot const*, ObjectGuid> LootMgr::*; friend Type Access(LootSourceMapTag); };
template struct FixtureAccess<LootSourceMapTag, &LootMgr::m_LootSources>;
struct LootSourceIndexTag { using Type = std::map<ObjectGuid, std::set<Loot const*>> LootMgr::*; friend Type Access(LootSourceIndexTag); };
template struct FixtureAccess<LootSourceIndexTag, &LootMgr::m_LootsBySource>;

static void CheckRegistry(Map& map, LootPlayer& owner, LootPlayer& other)
{
    owner.Accept(); other.Accept();
    Actor source(&map, 116180, uint32(++nextSource), owner.GetPosition()); owner.BindQuestObject(44923, &source);
    Actor replacement(&map, 116180, uint32(++nextSource), owner.GetPosition()); other.BindQuestObject(44923, &replacement);
    Loot conflict;
    Require(conflict.BindQuestObjectScope(&source) && conflict.CanLootQuestObject(&owner), "Fresh native weak binding failed");
    Require(!conflict.BindQuestObjectScope(&replacement) && !conflict.CanLootQuestObject(&owner) &&
        !conflict.CanLootQuestObject(&other), "Conflicting uncleared container did not close to all recipients"); ++checks;
    conflict.clear();
    Require(conflict.BindQuestObjectScope(&replacement) && conflict.CanLootQuestObject(&other), "Cleared container cannot take its new real source"); ++checks;
    conflict.clear();
    auto mgr = sLootMgr;
    auto countScopes = (mgr->*Access(LootScopeMapTag{})).size();
    auto countSources = (mgr->*Access(LootSourceMapTag{})).size();
    auto countIndex = (mgr->*Access(LootSourceIndexTag{})).size();
    auto refs = owner.getQuestStatus(44923)->ScriptObjects.use_count();
    std::array<Loot,8> containers;
    for (auto& loot : containers) { loot.objGuid = source.GetGUID(); loot.GenerateLootGuid(loot.objGuid); }
    std::atomic<bool> valid{true}; std::vector<std::thread> workers;
    for (auto& loot : containers) workers.emplace_back([&loot,&source,&owner,&other,&valid]()
    {
        for (unsigned i = 0; i < 1000; ++i)
        {
            sLootMgr->AddLoot(&loot);
            if (!loot.BindQuestObjectScope(&source) || !loot.CanLootQuestObject(&owner) || loot.CanLootQuestObject(&other)) valid = false;
            loot.clear(); // Even a registered, otherwise-empty container must unregister.
            if (sLootMgr->GetLoot(loot.GetGUID())) valid = false;
        }
    });
    for (unsigned i = 0; i < 1000; ++i) mgr->BindQuestObjectLoots(&source);
    for (auto& worker : workers) worker.join();
    Require(valid && (mgr->*Access(LootScopeMapTag{})).size() == countScopes &&
        (mgr->*Access(LootSourceMapTag{})).size() == countSources && (mgr->*Access(LootSourceIndexTag{})).size() == countIndex &&
        owner.getQuestStatus(44923)->ScriptObjects.use_count() == refs, "Concurrent native scope/source index lifecycle leaked or lost admission"); ++checks;
    alignas(Loot) unsigned char storage[sizeof(Loot)];
    auto first = new(storage) Loot; Require(first->BindQuestObjectScope(&source), "Placement binding failed"); first->~Loot();
    auto reused = new(storage) Loot;
    Require(reused->CanLootQuestObject(&other), "Reused native container address retained its destroyed lease"); reused->~Loot(); ++checks;
}
#endif

static int QuestLootRun(int argc, char** argv)
{
    InitializeNative(argc, argv); opcodeTable.Initialize(); LoadLootInputs();
    sObjectMgr->LoadPageTexts(); sObjectMgr->LoadGameObjectTemplate();
    LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
    Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Native login pool failed");
    LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY"); LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly = LoginDatabase.Query("SELECT @@session.transaction_read_only");
    Require(readOnly && readOnly->Fetch()[0].GetBool(), "Login pool is not read-only");
    // Socketless account0 sessions survive until process exit, avoiding normal
    // account UPDATE destruction. No real character is loaded or reset.
    auto first = new WorldSession(0, std::string("Quest loot owner probe"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    auto second = new WorldSession(0, std::string("Quest loot other probe"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    HashMapHolder<Player>::SetSize(32); HashMapHolder<GameObject>::SetSize(32);
    {
        Map map(1220, 300, 9999995, DIFFICULTY_NONE);
        LootPlayer owner(first, map, 100, {5478.0f,470.0f,150.0f,0.0f});
        LootPlayer other(second, map, 101, owner.GetPosition());
        for (uint8 race : {uint8(RACE_HUMAN),uint8(RACE_ORC)})
        {
            owner.SetByteValue(UNIT_FIELD_BYTES_0, 0, race); owner.setFactionForRace(race);
            other.SetByteValue(UNIT_FIELD_BYTES_0, 0, race); other.setFactionForRace(race);
            CheckSource(map, owner, other); CheckMoneyAndRetake(map, owner);
            CheckGameObject(map, owner, other); CheckMaster(map, owner, other);
            CheckCachedKinds(map,owner,other); CheckPacketEdges(map,owner,other);
            CheckOrdinaryAndFullBags(map, owner, other);
            CheckLateBinding(map, owner, other);
#ifndef QUEST_OBJECT_LOOT_BASELINE
            CheckRegistry(map, owner, other);
#endif
        }
    }
    LoginDatabase.Close(); HotfixDatabase.Close(); WorldDatabase.Close();
#ifdef QUEST_OBJECT_LOOT_BASELINE
    std::cout << "EXPECTED_BROKEN: " << checks << " pinned330996949 native checks reproduce another player's direct/creature-GUID/Loot-GUID receipt and cancelled source-less item/money receipt; this is a counterfactual, not a successful ordinary run.\n";
#else
    std::cout << "PASS: " << checks << " native quest-object loot checks; real packet handlers, storage, view, cancellation, retake, source removal, money, weak expiry and clear lifecycle; ordinary/full-bag/group coverage recorded separately. No client traversal or all36 completion claim.\n";
#endif
    return 0;
}

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX); SetUnhandledExceptionFilter(CrashTrace);
    try { return QuestLootRun(argc, argv); }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}
