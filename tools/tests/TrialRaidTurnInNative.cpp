// Native quest packet handlers, menu eligibility, inventory and prepared saves.
// Defeated questgiver placement and earlier44721 completion are fixture inputs.
#define TRIAL_RAID_CRITERIA_SUPPORT_ONLY
#include "TrialRaidCriteriaNative.cpp"
#include "GossipDef.h"

struct StartersTag { using Type=QuestRelations QuestDataStoreMgr::*;friend Type Access(StartersTag); };
template struct FixtureAccess<StartersTag,&QuestDataStoreMgr::_creatureQuestRelations>;
struct StarterIndexTag { using Type=QuestStarter QuestDataStoreMgr::*;friend Type Access(StarterIndexTag); };
template struct FixtureAccess<StarterIndexTag,&QuestDataStoreMgr::_creatureQuestStarter>;
struct RelationsLoaderTag { using Type=void (QuestDataStoreMgr::*)(QuestRelations&,QuestStarter&,std::string const&,bool,bool);friend Type Access(RelationsLoaderTag); };
template struct FixtureAccess<RelationsLoaderTag,&QuestDataStoreMgr::LoadQuestRelationsHelper>;
struct RewardSaveTag { using Type=QuestStatusSaveMap Player::*;friend Type Access(RewardSaveTag); };
template struct FixtureAccess<RewardSaveTag,&Player::m_RewardedQuestsSave>;

