// Full native raid loot generation/receipt and cooldown save/load. Defeated
// encounter state and membership are fixtures, not a running raid/client.
#define LOST_ARMY_TURNIN_NATIVE_SUPPORT_ONLY
#include "LostArmyTurnInNative.cpp"
#include "InstanceScript.h"
#include "GameObject.h"
#include "LFGMgr.h"
#include "AreaTriggerData.h"
#include "ItemEnchantmentMgr.h"

struct InstanceDataTag { using Type = InstanceScript* InstanceMap::*; friend Type Access(InstanceDataTag); };
template struct FixtureAccess<InstanceDataTag, &InstanceMap::i_data>;
struct WeeklyResetTag { using Type = time_t World::*; friend Type Access(WeeklyResetTag); };
template struct FixtureAccess<WeeklyResetTag, &World::m_NextInstanceWeeklyReset>;
struct CooldownLoadTag { using Type = void (Player::*)(PreparedQueryResult); friend Type Access(CooldownLoadTag); };
template struct FixtureAccess<CooldownLoadTag, &Player::_LoadLootCooldown>;
static_assert(sizeof(LootItem) == 0x78 && offsetof(LootItem, m_loot) == 0x68, "Pinned library LootItem ABI changed");
static uint32 guidCounter = 1000;
static Position const raidPosition{466.229f,498.477f,6.06031f,0};
struct RaidPlayer : LootPlayer
{
    RaidPlayer(Map& map, uint32 guid = 0) : LootPlayer(Session(), map, guid ? guid : ++guidCounter, raidPosition)
    {
        SetName("Raid" + std::to_string(GetGUIDLow())); SetQuestStatus(44746, QUEST_STATUS_NONE);
        AddQuestAndCheckCompletion(sQuestDataStore->GetQuestTemplate(45088), nullptr);
    }
    void PutFragments(uint32 count)
    {
        ItemPosCountVec dest; Require(CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, 142487, count) == EQUIP_ERR_OK, "Fragment placement rejected");
        Require(StoreNewItem(dest, 142487, true), "Fragment placement failed");
    }
};
struct RaidParty
{
    Group group;
    RaidParty(std::vector<std::unique_ptr<RaidPlayer>>& players)
    {
        group.*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(++guidCounter, 0);
        group.*Access(GroupFlagsTag{}) = GROUP_FLAG_RAID;
        group.SetLootMethod(PERSONAL_LOOT); sGroupMgr->AddGroup(&group);
        for (auto& player : players)
        {
            Group::MemberSlot slot; slot.Guid = player->GetGUID(); slot.Name = player->GetName(); slot.Class = player->getClass();
            const_cast<Group::MemberSlotList&>(group.GetMemberSlots()).push_back(slot); player->SetGroup(&group, 0);
        }
    }
    ~RaidParty()
    {
        while (auto ref = group.GetFirstMember()) ref->getSource()->SetGroup(nullptr);
        sGroupMgr->RemoveGroup(&group);
    }
};
struct Chest : GameObject
{
    Chest(Map& map)
    {
        Require(Create(++guidCounter, 260526, &map, 1, raidPosition, G3D::Quat(0,0,0,1), 0, GO_STATE_READY), "Native Odyn chest creation failed");
        Require(map.AddToMap(static_cast<GameObject*>(this)), "Native Odyn chest map admission failed");
    }
    ~Chest() { if (IsInWorld()) GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false); ResetMap(); }
};
static void LoadRaidRows()
{
    LoadRandomEnchantmentsTable();
    sObjectMgr->LoadPageTexts(); sObjectMgr->LoadGameObjectTemplate(); sObjectMgr->LoadInstanceTemplate(); sAreaTriggerDataStore->LoadAreaTriggerTeleports(); sLFGMgr->LoadLFGDungeons(); sObjectMgr->LoadInstanceEncounters(); sObjectMgr->LoadPersonalLootTemplate();
    for (auto store : {&LootTemplates_Creature, &LootTemplates_Gameobject})
    {
        auto& name = *store.*Access(LootTableNameTag{}); auto saved = name;
        name += store == &LootTemplates_Creature ? " WHERE Entry IN(114323,114537)" : " WHERE Entry=70585";
        auto count = (*store.*Access(LoadLootTableTag{}))(); name = saved;
        Require(count > 0, "Native raid loot loader empty");
    }
    auto conditions = WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=1 AND SourceGroup IN(114323,114537)) OR (SourceTypeOrReferenceId=4 AND SourceGroup=70585)");
    Require(conditions && !conditions->Fetch()[0].GetUInt32(), "Load new raid conditions before testing");
    Require(sObjectMgr->GetGameObjectTemplate(260526)->GetLootId() == 70585, "Native chest loot link changed");
}
static void Generate(InstanceMap& map, std::vector<std::unique_ptr<RaidPlayer>>& players, uint32 entry)
{
    Actor boss(&map, entry, ++guidCounter, raidPosition);
    Require(map.AddToMap(static_cast<Creature*>(&boss)), "Native raid boss creation failed");
    Require(boss.IsPersonal() && boss.isWorldBoss(), "Native boss loot flags changed");
    boss.SetLootRecipient(players.front().get());
    players.front()->GeneratePersonalLoot(&boss, players.front().get());
    if (!map.IsLfr())
    {
        for (auto& player : players)
        {
            auto personal = player->personalLoot.find(boss.GetGUID());
            if (personal != player->personalLoot.end()) { personal->second.AutoStoreItems(); player->RemoveLoot(boss.GetGUID()); }
            for (uint8 slot = 0; slot < boss.loot.items.size() + boss.loot.quest_items.size(); ++slot) player->StoreLootItem(slot, &boss.loot);
        }
    }
}
static void CheckEligibility(InstanceMap& map, uint32 entry)
{
    std::vector<std::unique_ptr<RaidPlayer>> players;
    for (unsigned i=0;i<5;++i) players.emplace_back(new RaidPlayer(map)); RaidParty party(players);
    players[1]->SetQuestStatus(45088,QUEST_STATUS_NONE);
    players[2]->PutFragments(1000);
    players[3]->SetQuestStatus(45088,QUEST_STATUS_COMPLETE);
    Generate(map,players,entry);
    Require(players[0]->GetItemCount(142487)>0 && players[4]->GetItemCount(142487)>0, "Independent eligible raid recipients lost fragments");
    Require(!players[1]->GetItemCount(142487) && players[2]->GetItemCount(142487)==1000 && !players[3]->GetItemCount(142487), "No-quest/quota/completed raid recipient gained fragments");
    std::cout << "PASS: native raid admission denies absent/completed quest and1000 quota while independent eligible recipients receive loot.\n";
}
static void CheckMixed(InstanceMap& map, uint32 entry, LootMethod method)
{
    std::vector<std::unique_ptr<RaidPlayer>> players;
    for(unsigned i=0;i<5;++i) players.emplace_back(new RaidPlayer(map)); RaidParty party(players); party.group.SetLootMethod(method);
    // A saved member can join a fresh group's later kill. Native cooldown
    // mutation is setup here; the persistence sequence below earns/loads it.
    players[0]->AddPlayerLootCooldown(entry,0,TYPE_CREATURE,true,map.GetDifficultyID());
    Generate(map,players,entry);
#ifdef RAID_FRAGMENTS_BASELINE
    Require(players[0]->GetItemCount(142487)>0, "Pinned mixed-eligibility bypass changed");
#else
    Require(!players[0]->GetItemCount(142487), "Saved member received a fresh group's shared quest loot");
#endif
    for(unsigned i=1;i<5;++i) Require(players[i]->GetItemCount(142487)>0, "Fresh member lost mixed-party loot");
    std::cout << "PASS: actual mixed saved/fresh raid recipients and loot method=" << uint32(method) << " checked.\n";
}
static void CheckPersisted(Map& parent, uint32 entry)
{
    uint32 savedGuid=++guidCounter; bool first=true;
    for(auto diff : {DIFFICULTY_LFR_RAID,DIFFICULTY_NORMAL_RAID,DIFFICULTY_HEROIC_RAID,DIFFICULTY_MYTHIC_RAID,DIFFICULTY_NORMAL_RAID})
    {
        InstanceMap map(1648,300,++guidCounter,diff,&parent); map.*Access(InstanceDataTag{})=new InstanceScript(&map);
        std::vector<std::unique_ptr<RaidPlayer>> players; players.emplace_back(new RaidPlayer(map,savedGuid)); auto& p=*players.front();
        if(!first)
        {
            auto stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_INVENTORY);stmt->setUInt64(0,savedGuid);(p.*Access(InventoryLoadTag{}))(ReadPrepared(stmt),{},0);
            stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_PLAYER_LOOTCOOLDOWN);stmt->setUInt64(0,savedGuid);(p.*Access(CooldownLoadTag{}))(ReadPrepared(stmt));
        }
        auto type=entry==260526?TYPE_GO:TYPE_CREATURE; auto before=p.GetItemCount(142487);bool saved=p.IsPlayerLootCooldown(entry,type,diff);
        if(entry==260526){Chest chest(map);p.SendLoot(chest.GetGUID(),LOOT_CORPSE);if(auto loot=p.personalLoot.find(chest.GetGUID());loot!=p.personalLoot.end())loot->second.AutoStoreItems();p.GetSession()->DoLootRelease(chest.GetGUID());}
        else Generate(map,players,entry);
        auto after=p.GetItemCount(142487);
