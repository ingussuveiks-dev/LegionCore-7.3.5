// Native Player/Scenario achievement dispatch and real Unit::Kill call the
// exported complete production death hook. No client login or character save.
#define ROCKAWAY_NATIVE_SUPPORT_ONLY
#include "RockawayOffensiveNative.cpp"
#include "Conversation.h"
#include "ConversationData.h"
#include "MapInstanced.h"
#include "Pet.h"
#include "TemporarySummon.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "GroupMgr.h"
#include "LootMgr.h"
#include "GameObjectAI.h"
#include <filesystem>
#include <cstdlib>
#include "AltheaActor.inc"
struct CriteriaDataTag{using Type=AchievementCriteriaDataVector AchievementGlobalMgr::*;friend Type Access(CriteriaDataTag);};
template struct FixtureAccess<CriteriaDataTag,&AchievementGlobalMgr::m_criteriaDataVector>;
struct PartyGuidTag{using Type=ObjectGuid Group::*;friend Type Access(PartyGuidTag);};
template struct FixtureAccess<PartyGuidTag,&Group::m_guid>;
struct StoriesTag{using Type=HashMapHolder<Conversation>::MapTypeVector*;friend Type Access(StoriesTag);};
template struct FixtureAccess<StoriesTag,&HashMapHolder<Conversation>::_objectVector>;
struct LootNameTag{using Type=std::string LootStore::*;friend Type Access(LootNameTag);};
template struct FixtureAccess<LootNameTag,&LootStore::m_name>;
struct LootLoadTag{using Type=uint32 (LootStore::*)();friend Type Access(LootLoadTag);};
template struct FixtureAccess<LootLoadTag,&LootStore::LoadLootTable>;
struct CurrentAlthea:ScriptedAI{using ScriptedAI::ScriptedAI;
#include "AltheaVictory.inc"
#include "Althea.inc"
};
struct BeforeAlthea:ScriptedAI{using ScriptedAI::ScriptedAI;
#include "AltheaBeforeCredit.inc"
};
struct OriginalDoor:GameObjectAI{using GameObjectAI::GameObjectAI;
#include "AltheaDoor.inc"
};
struct CreditActor:Actor{
    using Actor::Actor;
    void AddToWorld()override{
        sObjectAccessor->AddObject(static_cast<Creature*>(this));Unit::AddToWorld();
        Require(AIM_Initialize(new CurrentAlthea(this)),"Native current death AI failed");
    }
};
struct CreditPet:Pet{
    CreditPet(Player& owner,Map& map):Pet(&owner,HUNTER_PET){
        Require(Create(200,&map,1,299,1),"Native pet create failed");setPetType(HUNTER_PET);
        SetOwnerGUID(owner.GetGUID());SetCreatorGUID(owner.GetGUID());Relocate(owner.GetPosition());setFaction(owner.getFaction());
        SetLevel(110);SetMaxHealth(1000000);SetHealth(1000000);Require(map.AddToMap(static_cast<Creature*>(this)),"Native pet admission failed");
    }
    void AddToWorld()override{sObjectAccessor->AddObject(static_cast<Pet*>(this));Unit::AddToWorld();Require(AIM_Initialize(new NullCreatureAI(this)),"Native pet AI failed");}
    ~CreditPet(){CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();}
};
struct CreditGuardian:Guardian{
    CreditGuardian(Player& owner,Map& map):Guardian(nullptr,&owner,false){
        auto p=owner.GetPosition();Require(Create(201,&map,1,299,0,ALLIANCE,p.GetPositionX(),p.GetPositionY(),p.GetPositionZ(),0),"Native guardian create failed");
        SetOwnerGUID(owner.GetGUID());SetCreatorGUID(owner.GetGUID());setFaction(owner.getFaction());SetLevel(110);SetMaxHealth(1000000);SetHealth(1000000);
        Require(map.AddToMap(static_cast<Creature*>(this)),"Native guardian admission failed");
    }
    void AddToWorld()override{sObjectAccessor->AddObject(static_cast<Creature*>(this));Unit::AddToWorld();Require(AIM_Initialize(new NullCreatureAI(this)),"Native guardian AI failed");}
    ~CreditGuardian(){CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();}
};
static uint64 Counter(Scenario* scenario){auto const* map=scenario->GetAchievementMgr().GetCriteriaProgressMap();auto found=map->find(48585);return found==map->end()?0:found->second.Counter;}
static void CleanStories(Map& map){
    // Conversation uses the vector accessor, while Player additionally uses the map.
    std::vector<Conversation*> stories;
    for(auto value:*Access(StoriesTag{}))if(value&&value->GetMap()==&map)stories.push_back(value);
    for(auto value:stories)value->Remove();map.RemoveAllObjectsInRemoveList();
}
static WorldSession* Session(){return new WorldSession(0,"Isolated Althea credit probe",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});}
struct CreditParty:Group{
    Player* first;Player* second;
    CreditParty(Player* a,Player* b):first(a),second(b){
        this->*Access(PartyGuidTag{})=ObjectGuid::Create<HighGuid::Party>(1000,0);
        for(auto player:{a,b}){Group::MemberSlot slot;slot.Guid=player->GetGUID();slot.Class=player->getClass();const_cast<Group::MemberSlotList&>(GetMemberSlots()).push_back(slot);player->SetGroup(this,0);}
        sGroupMgr->AddGroup(this);
    }
    ~CreditParty(){first->SetGroup(nullptr);second->SetGroup(nullptr);sGroupMgr->RemoveGroup(this);}
};
int main(int argc,char** argv){try{
    SetUnhandledExceptionFilter(CrashTrace);Require(argc==2,"Expected dbc directory");auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config,"Private config missing");
    auto directory=std::filesystem::path(argv[1]).lexically_normal();if(directory.filename().empty())directory=directory.parent_path();
    std::string runtime=directory.parent_path().parent_path().string()+'/';char* args[]={argv[0],config,runtime.data()};InitializeNative(3,args);
    LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo",""),1,1);CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo",""),1,1);
    Require(!LoginDatabase.Open()&&LoginDatabase.PrepareStatements()&&!CharacterDatabase.Open()&&CharacterDatabase.PrepareStatements(),"Native session schemas failed");
    // WorldSession's destructor updates account.online even without a socket.
    // Give both private connections an empty TEMP table before readonly mode;
    // native cleanup then cannot address a persistent account row.
    for(auto sql:{"CREATE TEMPORARY TABLE althea_account_seed LIKE account","CREATE TEMPORARY TABLE account LIKE althea_account_seed"}){LoginDatabase.DirectExecute(sql);LoginDatabase.Execute(sql);}
    LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");CharacterDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    for(bool login:{true,false}){auto row=login?LoginDatabase.Query("SELECT @@session.transaction_read_only"):CharacterDatabase.Query("SELECT @@session.transaction_read_only");Require(row&&row->Fetch()[0].GetBool(),"Native session pool is not readonly");}
    sObjectMgr->LoadInstanceTemplate();sObjectMgr->LoadScenarioData();sConversationDataStore->LoadConversationData();
    sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();
    auto lootRows=WorldDatabase.Query("SELECT COUNT(*) FROM creature_loot_template WHERE Entry=108218");Require(lootRows&&lootRows->Fetch()[0].GetUInt32(),"Actual Althea loot input missing");
    auto& lootName=LootTemplates_Creature.*Access(LootNameTag{});auto originalLootName=lootName;lootName+=" WHERE Entry=108218 ORDER BY Entry,Item";
    auto lootCount=(LootTemplates_Creature.*Access(LootLoadTag{}))();lootName=originalLootName;Require(lootCount==lootRows->Fetch()[0].GetUInt32()&&LootTemplates_Creature.HaveLootFor(108218),"Actual scoped Althea loot loader differs");
    auto rows=WorldDatabase.Query(
#include "NativeAltheaQuest.inc"
    );Require(bool(rows),"Native Althea quests absent");auto& quests=sQuestDataStore->*Access(QuestVectorTag{});quests.resize(50000);sQuestDataStore->*Access(QuestMaxTag{})=50000;
    do{auto quest=new Quest(rows->Fetch());quests[quest->GetQuestId()]=quest;const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[quest->GetQuestId()]=quest;}while(rows->NextRow());
    auto rules=WorldDatabase.Query("SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id=30473");Require(rules&&!rules->Fetch()[0].GetUInt32(),"Actual Althea criteria requires additional loader rules");
    sAchievementMgr->LoadCriteriaList();(sAchievementMgr->*Access(CriteriaDataTag{})).assign(MAX_CRITERIA_TREE,nullptr);opcodeTable.Initialize();
    for(uint32 id:{48584u,48586u,48588u}){auto tree=sCriteriaTreeStore.LookupEntry(id);Require(tree,"Native tree absent");std::cout<<"TREE "<<id<<" operator="<<unsigned(tree->Operator)<<" amount="<<tree->Amount<<" criteria="<<tree->CriteriaID<<'\n';}
    for(auto tree:sCriteriaTreeStore)if(tree->Parent==48584){auto criterion=sCriteriaStore.LookupEntry(tree->CriteriaID);std::cout<<"CHILD "<<tree->ID<<" operator="<<unsigned(tree->Operator)<<" amount="<<tree->Amount<<" criteria="<<tree->CriteriaID<<" asset="<<(criterion?criterion->Asset:0)<<'\n';}
    HashMapHolder<Player>::SetSize(1000);HashMapHolder<Pet>::SetSize(1000);HashMapHolder<Conversation>::SetSize(20000);
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1620,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1620,{}}});
    uint32 number=0;
    for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})for(uint32 mode=0;mode<13;++mode){
        MapInstanced parent(1620,300);InstanceMap map(1620,300,9999000+number++,DIFFICULTY_N_SCENARIO,&parent);
        Scenario* scenario=sScenarioMgr->AddScenario(&map,1123);scenario->SetCurrentStep(5);
        auto a=Session(),b=Session();
        {
            NativePlayer first(a,map,101,{-8607.5f,394.1f,110.255f,0}),second(b,map,102,first.GetPosition());
            for(auto player:{&first,&second}){player->SetByteValue(UNIT_FIELD_BYTES_0,0,race);player->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);player->setFactionForRace(race);player->SetQuestStatus(race==RACE_HUMAN?42504:42627,QUEST_STATUS_INCOMPLETE);player->SetQuestSlot(0,race==RACE_HUMAN?42504:42627);}
            CreditActor source(&map,108218,12001,first.GetPosition());Require(map.AddToMap(static_cast<Creature*>(&source)),"Native Althea source admission failed");
            Player* gm=source.FindNearestPlayer(100);Require(gm==&first||gm==&second,"Native nearby-player control missing");Player* owner=gm==&first?static_cast<Player*>(&second):static_cast<Player*>(&first);
            gm->SetGameMaster(true);
            std::unique_ptr<CreditParty> party;if(mode==10){gm->SetGameMaster(false);party=std::make_unique<CreditParty>(owner,gm);}
            if(mode!=9){OriginalDoor door(nullptr);Require(door.GossipHello(owner,true),"Original door use hook failed");auto const* progress=scenario->GetAchievementMgr().GetCriteriaProgressMap();Require(progress->count(48597)&&progress->at(48597).Counter==1&&Counter(scenario)==0&&scenario->GetCurrentStep()==5,"Original door51654 prerequisite differs");}
            std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;Unit* killer=owner;
            if(mode==2){pet=std::make_unique<CreditPet>(*owner,map);killer=pet.get();}
            if(mode==3){guardian=std::make_unique<CreditGuardian>(*owner,map);killer=guardian.get();}
            if(mode==6)owner->SetQuestStatus(race==RACE_HUMAN?42504:42627,QUEST_STATUS_NONE);
            if(mode==7)scenario->SetCurrentStep(6);
            if(mode==8)owner->Unit::setDeathState(CORPSE);
            if(mode==11){owner->SetQuestStatus(race==RACE_HUMAN?42504:42627,QUEST_STATUS_NONE);Require(!static_cast<CurrentAlthea*>(source.AI())->GetVictoryPlayer(owner),"Abandoned attempt remained eligible");owner->SetQuestStatus(race==RACE_HUMAN?42504:42627,QUEST_STATUS_INCOMPLETE);}
            if(mode==12)owner->SetQuestStatus(race==RACE_HUMAN?42504:42627,QUEST_STATUS_COMPLETE);
            auto selected=static_cast<CurrentAlthea*>(source.AI())->GetVictoryPlayer(owner);
            Require(selected==((mode==6||mode==7)?nullptr:owner),"Native owner-first eligible reference differs");
            std::cout<<"TRACE native Althea race="<<unsigned(race)<<" mode="<<mode<<std::endl;
            if(mode==0){source.Unit::setDeathState(CORPSE);BeforeAlthea before(&source);before.JustDied(owner);Require(Counter(scenario)==0&&scenario->GetCurrentStep()==5,"Old nearby-GM refusal no longer reproduces");std::cout<<"COUNTERFACTUAL original nearest GM blocks native51613 despite an eligible Rogue in the same instance\n";}
            else if(mode==4){source.Unit::setDeathState(CORPSE);source.AI()->JustDied(nullptr);}
            else if(mode==5){Actor ally(&map,97167,12002,owner->GetPosition());Require(map.AddToMap(static_cast<Creature*>(&ally)),"Native NPC killer admission failed");ally.Kill(&source,false);}
            else if(mode==1){source.SetLootRecipient(owner);source.LowerPlayerDamageReq(uint32(source.GetMaxHealth()));owner->DealDamage(&source,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);}
            else {source.SetLootRecipient(owner);source.LowerPlayerDamageReq(uint32(source.GetMaxHealth()));killer->Kill(&source,false);killer->Kill(&source,false);}
            if(mode!=0){uint64 expected=mode==6||mode==7?0:1;std::cout<<"TRACE result counter="<<Counter(scenario)<<" stage="<<scenario->GetCurrentStep()<<" root-complete="<<scenario->GetAchievementMgr().IsCompletedScenarioTree(sCriteriaTreeStore.LookupEntry(48584))<<std::endl;Require(Counter(scenario)==expected,"Native Althea victory counter differs");Require(scenario->GetCurrentStep()==(mode==9?5u:(expected?6u:(mode==7?6u:5u))),"Native Althea stage differs");if(expected){auto progress=scenario->GetAchievementMgr().GetCriteriaProgressMap()->at(48585);Require(progress.PlayerGUID.IsEmpty(),"Native shared scenario unexpectedly acquired personal attribution");}if(mode==9){source.AI()->JustDied(owner);Require(Counter(scenario)==1&&!scenario->GetAchievementMgr().GetCriteriaProgressMap()->count(48597),"Repeated victory bypassed the independent door prerequisite");OriginalDoor door(nullptr);Require(door.GossipHello(owner,true)&&Counter(scenario)==1&&scenario->GetCurrentStep()==6,"Later original door event failed to finish the independently retained stage");}}
            if(party){bool found=false;for(auto story:*Access(StoriesTag{}))if(story&&story->GetMap()==&map&&story->GetEntry()==2431){Require(story->GetCasterGUID()==owner->GetGUID()&&story->IsInPersonnalVisibilityList(owner->GetGUID())&&story->IsInPersonnalVisibilityList(gm->GetGUID()),"Native group conversation recipient visibility changed");found=true;}Require(found,"Native group story missing");}
            CleanStories(map);Require(objectCountInWorld[uint8(HighGuid::Conversation)]==0,"Native story allocation leaked");
        }
        a->SetPlayer(nullptr);b->SetPlayer(nullptr);delete a;delete b;sScenarioMgr->RemoveScenario(map.GetInstanceId());
    }
    std::cout<<"PASS "<<number<<" native Althea cases: original GM counterfactual, both faction quests, actual Player/pet/guardian/NPC Kill, scaled one-hit damage, repeat Kill/callback, null fallback, abandoned/reaccepted/complete quest, wrong stage and dead owner; actual party/shared48585/51613 amount1 progresses5->6 only with independent original door48597/51654, in either event order. Native owner reference, shared empty PlayerGUID and original party conversation visibility retained. Earlier progression and direct door/death hook invocation are controlled; no physical door/client/full1123 claim\n";std::cout.flush();
    auto accountRows=LoginDatabase.Query("SELECT COUNT(*) FROM account");Require(accountRows&&!accountRows->Fetch()[0].GetUInt32(),"Private TEMP account changed unexpectedly");
    bool asyncChecked=false;auto receipt=LoginDatabase.AsyncQuery("SELECT COUNT(*) FROM account");
    receipt.WithCallback([&](QueryResult rows){Require(rows&&!rows->Fetch()[0].GetUInt32(),"Async TEMP account changed unexpectedly");asyncChecked=true;});
    auto deadline=GetTickCount64()+5000;while(!asyncChecked&&GetTickCount64()<deadline){receipt.InvokeIfReady();Sleep(1);}Require(asyncChecked,"Native session cleanup did not drain");
    CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();
}catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}}