static void LoadTurnInInputs()
{
    LoadTrialInputs();
    auto rows=WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN(44721,44729,44868)");
    Require(bool(rows),"Missing Trial quest addons");unsigned count=0;
    do {auto f=rows->Fetch();auto q=const_cast<Quest*>(sQuestDataStore->GetQuestTemplate(f[0].GetUInt32()));Require(q,"Missing Trial addon quest");q->LoadQuestTemplateAddon(f);++count;}while(rows->NextRow());
    Require(count==3,"Trial addon count changed");
    // Compile the actual native LoadQuests link-building blocks, including
    // prevChainQuests and both PrevQuestID/NextQuestID prerequisite routes.
    auto& _questTemplates=const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates());
    auto& mExclusiveQuestGroups=sQuestDataStore->mExclusiveQuestGroups;
    for(auto id:{44721u,44729u,44868u})
    {
        auto qinfo=_questTemplates.at(id);
#include "NativeTrialQuestLinks.inc"
    }
    auto& starters=sQuestDataStore->*Access(StartersTag{});auto& ends=sQuestDataStore->*Access(EndersTag{});auto& index=sQuestDataStore->*Access(StarterIndexTag{});
    (sQuestDataStore->*Access(RelationsLoaderTag{}))(starters,index,"(SELECT id,quest FROM creature_queststarter WHERE quest IN(44729,44868))",true,false);
    (sQuestDataStore->*Access(RelationsLoaderTag{}))(ends,index,"(SELECT id,quest FROM creature_questender WHERE quest IN(44729,44868))",false,false);
    Require(starters.size()==2&&ends.size()==2,"Native Trial quest relations changed");
    auto conditions=WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceEntry IN(44729,44868)");
    Require(conditions&&!conditions->Fetch()[0].GetUInt32(),"Load newly added quest acceptance conditions before testing");
    for(auto pair:{std::array<uint32,2>{44729,142533},std::array<uint32,2>{44868,142534}})
    {
        auto q=sQuestDataStore->GetQuestTemplate(pair[0]);Require(q&&!q->m_rewChoiceItemsCount&&!q->PackageID&&q->RewardItemId[0]==pair[1]&&q->RewardItemCount[0]==1,"Native fixed reward structure changed");
    }
}
static void Accept(TrialPlayer& p,Actor& giver,uint32 id)
{
    WorldPacket wire(CMSG_QUEST_GIVER_ACCEPT_QUEST);wire<<giver.GetGUID()<<int32(id);wire.WriteBit(false);wire.FlushBits();
    WorldPackets::Quest::QuestGiverAcceptQuest packet(std::move(wire));packet.Read();p.GetSession()->HandleQuestGiverAcceptQuest(packet);Tick(p,200);
}
static void Claim(TrialPlayer& p,Actor& giver,uint32 id)
{
    WorldPacket wire(CMSG_QUEST_GIVER_CHOOSE_REWARD);wire<<giver.GetGUID()<<int32(id)<<int32(0);
    WorldPackets::Quest::QuestGiverChooseReward packet(std::move(wire));packet.Read();p.GetSession()->HandleQuestGiverChooseReward(packet);Tick(p,200);FlushWrites();
}
static bool MenuHas(TrialPlayer& p,Actor& giver,uint32 id,uint8 icon)
{
    p.PrepareQuestMenu(giver.GetGUID());auto& menu=p.PlayerTalkClass->GetQuestMenu();
    for(uint8 i=0;i<menu.GetMenuItemCount();++i)if(menu.GetItem(i).QuestId==id&&menu.GetItem(i).QuestIcon==icon)return true;
    return false;
}
static void FillBags(TrialPlayer& p)
{
    for(uint8 slot=INVENTORY_SLOT_ITEM_START;slot<INVENTORY_SLOT_ITEM_END;++slot)if(!p.GetItemByPos(INVENTORY_SLOT_BAG_0,slot))
    {ItemPosCountVec dest;dest.emplace_back((INVENTORY_SLOT_BAG_0<<8)|slot,sObjectMgr->GetItemTemplate(2589)->GetMaxStackSize());Require(p.StoreNewItem(dest,2589,true),"Full Trial inventory setup failed");}
}
static void FreeSlot(TrialPlayer& p)
{
    for(uint8 slot=INVENTORY_SLOT_ITEM_START;slot<INVENTORY_SLOT_ITEM_END;++slot)if(auto item=p.GetItemByPos(INVENTORY_SLOT_BAG_0,slot))if(item->GetEntry()==2589){p.DestroyItem(INVENTORY_SLOT_BAG_0,slot,true);return;}
    throw std::runtime_error("No Trial filler slot to release");
}
static void Checkpoint(TrialPlayer& p,uint32 id,uint32 reward,bool rewarded)
{
    auto result=CharacterDatabase.Query(Trinity::StringFormat("SELECT (SELECT COUNT(*) FROM character_queststatus_rewarded WHERE guid=%u AND quest=%u),(SELECT COALESCE(SUM(ii.count),0) FROM character_inventory ci JOIN item_instance ii ON ci.item=ii.guid WHERE ci.guid=%u AND ii.itemEntry=%u)",p.GetGUIDLow(),id,p.GetGUIDLow(),reward).c_str());
    Require(result&&result->Fetch()[0].GetUInt32()==uint32(rewarded)&&result->Fetch()[1].GetUInt32()==uint32(rewarded),"Native fixed reward checkpoint differs from completion/inventory");
    auto gold=CharacterDatabase.Query(Trinity::StringFormat("SELECT money FROM characters WHERE guid=%u",p.GetGUIDLow()).c_str());
    Require(gold&&gold->Fetch()[0].GetUInt64()==p.GetMoney(),"Native Trial reward checkpoint lost gold");
}
static void Reload(TrialPlayer& p)
{
    auto stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_INVENTORY);stmt->setUInt64(0,p.GetGUIDLow());(p.*Access(InventoryLoadTag{}))(ReadPrepared(stmt),{},0);
    stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS);stmt->setUInt32(0,0);stmt->setUInt64(1,p.GetGUIDLow());(p.*Access(QuestLoadTag{}))(ReadPrepared(stmt));
    stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_OBJECTIVES);stmt->setUInt32(0,0);stmt->setUInt64(1,p.GetGUIDLow());(p.*Access(ObjectiveLoadTag{}))(ReadPrepared(stmt));
    stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUSREW);stmt->setUInt32(0,0);stmt->setUInt64(1,p.GetGUIDLow());(p.*Access(RewardedLoadTag{}))(ReadPrepared(stmt));
}
static void CheckChain(InstanceMap& map,uint8 race,bool fullBags)
{
    Actor havi(&map,92539,++nextGuid,pos),odyn(&map,114263,++nextGuid,pos),afterHelya(&map,116760,++nextGuid,pos);
    for(auto giver:{&havi,&odyn,&afterHelya})Require(map.AddToMap(static_cast<Creature*>(giver)),"Native Trial questgiver admission failed");
    auto guid=++nextGuid;
    {
        TrialPlayer p(map,race,guid);
        Require(!p.CanTakeQuest(sQuestDataStore->GetQuestTemplate(44729),false)&&!p.CanTakeQuest(sQuestDataStore->GetQuestTemplate(44868),false),"Native previous quest gates missing");
        // The already completed preceding story quest is the only progress seed.
        const_cast<RewardedQuestSet&>(p.getRewardedQuests()).insert(44721);(p.*Access(RewardSaveTag{}))[44721]=QUEST_DEFAULT_SAVE_TYPE;
        Require(MenuHas(p,havi,44729,2),"Havi does not offer native44729");Accept(p,havi,44729);Require(p.GetQuestStatus(44729)==QUEST_STATUS_INCOMPLETE,"Native44729 acceptance failed");
        Require(!MenuHas(p,odyn,44868,2),"Next quest offered before44729 reward");Accept(p,odyn,44868);Require(p.GetQuestStatus(44868)==QUEST_STATUS_NONE,"Early next quest acceptance bypassed prerequisites");
        Emit(map,1958);Tick(p,2000);Require(p.GetQuestStatus(44729)==QUEST_STATUS_COMPLETE&&MenuHas(p,odyn,44729,4),"Native44729 completion/menu failed");
        if(fullBags)FillBags(p);p.SaveToDB(true);FlushWrites();
        for(auto node:{std::array<uint32,3>{44729,142533,1958},std::array<uint32,3>{44868,142534,2008}})
        {
            auto id=node[0],reward=node[1];auto& giver=id==44729?odyn:afterHelya;
            Require(p.CanInteractWithQuestGiver(&giver),"Native defeated Odyn interaction denied");
            Claim(p,id==44729?afterHelya:odyn,id);Require(!p.GetQuestRewardStatus(id)&&!p.GetItemCount(reward),"Wrong Trial questgiver granted reward");
            p.Relocate(pos.GetPositionX()+100,pos.GetPositionY(),pos.GetPositionZ());Claim(p,giver,id);p.Relocate(pos);
            Require(!p.GetQuestRewardStatus(id)&&!p.GetItemCount(reward),"Distant Trial questgiver granted reward");
            if(fullBags)
            {
                Require(!p.CanRewardQuest(sQuestDataStore->GetQuestTemplate(id),0,false),"Full bags admitted fixed Trial reward");Claim(p,giver,id);
                Require(!p.GetQuestRewardStatus(id)&&!p.GetItemCount(reward)&&p.GetQuestStatus(id)==QUEST_STATUS_COMPLETE,"Full bags consumed Trial quest/reward");
                Checkpoint(p,id,reward,false);FreeSlot(p);
            }
            Require(p.CanRewardQuest(sQuestDataStore->GetQuestTemplate(id),0,false),"Native fixed reward admission failed");Claim(p,giver,id);
            Require(p.GetQuestRewardStatus(id)&&p.GetItemCount(reward)==1,"Native Trial reward missing");Checkpoint(p,id,reward,true);
            Claim(p,giver,id);Require(p.GetItemCount(reward)==1,"Repeated Trial reward duplicated item");Checkpoint(p,id,reward,true);
            if(id==44729)
            {
                Require(p.GetNextQuest(odyn.GetGUID(),sQuestDataStore->GetQuestTemplate(id))==sQuestDataStore->GetQuestTemplate(44868)&&MenuHas(p,odyn,44868,2),"Native next quest missing after44729 reward");
                Accept(p,odyn,44868);Require(p.GetQuestStatus(44868)==QUEST_STATUS_INCOMPLETE,"Native44868 acceptance failed");Emit(map,2008);Tick(p,2000);Require(p.GetQuestStatus(44868)==QUEST_STATUS_COMPLETE&&MenuHas(p,afterHelya,44868,4),"Native44868 completion/menu failed");
                if(fullBags)FillBags(p);p.SaveToDB();FlushWrites();
            }
            else Require(!p.GetNextQuest(afterHelya.GetGUID(),sQuestDataStore->GetQuestTemplate(id)),"Native Trial ending unexpectedly offers a next quest");
        }
        // Checkpoints above precede this full save; they test the reward's own
        // transaction rather than concealing a missing write with SaveToDB.
        p.SaveToDB();FlushWrites();
    }
    {
        TrialPlayer p(map,race,guid);Reload(p);
        for(auto pair:{std::array<uint32,2>{44729,142533},std::array<uint32,2>{44868,142534}})
        {
            auto& giver=pair[0]==44729?odyn:afterHelya;Require(p.GetQuestRewardStatus(pair[0])&&p.GetItemCount(pair[1])==1,"Native Trial reward reload lost state");
            Claim(p,giver,pair[0]);Require(p.GetItemCount(pair[1])==1&&!MenuHas(p,giver,pair[0],4),"Reloaded Trial quest rewarded again");
        }
    }
    std::cout<<"PASS: native44729 ->44868 acceptance/menu/reward/checkpoint/repeat/prepared reload; race="<<uint32(race)<<" fullBags="<<fullBags<<" diff="<<uint32(map.GetDifficultyID())<<std::endl;
}
static void CheckSavedRetry(InstanceMap& map,uint8 race)
{
    Actor havi(&map,92539,++nextGuid,pos),odyn(&map,114263,++nextGuid,pos),afterHelya(&map,116760,++nextGuid,pos);
    for(auto giver:{&havi,&odyn,&afterHelya})Require(map.AddToMap(static_cast<Creature*>(giver)),"Saved Trial questgiver admission failed");
    auto guid=++nextGuid;
    {
        TrialPlayer p(map,race,guid);const_cast<RewardedQuestSet&>(p.getRewardedQuests()).insert(44721);(p.*Access(RewardSaveTag{}))[44721]=QUEST_DEFAULT_SAVE_TYPE;
        Accept(p,havi,44729);Claim(p,odyn,44729);Require(!p.GetQuestRewardStatus(44729)&&!p.GetItemCount(142533),"Incomplete Trial quest rewarded");
        Emit(map,1958);Tick(p,2000);FillBags(p);p.SaveToDB(true);FlushWrites();Claim(p,odyn,44729);Checkpoint(p,44729,142533,false);
    }
    {
        TrialPlayer p(map,race,guid);Reload(p);Require(p.GetQuestStatus(44729)==QUEST_STATUS_COMPLETE&&!p.GetItemCount(142533),"Reload lost pending44729");
        Claim(p,odyn,44729);Checkpoint(p,44729,142533,false);FreeSlot(p);Claim(p,odyn,44729);Checkpoint(p,44729,142533,true);
        // Deliberately no full save after the reward checkpoint.
    }
    {
        TrialPlayer p(map,race,guid);Reload(p);Require(p.GetQuestRewardStatus(44729)&&p.GetItemCount(142533)==1&&MenuHas(p,odyn,44868,2),"Immediate reward checkpoint reload lost item or next offer");
        Claim(p,odyn,44729);Require(p.GetItemCount(142533)==1,"Immediate checkpoint replay duplicated Titan's Boon");
        Accept(p,odyn,44868);Require(p.GetQuestStatus(44868)==QUEST_STATUS_INCOMPLETE,"Reloaded handoff cannot accept44868");Emit(map,2008);Tick(p,2000);
        FillBags(p);p.SaveToDB();FlushWrites();Claim(p,afterHelya,44868);Checkpoint(p,44868,142534,false);
    }
    {
        TrialPlayer p(map,race,guid);Reload(p);Require(p.GetQuestStatus(44868)==QUEST_STATUS_COMPLETE&&p.GetItemCount(142533)==1&&!p.GetItemCount(142534),"Reload lost pending44868 or previous reward");
        Claim(p,afterHelya,44868);Checkpoint(p,44868,142534,false);FreeSlot(p);Claim(p,afterHelya,44868);Checkpoint(p,44868,142534,true);
    }
    {
        TrialPlayer p(map,race,guid);Reload(p);
        for(auto pair:{std::array<uint32,2>{44729,142533},std::array<uint32,2>{44868,142534}})
        {Require(p.GetQuestRewardStatus(pair[0])&&p.GetItemCount(pair[1])==1,"Final checkpoint reload lost reward");Claim(p,pair[0]==44729?odyn:afterHelya,pair[0]);Require(p.GetItemCount(pair[1])==1,"Checkpoint reload duplicated reward");}
        Require(!MenuHas(p,odyn,44868,2)&&!MenuHas(p,afterHelya,44868,4),"Rewarded Trial ending still offered");
    }
    std::cout<<"PASS: native full-bag pending reload/retry, immediate checkpoint reload before another SaveToDB, saved44729 ->44868 offer/accept and two fixed rewards; race="<<uint32(race)<<" diff="<<uint32(map.GetDifficultyID())<<std::endl;
}
#ifndef TRIAL_RAID_TURNIN_SUPPORT_ONLY
static int RunTurnIn(int argc,char** argv)
{
    InitializeNative(argc,argv);opcodeTable.Initialize();LoadTurnInInputs();OpenScratch();
    sAchievementMgr->LoadRewards();sAchievementMgr->LoadCompletedAchievements();HashMapHolder<Player>::SetSize(256);
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1648,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1648,{}}});
    {
        Map parent(1648,300,0,DIFFICULTY_NONE);
        for(auto diff:{DIFFICULTY_LFR_RAID,DIFFICULTY_NORMAL_RAID,DIFFICULTY_HEROIC_RAID,DIFFICULTY_MYTHIC_RAID})
        {InstanceMap map(1648,300,++nextGuid,diff,&parent);map.*Access(TrialInstanceTag{})=new InstanceScript(&map);for(auto race:{RACE_HUMAN,RACE_ORC}){for(bool full:{false,true})CheckChain(map,race,full);CheckSavedRetry(map,race);}}
    }
    FlushWrites();CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();return 0;
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{return RunTurnIn(argc,argv);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
#endif