#ifdef RAID_FRAGMENTS_BASELINE
        Require(after>before, "Pinned source/difficulty persistence behavior changed");
#else
        Require(saved ? after==before : after>before, "Native saved cooldown/difficulty boundary failed");
#endif
        Require(p.IsPlayerLootCooldown(entry,type,diff), "Native source cooldown not recorded");
        p.SaveToDB(first);FlushWrites();first=false;
        std::cout << "PERSIST source="<<entry<<" diff="<<uint32(diff)<<" saved="<<saved<<" fragments="<<before<<"->"<<after<<std::endl;
    }
    std::cout << "PASS: actual saved inventory/cooldown prepared reload checks all four raid difficulties and a repeated Normal reward.\n";
}
static void CheckPending(InstanceMap& map,uint32 entry)
{
    std::vector<std::unique_ptr<RaidPlayer>> players;
    for(unsigned i=0;i<5;++i)players.emplace_back(new RaidPlayer(map));RaidParty party(players);auto& p=*players[0];
    for(uint8 slot=INVENTORY_SLOT_ITEM_START;slot<INVENTORY_SLOT_ITEM_END;++slot)
    {ItemPosCountVec dest;dest.emplace_back((INVENTORY_SLOT_BAG_0<<8)|slot,sObjectMgr->GetItemTemplate(2589)->GetMaxStackSize());Require(p.StoreNewItem(dest,2589,true),"Full raid inventory setup failed");}
    Actor boss(&map,entry,++guidCounter,raidPosition);Require(map.AddToMap(static_cast<Creature*>(&boss)),"Pending boss map admission failed");
    boss.LowerPlayerDamageReq(uint32(boss.GetMaxHealth()));boss.SetLootRecipient(&p);p.Kill(&boss,false);
    Require(boss.isDead(false),"Actual native raid kill failed");
    if(!map.IsLfr()) for(uint8 slot=0;slot<boss.loot.items.size()+boss.loot.quest_items.size();++slot)p.StoreLootItem(slot,&boss.loot);
    Require(!p.GetItemCount(142487) && p.IsPlayerLootCooldown(entry,TYPE_CREATURE,map.GetDifficultyID()),"Full bags granted fragments or failed cooldown");
    for(uint8 slot=INVENTORY_SLOT_ITEM_START;slot<INVENTORY_SLOT_ITEM_END;++slot)if(auto item=p.GetItemByPos(INVENTORY_SLOT_BAG_0,slot))if(item->GetEntry()==2589)p.DestroyItem(INVENTORY_SLOT_BAG_0,slot,true);
    auto personal=p.personalLoot.find(boss.GetGUID());
#ifdef RAID_FRAGMENTS_BASELINE
    if(map.IsLfr())Require(personal==p.personalLoot.end(),"Pinned LFR full-bag loss changed");
#else
    if(map.IsLfr())Require(personal!=p.personalLoot.end()&&boss.HasInLootList(p.GetGUID()),"LFR discarded pending full-bag loot");
#endif
    if(personal!=p.personalLoot.end())personal->second.AutoStoreItems();
    if(!map.IsLfr())for(uint8 slot=0;slot<boss.loot.items.size()+boss.loot.quest_items.size();++slot)p.StoreLootItem(slot,&boss.loot);
#ifdef RAID_FRAGMENTS_BASELINE
    Require(map.IsLfr() ? !p.GetItemCount(142487) : p.GetItemCount(142487)>0,"Pinned full-bag retry behavior changed");
#else
    Require(p.GetItemCount(142487)>0,"Native earned loot cannot be retried after cooldown");
#endif
    auto count=p.GetItemCount(142487);if(personal!=p.personalLoot.end())personal->second.AutoStoreItems();
    for(uint8 slot=0;slot<boss.loot.items.size()+boss.loot.quest_items.size();++slot)p.StoreLootItem(slot,&boss.loot);
    Require(p.GetItemCount(142487)==count,"Repeated pending-loot receipt duplicated fragments");
    std::cout<<"PASS: full native Unit::Kill, full inventory, earned-container retry after cooldown and repeated receipt checked; diff="<<uint32(map.GetDifficultyID())<<" fragments="<<count<<std::endl;
}
static void CheckBoss(InstanceMap& map, uint32 entry)
{
    std::vector<std::unique_ptr<RaidPlayer>> players;
    for (unsigned i = 0; i < 5; ++i) players.emplace_back(new RaidPlayer(map)); RaidParty party(players);
    Generate(map, players, entry);
    unsigned noGear = 0; std::vector<uint32> first;
    for (auto& p : players)
    {
        auto fragments = p->GetItemCount(142487); Require(fragments >= (entry == 114537 ? 100u : 30u) && fragments <= (entry == 114537 ? 150u : 60u), "Fresh native boss fragments missing/outside live bounds"); first.push_back(fragments);
        Require(p->IsPlayerLootCooldown(entry, TYPE_CREATURE, map.GetDifficultyID()), "Native boss cooldown missing");
        bool gear = false;
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot) if (auto item = p->GetItemByPos(INVENTORY_SLOT_BAG_0, slot)) if (item->GetEntry() != 142487 && item->GetTemplate()->GetClass() != ITEM_CLASS_QUEST) gear = true;
        if (!gear) ++noGear;
    }
    if (map.IsLfr()) Require(noGear >= 4, "No LFR equipment-nonwinner exercised");
    Generate(map, players, entry);
    bool repeat = false; for (unsigned i = 0; i < players.size(); ++i) repeat |= players[i]->GetItemCount(142487) > first[i];
    std::cout << "BOSS entry=" << entry << " diff=" << uint32(map.GetDifficultyID()) << " noGear=" << noGear << " repeated=" << repeat << std::endl;
