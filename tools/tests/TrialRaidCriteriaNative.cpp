// Actual InstanceScript -> Player -> AchievementMgr -> CriteriaTree -> delayed
// quest completion. Native DB2/SQL loaders, no modeled achievement bridge.
#define LOST_ARMY_TURNIN_NATIVE_SUPPORT_ONLY
#include "LostArmyTurnInNative.cpp"
#include "AchievementMgr.h"
#include "InstanceScript.h"
#include "CreatureTextMgr.h"
#include "TrialOfValor/trial_of_valor.h"
void AddSC_tov_boss_odyn();
void AddSC_boss_tov_helya();

struct TrialInstanceTag { using Type = InstanceScript* InstanceMap::*; friend Type Access(TrialInstanceTag); };
template struct FixtureAccess<TrialInstanceTag, &InstanceMap::i_data>;
struct QuestLoadTag { using Type = void (Player::*)(PreparedQueryResult); friend Type Access(QuestLoadTag); };
template struct FixtureAccess<QuestLoadTag, &Player::_LoadQuestStatus>;
struct ObjectiveLoadTag { using Type = void (Player::*)(PreparedQueryResult); friend Type Access(ObjectiveLoadTag); };
template struct FixtureAccess<ObjectiveLoadTag, &Player::_LoadQuestStatusObjectives>;
static uint32 nextGuid = 5000;
static Position const pos{2400.65f,529.314f,748.996f,0};
struct TrialPlayer : LootPlayer
{
    TrialPlayer(Map& map, uint8 race = RACE_HUMAN, uint32 guid = 0) : LootPlayer(Session(),map,guid ? guid : ++nextGuid,pos)
    {
        SetName("Trial"+std::to_string(GetGUIDLow())); SetQuestStatus(44746,QUEST_STATUS_NONE);
        SetByteValue(UNIT_FIELD_BYTES_0,0,race); setFactionForRace(race);
        GetAchievementMgr()->m_canUpdateAchiev=1; // Login normally enables updates.
    }
    void Begin(uint32 id) { AddQuestAndCheckCompletion(sQuestDataStore->GetQuestTemplate(id),nullptr); }
};
struct TrialParty
{
    Group group;
    TrialParty(std::vector<std::unique_ptr<TrialPlayer>>& players)
    {
        group.*Access(GroupGuidTag{})=ObjectGuid::Create<HighGuid::Party>(++nextGuid,0);
        group.*Access(GroupFlagsTag{})=GROUP_FLAG_RAID; sGroupMgr->AddGroup(&group);
        for(auto& p:players) { Group::MemberSlot slot;slot.Guid=p->GetGUID();slot.Name=p->GetName();slot.Class=p->getClass();const_cast<Group::MemberSlotList&>(group.GetMemberSlots()).push_back(slot);p->SetGroup(&group,0); }
    }
    ~TrialParty() { while(auto ref=group.GetFirstMember())ref->getSource()->SetGroup(nullptr);sGroupMgr->RemoveGroup(&group); }
};
struct TrialInstance : InstanceScript
{
    using InstanceScript::InstanceScript;
    uint32 GetData(uint32 id) const override
    {return id==Data::DataMisc::OdynEventComplete1||id==Data::DataMisc::OdynEventComplete2?DONE:0;}
};
static void LoadTrialInputs()
{
    LoadLootInputs();
    auto objectives=WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID IN(44729,44868) ORDER BY QuestID,StorageIndex");
    Require(bool(objectives),"Missing Trial quest objectives");unsigned count=0;
    do {auto f=objectives->Fetch();auto q=const_cast<Quest*>(sQuestDataStore->GetQuestTemplate(f[1].GetUInt32()));Require(q,"Missing Trial quest");q->LoadQuestObjective(f);++count;}while(objectives->NextRow());
    Require(count==2,"Trial objective count changed");
    sAchievementMgr->LoadCriteriaList();sAchievementMgr->LoadAchievementReferenceList();sAchievementMgr->LoadAchievementCriteriaData();
    for(auto ids:{std::array<uint32,4>{44729,55330,55331,1958},std::array<uint32,4>{44868,55482,55483,2008}})
    {
        auto root=sAchievementMgr->GetCriteriaTree(ids[1]);auto leaf=sAchievementMgr->GetCriteriaTree(ids[2]);
        Require(root&&leaf&&leaf->Criteria&&root->Children.size()==1&&root->Children[0]==leaf,"Actual Trial tree loader failed");
        Require(leaf->Entry->Parent==ids[1]&&leaf->Criteria->Entry->Type==165&&leaf->Criteria->Entry->Asset==ids[3]&&sAchievementMgr->_criteriaTreeForQuest[ids[2]]==ids[0],"Native Trial catalog changed");
    }
}
static void Emit(InstanceMap& map,uint32 asset,Unit* source=nullptr)
{map.GetInstanceScript()->DoUpdateAchievementCriteria(CRITERIA_TYPE_COMPLETE_DUNGEON_ENCOUNTER,asset,0,0,source);}
static uint64 Counter(TrialPlayer& p,uint32 leaf)
{auto progress=p.GetAchievementMgr()->GetCriteriaProgressMap();auto it=progress->find(leaf);return it==progress->end()?0:it->second.Counter;}
static void CheckParty(InstanceMap& map,Map& elsewhere,uint32 id,uint32 root,uint32 leaf,uint32 encounter)
{
    std::vector<std::unique_ptr<TrialPlayer>> players;
    for(unsigned i=0;i<6;++i)players.emplace_back(new TrialPlayer(i==5?elsewhere:map,i%2?RACE_ORC:RACE_HUMAN));
    TrialParty party(players);
    for(unsigned i:{0u,1u,4u,5u})players[i]->Begin(id);
    players[3]->SetQuestStatus(id,QUEST_STATUS_REWARDED);
    players[4]->Begin(id);players[4]->SetQuestStatus(id,QUEST_STATUS_NONE);players[4]->SetQuestSlot(0,0);
    for(uint32 wrong:{0u,1809u,1962u,encounter==1958?2008u:1958u})Emit(map,wrong);
    for(auto& p:players)Require(!Counter(*p,leaf)&&!p->GetQuestObjectiveData(id,root),"Wrong encounter generated quest progress");
    Actor boss(&map,encounter==1958?114263:114537,++nextGuid,pos);Require(map.AddToMap(static_cast<Creature*>(&boss)),"Trial source map admission failed");
    Emit(map,encounter,&boss);Emit(map,encounter,&boss);
    for(unsigned i=0;i<players.size();++i)
    {
        auto& p=*players[i];bool eligible=i<2;
        Require(Counter(p,leaf)==(eligible?2:0),"Actual native achievement recipient/repeat counter failed");
        Tick(p,2000);
        Require(p.GetQuestObjectiveData(id,root)==(eligible?1:0),"Actual tree-to-quest notification failed");
        if(eligible)Require(p.GetQuestStatus(id)==QUEST_STATUS_COMPLETE,"Native delayed quest completion failed");
    }
    Emit(map,encounter,&boss);for(auto& p:players)Tick(*p,2000);
    for(unsigned i=0;i<2;++i)Require(players[i]->GetQuestObjectiveData(id,root)==1&&Counter(*players[i],leaf)==2,"Completed quest gained repeated progress");
    std::cout<<"PASS: actual instance/achievement/tree/delayed Player chain quest="<<id<<" diff="<<uint32(map.GetDifficultyID())<<" both factions; wrong encounters, absent/rewarded/abandoned/outside-map and repeated emission checked.\n";
}
static void CheckLifetime(InstanceMap& map,uint32 id,uint32 root,uint32 encounter)
{
    for(uint32 delay:{0u,200u})
    {
        TrialPlayer p(map);p.Begin(id);Emit(map,encounter);Tick(p,delay);
        WorldPacket wire(CMSG_QUEST_LOG_REMOVE_QUEST);wire<<uint8(p.FindQuestSlot(id));
        WorldPackets::Quest::QuestLogRemoveQuest packet(std::move(wire));packet.Read();
        p.GetSession()->HandleQuestLogRemoveQuest(packet);Require(p.GetQuestStatus(id)==QUEST_STATUS_NONE,"Native abandonment failed");p.Begin(id);
        Tick(p,2000);
        std::cout<<"LIFETIME quest="<<id<<" delay="<<delay<<" objective="<<p.GetQuestObjectiveData(id,root)<<" status="<<uint32(p.GetQuestStatus(id))<<std::endl;
#ifdef TRIAL_CRITERIA_BASELINE
        Require(p.GetQuestObjectiveData(id,root)==(delay?0:1)&&p.GetQuestStatus(id)==(delay?QUEST_STATUS_INCOMPLETE:QUEST_STATUS_COMPLETE),"Pinned stale achievement notification behavior changed");
#else
        Require(!p.GetQuestObjectiveData(id,root)&&p.GetQuestStatus(id)==QUEST_STATUS_INCOMPLETE,"Old achievement notification completed a reaccepted quest");
#endif
        Emit(map,encounter);Tick(p,2000);
        Require(p.GetQuestObjectiveData(id,root)==1&&p.GetQuestStatus(id)==QUEST_STATUS_COMPLETE,"Fresh encounter cannot complete reaccepted quest");
    }
    std::cout<<"PASS: actual achievement notification/quest lifetime checked (baseline reproduces stale notification); quest="<<id<<std::endl;
}
static void CheckSaved(InstanceMap& map,uint32 id,uint32 root,uint32 leaf,uint32 encounter)
{
    auto guid=++nextGuid;
    {
        TrialPlayer p(map,RACE_ORC,guid);p.Begin(id);Emit(map,encounter);Tick(p,2000);
        Require(p.GetQuestStatus(id)==QUEST_STATUS_COMPLETE,"Native pre-save completion failed");p.SaveToDB(true);FlushWrites();
    }
    {
        TrialPlayer p(map,RACE_ORC,guid);
        auto stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS);stmt->setUInt32(0,0);stmt->setUInt64(1,guid);(p.*Access(QuestLoadTag{}))(ReadPrepared(stmt));
        stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_OBJECTIVES);stmt->setUInt32(0,0);stmt->setUInt64(1,guid);(p.*Access(ObjectiveLoadTag{}))(ReadPrepared(stmt));
        stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_CRITERIAPROGRESS);stmt->setUInt64(0,guid);p.GetAchievementMgr()->LoadFromDB({},ReadPrepared(stmt));
        Require(p.GetQuestStatus(id)==QUEST_STATUS_COMPLETE&&p.GetQuestObjectiveData(id,root)==1&&Counter(p,leaf)==1,"Native saved quest/objective/achievement reload lost completion");
        Emit(map,encounter);Tick(p,2000);
        Require(p.GetQuestObjectiveData(id,root)==1&&Counter(p,leaf)==1,"Native saved completion gained duplicate progress");
    }
    std::cout<<"PASS: actual SaveToDB and prepared quest/objective/AchievementMgr reload preserve completion exactly once; quest="<<id<<std::endl;
}
static void CheckBossDispatch(InstanceMap& map,bool odyn,bool petKiller)
{
    uint32 id=odyn?44729:44868,root=odyn?55330:55482,leaf=odyn?55331:55483;
    auto instance=map.GetInstanceScript();
    // Seed the encounter's final phase and skip initial defender/trash spawning.
    // Boss callbacks themselves come from the actual final-link scripts.lib.
    instance->SetBossNumber(3);for(unsigned i=0;i<3;++i)instance->SetBossState(i,DONE);
    std::vector<std::unique_ptr<TrialPlayer>> players;
    players.emplace_back(new TrialPlayer(map));players.emplace_back(new TrialPlayer(map,RACE_ORC));TrialParty party(players);
    for(auto& p:players)p->Begin(id);
    Actor boss(&map,odyn?114263:114537,++nextGuid,pos);Require(map.AddToMap(static_cast<Creature*>(&boss)),"Actual Trial boss admission failed");
    Require(boss.AIM_Initialize(sScriptMgr->GetCreatureAI(&boss)),"Actual final-link boss AI failed");
    instance->SetBossState(odyn?0:2,IN_PROGRESS);
    LootPet pet(*players[0],map);Unit* killer=petKiller?static_cast<Unit*>(&pet):static_cast<Unit*>(players[0].get());
    if(odyn)
    {
        boss.AI()->DoAction(ACTION_1);boss.AI()->UpdateAI(7000);boss.CastStop();boss.AI()->UpdateAI(2000);boss.CastStop();
        Require(boss.AI()->GetData(Data::BossIDs::OdynID)==2,"Native Odyn phase2 setup failed");
        uint32 damage=UINT32_MAX;boss.AI()->DamageTakenAfterScaling(killer,damage,DIRECT_DAMAGE);
        Require(boss.IsAlive()&&instance->GetBossState(0)==DONE,"Native Odyn surrender failed");
        damage=UINT32_MAX;boss.AI()->DamageTakenAfterScaling(killer,damage,DIRECT_DAMAGE);Require(!damage,"Repeated Odyn surrender accepted damage");
    }
    else
    {
        // No tap/damage eligibility is seeded: this test exercises death/credit,
        // while the independent raid-fragment probe executes rewarded kills.
        killer->Kill(&boss,false);Require(boss.isDead(false)&&instance->GetBossState(2)==DONE,"Native Helya death failed");
    }
    for(auto& p:players){Tick(*p,2000);Require(p->GetQuestStatus(id)==QUEST_STATUS_COMPLETE&&p->GetQuestObjectiveData(id,root)==1&&Counter(*p,leaf)==1,"Actual boss callback lost party quest credit");}
    std::cout<<"PASS: final-link boss callback -> actual instance/achievement/tree/party quest completion; quest="<<id<<" pet="<<petKiller<<" diff="<<uint32(map.GetDifficultyID())<<std::endl;
}
#ifndef TRIAL_RAID_CRITERIA_SUPPORT_ONLY
static int Run(int argc,char** argv)
{
    InitializeNative(argc,argv);opcodeTable.Initialize();LoadTrialInputs();OpenScratch();
    sAchievementMgr->LoadRewards();sAchievementMgr->LoadCompletedAchievements();HashMapHolder<Player>::SetSize(256);
    sCreatureTextMgr->LoadCreatureTexts();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());AddSC_tov_boss_odyn();AddSC_boss_tov_helya();sScriptMgr->SwapScriptContext(true);
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1648,{}},{1220,{}}});
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1648,{}},{1220,{}}});
    {
        Map parent(1648,300,9999993,DIFFICULTY_NONE),elsewhere(1220,300,9999994,DIFFICULTY_NONE);
        for(auto diff:{DIFFICULTY_LFR_RAID,DIFFICULTY_NORMAL_RAID,DIFFICULTY_HEROIC_RAID,DIFFICULTY_MYTHIC_RAID})
        {
            InstanceMap map(1648,300,++nextGuid,diff,&parent);map.*Access(TrialInstanceTag{})=new TrialInstance(&map);
            CheckParty(map,elsewhere,44729,55330,55331,1958);CheckParty(map,elsewhere,44868,55482,55483,2008);
            CheckLifetime(map,44729,55330,1958);CheckLifetime(map,44868,55482,2008);
            CheckSaved(map,44729,55330,55331,1958);CheckSaved(map,44868,55482,55483,2008);
            for(bool pet:{false,true}){CheckBossDispatch(map,true,pet);CheckBossDispatch(map,false,pet);}
        }
    }
    FlushWrites();CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();return 0;
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{return Run(argc,argv);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
#endif
