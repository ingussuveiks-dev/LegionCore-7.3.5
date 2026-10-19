// Actual instance factory, save serializer/loader, native DB-spawn lifecycle
// and native summon/AI execution. No modeled victory actor creation.
#define TRIAL_RAID_TURNIN_SUPPORT_ONLY
#include "TrialRaidTurnInNative.cpp"
#include "InstanceSaveMgr.h"
#include "ConversationData.h"
#include "AreaTriggerData.h"
#include "LFGMgr.h"
#include "MapInstanced.h"
void AddSC_instance_trial_of_valor();
void AddSC_trial_of_valor();

static void LoadInstanceInputs()
{
    LoadTurnInInputs();
    sSpellMgr->LoadSpellBonusess();sSpellMgr->LoadSpellThreats();
    sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();sObjectMgr->LoadInstanceTemplate();sObjectMgr->LoadTempSummons();sObjectMgr->LoadCreatureTemplateAddons();
    sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sObjectMgr->LoadInstanceEncounters();
    sSpellMgr->LoadSpellTargetPositions();sCreatureTextMgr->LoadCreatureTexts();sConversationDataStore->LoadConversationData();
    auto rows=WorldDatabase.Query("SELECT guid,id,map,zoneId,areaId,modelid,equipment_id,position_x,position_y,position_z,orientation,spawntimesecs,spawndist,currentwaypoint,curhealth,curmana,MovementType,spawnMask,phaseMask,npcflag,npcflag2,unit_flags,unit_flags3,dynamicflags,isActive,PhaseId,AiID,MovementID,MeleeID,skipClone,personal_size,isTeemingSpawn FROM creature WHERE guid IN(505062,505276)");
    Require(bool(rows),"Missing native Trial DB spawns");unsigned count=0;
    do
    {
        auto f=rows->Fetch();auto& d=sObjectMgr->NewOrExistCreatureData(f[0].GetUInt64());d.guid=f[0].GetUInt64();d.id=f[1].GetUInt32();d.mapid=f[2].GetUInt16();d.zoneId=f[3].GetUInt16();d.areaId=f[4].GetUInt16();d.displayid=f[5].GetUInt32();d.equipmentId=f[6].GetInt8();d.posX=f[7].GetFloat();d.posY=f[8].GetFloat();d.posZ=f[9].GetFloat();d.orientation=f[10].GetFloat();d.spawntimesecs=f[11].GetUInt32();d.spawndist=f[12].GetFloat();d.currentwaypoint=f[13].GetUInt32();d.curhealth=f[14].GetUInt64();d.curmana=f[15].GetUInt32();d.movementType=f[16].GetUInt8();d.spawnMask=f[17].GetUInt64();d.phaseMask=f[18].GetUInt32();d.npcflag=f[19].GetUInt32();d.npcflag2=f[20].GetUInt32();d.unit_flags=f[21].GetUInt32();d.unit_flags3=f[22].GetUInt32();d.dynamicflags=f[23].GetUInt32();d.isActive=f[24].GetBool();Require(f[25].GetString().empty(),"Load new native phase IDs before testing");d.AiID=f[26].GetUInt32();d.MovementID=f[27].GetUInt32();d.MeleeID=f[28].GetUInt32();d.skipClone=f[29].GetBool();d.personalSize=f[30].GetFloat();d.isTeemingSpawn=f[31].GetBool();++count;
    }while(rows->NextRow());
    auto addons=WorldDatabase.Query("SELECT COUNT(*) FROM creature_addon WHERE guid IN(505062,505276)");Require(count==2&&addons&&!addons->Fetch()[0].GetUInt32(),"Load new native DB spawn addons before testing");
}
static std::vector<Creature*> Actors(Map& map,uint32 entry=0)
{
    std::vector<Creature*> result;
    auto bound=std::max<uint64>(nextGuid+1,sObjectMgr->GetGenerator<HighGuid::Creature>()->GetNextAfterMaxUsed());
    for(uint64 guid=1;guid<bound;++guid)if(auto c=HashMapHolder<Creature>::FindLow(guid))if(c->IsInWorld()&&c->GetMap()==&map&&(!entry||c->GetEntry()==entry))result.push_back(c);
    return result;
}
static Creature* LoadBoss(InstanceMap& map,uint64 guid)
{
    auto boss=new Creature();Require(boss->LoadCreatureFromDB(guid,&map),"Native Trial LoadCreatureFromDB failed");return boss;
}
static std::set<ObjectGuid> VictoryGuids(InstanceMap& map)
{
    std::set<ObjectGuid> result;
    for(auto c:Actors(map))switch(c->GetEntry())
    {case 116270:case 116268:case 116269:case 116271:case 101459:case 116760:case 116761:case 116818:case 116830:case 116763:case 116762:result.insert(c->GetGUID());break;}
    return result;
}
static void Counts(InstanceMap& map,unsigned copies)
{
    for(uint32 entry:{116270u,116268u,116269u,116271u,101459u})Require(Actors(map,entry).size()==copies,"Native Odyn victory actors duplicated/missing");
    for(auto pair:{std::array<uint32,2>{116760,1},{116761,1},{116818,1},{116830,1},{116763,3},{116762,2}})
    {auto actual=Actors(map,pair[0]).size();std::cout<<"ACTORS entry="<<pair[0]<<" count="<<actual<<std::endl;Require(actual==pair[1]*copies,"Native Helya victory actors duplicated/missing");}
}
static void PumpInstance(InstanceMap& map,uint32 duration)
{
    for(uint32 elapsed=0;elapsed<duration;elapsed+=10)
    {for(auto c:Actors(map))c->Update(10);map.MoveAllCreaturesInMoveList();map.RemoveAllObjectsInRemoveList();}
}
static void CheckActualTurnIns(InstanceMap& map,Creature& odyn,Creature& afterHelya)
{
    for(auto race:{RACE_HUMAN,RACE_ORC})
    {
        TrialPlayer p(map,race);const_cast<RewardedQuestSet&>(p.getRewardedQuests()).insert(44721);(p.*Access(RewardSaveTag{}))[44721]=QUEST_DEFAULT_SAVE_TYPE;
        p.Begin(44729);Emit(map,1958);Tick(p,2000);p.Relocate(odyn.GetPosition());p.SaveToDB(true);FlushWrites();
        Require(p.CanInteractWithQuestGiver(&odyn),"Actual restored Odyn interaction denied");
        p.PrepareQuestMenu(odyn.GetGUID());Require(p.PlayerTalkClass->GetQuestMenu().HasItem(44729),"Actual restored Odyn turn-in menu missing");
        WorldPacket wire(CMSG_QUEST_GIVER_CHOOSE_REWARD);wire<<odyn.GetGUID()<<int32(44729)<<int32(0);WorldPackets::Quest::QuestGiverChooseReward packet(std::move(wire));packet.Read();p.GetSession()->HandleQuestGiverChooseReward(packet);Tick(p,200);FlushWrites();
        Require(p.GetQuestRewardStatus(44729)&&p.GetItemCount(142533)==1,"Actual restored Odyn reward failed");
        WorldPacket accept(CMSG_QUEST_GIVER_ACCEPT_QUEST);accept<<odyn.GetGUID()<<int32(44868);accept.WriteBit(false);accept.FlushBits();WorldPackets::Quest::QuestGiverAcceptQuest next(std::move(accept));next.Read();p.GetSession()->HandleQuestGiverAcceptQuest(next);Emit(map,2008);Tick(p,2000);
        p.Relocate(afterHelya.GetPosition());Require(p.CanInteractWithQuestGiver(&afterHelya),"Actual post-Helya Odyn interaction denied");
        WorldPacket final(CMSG_QUEST_GIVER_CHOOSE_REWARD);final<<afterHelya.GetGUID()<<int32(44868)<<int32(0);WorldPackets::Quest::QuestGiverChooseReward finish(std::move(final));finish.Read();p.GetSession()->HandleQuestGiverChooseReward(finish);Tick(p,200);FlushWrites();
        Require(p.GetQuestRewardStatus(44868)&&p.GetItemCount(142534)==1,"Actual post-Helya Odyn reward failed");
    }
}
struct CompletedInstance { uint32 id;std::string data; };
using ExitCheck = void (*)(InstanceMap&, Creature&);
static CompletedInstance WinCase(Map& parent,Difficulty diff,ExitCheck exitCheck=nullptr)
{
    auto id=++nextGuid;InstanceMap map(1648,300,id,diff,&parent);
    // Guarm/defender completion is setup; execute the real Odyn surrender and
    // Helya death against the actual instance rather than a base-instance stub.
    InstanceSave partial(1648,id,diff,0,"T O V 0 3 0 0 0",GameTime::GetGameTime()+WEEK,false);
    map.CreateInstanceData(&partial);auto odyn=LoadBoss(map,505062);auto helya=LoadBoss(map,505276);
    Require(Actors(map,116270).empty()&&Actors(map,116760).empty()&&helya->IsAlive(),"Victory actors spawned before victory");
    auto instance=map.GetInstanceScript();instance->SetData(Data::DataMisc::OdynEventComplete1,DONE);instance->SetData(Data::DataMisc::OdynEventComplete2,DONE);instance->SetBossState(0,IN_PROGRESS);
    odyn->AI()->DoAction(ACTION_1);odyn->AI()->UpdateAI(7000);odyn->CastStop();odyn->AI()->UpdateAI(2000);odyn->CastStop();
    Require(odyn->AI()->GetData(Data::BossIDs::OdynID)==2,"Real-instance Odyn phase2 setup failed");
    odyn->SetHealth(odyn->CountPctFromMaxHealth(9));uint32 damage=1;odyn->AI()->DamageTakenAfterScaling(odyn,damage,DIRECT_DAMAGE);
    Require(instance->GetBossState(0)==DONE&&odyn->IsAlive()&&odyn->HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_QUESTGIVER),"Actual Odyn surrender lost victory or questgiver flags");
    map.GetInstanceScript()->SetBossState(2,IN_PROGRESS);helya->Kill(helya,false);
    Require(map.GetInstanceScript()->GetBossState(2)==DONE&&helya->isDead(false),"Actual Helya death failed instance victory");Counts(map,1);
    PumpInstance(map,15000);auto giver=Actors(map,116760);Require(giver.size()==1,"Native Helya victory questgiver missing");
    std::cout<<"LANDING z="<<giver[0]->GetPositionZ()<<std::endl;
    Require(giver[0]->GetExactDist(498.595f,640.212f,1.8432f)<2,"Native post-Helya Odyn arrival did not land");
    Require(odyn->GetExactDist(2400.65f,529.314f,748.996f)<2,"Actual surrendered Odyn did not return to arena");
    CheckActualTurnIns(map,*odyn,*giver[0]);
    if(exitCheck)exitCheck(map,*giver[0]);
    auto saved=map.GetInstanceScript()->GetSaveData();FlushWrites();map.UnloadAll();
    std::cout<<"PASS: actual Helya Unit::Kill -> instance DONE -> native victory summons/arrival and both quest turn-ins; diff="<<uint32(diff)<<std::endl;
    return {id,saved};
}
static void SavedCase(Map& parent,Difficulty diff,CompletedInstance const& completed,bool helyaFirst,ExitCheck exitCheck=nullptr)
{
    auto id=completed.id;
    InstanceMap map(1648,300,id,diff,&parent);
    InstanceSave save(1648,id,diff,0,completed.data,GameTime::GetGameTime()+WEEK,false);
    map.CreateInstanceData(&save);auto instance=map.GetInstanceScript();Require(instance&&instance->GetBossState(0)==DONE&&instance->GetBossState(2)==DONE,"Actual saved Trial factory/Load failed");
    // Read the respawn record actually saved by the preceding native kill.
    map.LoadRespawnTimes();Require(map.GetCreatureRespawnTime(505276)>GameTime::GetGameTime(),"Actual dead-boss respawn record missing");
    Creature* odyn;Creature* helya;
    if(helyaFirst){helya=LoadBoss(map,505276);odyn=LoadBoss(map,505062);}else{odyn=LoadBoss(map,505062);helya=LoadBoss(map,505276);}
    Counts(map,1);Require(odyn->HasFlag(UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_QUESTGIVER)&&instance->GetBossState(2)==DONE,"Saved boss initialization lost DONE or questgiver flags");
    Require(odyn->GetExactDist(2399.73f,528.816f,748.997f)<1,"Saved Odyn not restored to arena");
    PumpInstance(map,15000);auto giver=Actors(map,116760);Require(!giver.empty(),"Saved victory questgiver missing");CheckActualTurnIns(map,*odyn,*giver[0]);
    if(exitCheck)exitCheck(map,*giver[0]);
    auto original=VictoryGuids(map);instance->Load(completed.data.c_str());Counts(map,1);
    helya->RemoveFromWorld();helya->AddToWorld();odyn->RemoveFromWorld();odyn->AddToWorld();
#ifdef TRIAL_INSTANCE_BASELINE
    Counts(map,2);
#else
    Counts(map,1);
    Require(VictoryGuids(map)==original,"Repeated boss creation replaced existing victory actors");
    // Losing one actor must replace only that slot on the next boss create.
    auto lost=Actors(map,116760).front()->ToTempSummon();Require(lost,"Victory questgiver is not a native summon");original.erase(lost->GetGUID());lost->UnSummon();map.RemoveAllObjectsInRemoveList();
    helya->RemoveFromWorld();helya->AddToWorld();Counts(map,1);
    auto repaired=VictoryGuids(map);Require(repaired.size()==original.size()+1&&std::includes(repaired.begin(),repaired.end(),original.begin(),original.end()),"Missing-actor recovery replaced unrelated companions");
#endif
    std::cout<<"PASS: actual Trial saved factory/Load, native DB spawns, restored Odyn and repeated Creature world lifecycle; diff="<<uint32(diff)<<" helyaFirst="<<helyaFirst<<std::endl;
    map.UnloadAll();
}
#ifndef TRIAL_RAID_INSTANCE_SUPPORT_ONLY
static int RunInstance(int argc,char** argv)
{
    InitializeNative(argc,argv);opcodeTable.Initialize();LoadInstanceInputs();OpenScratch();sAchievementMgr->LoadRewards();sAchievementMgr->LoadCompletedAchievements();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());AddSC_tov_boss_odyn();AddSC_boss_tov_helya();AddSC_trial_of_valor();AddSC_instance_trial_of_valor();sScriptMgr->SwapScriptContext(true);
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1648,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1648,{}}});
    {MapInstanced parent(1648,300);for(auto diff:{DIFFICULTY_LFR_RAID,DIFFICULTY_NORMAL_RAID,DIFFICULTY_HEROIC_RAID,DIFFICULTY_MYTHIC_RAID}){auto completed=WinCase(parent,diff);for(bool first:{false,true})SavedCase(parent,diff,completed,first);}}
    FlushWrites();CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();return 0;
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{return RunInstance(argc,argv);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
#endif