#ifdef RAID_FRAGMENTS_BASELINE
    Require(repeat == !map.IsLfr(), "Pinned repeat-generation behavior changed");
#else
    Require(!repeat, "Saved boss cooldown allowed repeated fragments");
#endif
}
static void CheckChest(InstanceMap& map)
{
    RaidPlayer p(map); Chest chest(map);
    p.SendLoot(chest.GetGUID(), LOOT_CORPSE);
    Require(p.personalLoot.count(chest.GetGUID()), "Odyn chest personal loot not constructed");
    p.personalLoot.at(chest.GetGUID()).AutoStoreItems(); auto count = p.GetItemCount(142487);
    Require(count >= 30 && count <= 60, "Fresh Odyn chest fragment receipt failed");
    p.GetSession()->DoLootRelease(chest.GetGUID());
    std::cout << "CHEST diff=" << uint32(map.GetDifficultyID()) << " fragments=" << count << " cooldown=" << p.IsPlayerLootCooldown(260526, TYPE_GO, map.GetDifficultyID()) << " retained=" << p.personalLoot.count(chest.GetGUID()) << std::endl;
    // A different object GUID models a fresh saved instance/chest, preserving
    // the native entry/difficulty cooldown rather than seeding it manually.
    Chest again(map); p.SendLoot(again.GetGUID(), LOOT_CORPSE);
    if (auto loot = p.personalLoot.find(again.GetGUID()); loot != p.personalLoot.end()) loot->second.AutoStoreItems();
    bool repeat = p.GetItemCount(142487) > count;
#ifdef RAID_FRAGMENTS_BASELINE
    Require(repeat, "Pinned chest repeat no longer reproduced");
#else
    Require(!repeat, "Saved chest cooldown allowed repeated fragments");
#endif
    std::cout << "PASS: actual Odyn SendLoot/FillLoot/AutoStoreItems checked with different chest GUID and native entry/difficulty cooldown.\n";
}
static void CheckChestBoundaries(InstanceMap& map)
{
    for(unsigned state=0;state<3;++state)
    {
        RaidPlayer p(map); if(state==0)p.SetQuestStatus(45088,QUEST_STATUS_NONE);else if(state==1)p.PutFragments(1000);else p.SetQuestStatus(45088,QUEST_STATUS_COMPLETE);
        auto before=p.GetItemCount(142487); Chest chest(map);p.SendLoot(chest.GetGUID(),LOOT_CORPSE);
        if(auto loot=p.personalLoot.find(chest.GetGUID());loot!=p.personalLoot.end())loot->second.AutoStoreItems();
        Require(p.GetItemCount(142487)==before,"Odyn chest bypassed quest/quota admission");p.GetSession()->DoLootRelease(chest.GetGUID());
    }
    RaidPlayer p(map); Chest chest(map);
    for(uint8 slot=INVENTORY_SLOT_ITEM_START;slot<INVENTORY_SLOT_ITEM_END;++slot)
    {ItemPosCountVec dest;dest.emplace_back((INVENTORY_SLOT_BAG_0<<8)|slot,sObjectMgr->GetItemTemplate(2589)->GetMaxStackSize());Require(p.StoreNewItem(dest,2589,true),"Full chest inventory setup failed");}
    p.SendLoot(chest.GetGUID(),LOOT_CORPSE);Require(p.personalLoot.count(chest.GetGUID())&&p.personalLoot.at(chest.GetGUID()).dungeonEncounterID,"Native earned chest container missing");
    p.personalLoot.at(chest.GetGUID()).AutoStoreItems();Require(!p.GetItemCount(142487),"Full chest inventory received fragments");
    for(uint8 slot=INVENTORY_SLOT_ITEM_START;slot<INVENTORY_SLOT_ITEM_END;++slot)if(auto item=p.GetItemByPos(INVENTORY_SLOT_BAG_0,slot))if(item->GetEntry()==2589)p.DestroyItem(INVENTORY_SLOT_BAG_0,slot,true);
    // SendLoot's native release may auto-store the retained reward before it
    // reopens. Either route must grant the original earned stack exactly once.
    p.SendLoot(chest.GetGUID(),LOOT_CORPSE);
    if(auto loot=p.personalLoot.find(chest.GetGUID());loot!=p.personalLoot.end())loot->second.AutoStoreItems();
#ifdef RAID_FRAGMENTS_BASELINE
    Require(p.GetItemCount(142487)>=60&&p.GetItemCount(142487)<=120,"Pinned chest reopen no longer duplicates its earned stack");
#else
    Require(p.GetItemCount(142487)>=30&&p.GetItemCount(142487)<=60,"Cooldown blocked earned chest inventory retry");
#endif
    std::cout<<"PASS: native Odyn quest/quota admission and retained full-bag chest retry checked.\n";
}
static int RaidRun(int argc, char** argv)
{
    InitializeNative(argc, argv); opcodeTable.Initialize(); LoadLootInputs(); OpenScratch(); LoadRaidRows();
    sWorld->*Access(WeeklyResetTag{}) = GameTime::GetGameTime() + WEEK;
    HashMapHolder<Player>::SetSize(256); HashMapHolder<GameObject>::SetSize(256);
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1648,{}}});
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1648,{}}});
    Map parent(1648,300,0,DIFFICULTY_NONE);
    for (auto diff : {DIFFICULTY_LFR_RAID, DIFFICULTY_NORMAL_RAID, DIFFICULTY_HEROIC_RAID, DIFFICULTY_MYTHIC_RAID})
    {
        InstanceMap map(1648,300,990000 + uint32(diff),diff,&parent);
        map.*Access(InstanceDataTag{}) = new InstanceScript(&map);
        CheckBoss(map,114323); CheckBoss(map,114537);
        CheckEligibility(map,114323); CheckEligibility(map,114537);
        if(!map.IsLfr()){CheckPending(map,114323);CheckPending(map,114537);}
        if(!map.IsLfr()) for(auto entry:{114323u,114537u}) for(auto method:{PERSONAL_LOOT,GROUP_LOOT,MASTER_LOOT}) CheckMixed(map,entry,method);
    }
    CheckPersisted(parent,114323);CheckPersisted(parent,114537);
    FlushWrites(); CharacterDatabase.Close(); LoginDatabase.Close(); HotfixDatabase.Close(); WorldDatabase.Close(); return 0;
}
int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX); SetUnhandledExceptionFilter(CrashTrace);
    try { return RaidRun(argc,argv); } catch(std::exception const& e) { std::cerr << e.what() << '\n'; return 1; }
}
