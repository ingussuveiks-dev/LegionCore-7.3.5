// Real Guarm AI, DB doors, native combat/evade/death and persisted checkpoints.
#define TRIAL_RAID_INSTANCE_SUPPORT_ONLY
#include "TrialRaidInstanceNative.cpp"
#include "MapManager.h"
#include "GameObject.h"
#include "Vehicle.h"
#include "ItemEnchantmentMgr.h"
#include "GossipData.h"
#include "NPCPackets.h"
#include "MovementPackets.h"
#include "EventObjectData.h"
void AddSC_boss_garm();
static uint64 const doorIds[]={545004,545028,545027};
static void LoadGarmInputs(bool helya=false)
{
    LoadInstanceInputs(true);sObjectMgr->LoadCreatureAIInstance();sSpellMgr->LoadSpellProcEvents();sSpellMgr->LoadSpellProcs();
    sObjectMgr->LoadNPCText();sQuestDataStore->LoadPointsOfInterest();sGossipDataStore->LoadGossipMenu();sGossipDataStore->LoadGossipMenuItems();sEventObjectDataStore->LoadEventObjectTemplates();
    auto conditions=WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId IN(14,15) AND SourceGroup=20502) OR (SourceTypeOrReferenceId=13 AND SourceEntry=231166) OR (SourceTypeOrReferenceId=13 AND SourceEntry IN(724,725))");Require(conditions&&!conditions->Fetch()[0].GetUInt32(),"Load newly added native travel conditions before testing");
    auto triggers=WorldDatabase.Query("SELECT guid,id,map,zoneId,areaId,position_x,position_y,position_z,orientation,spawnMask,phaseMask,PhaseId FROM eventobject WHERE guid IN(410223,410224)");Require(bool(triggers),"Native Helheim triggers missing");unsigned triggerCount=0;
    do {auto f=triggers->Fetch();auto& d=sEventObjectDataStore->NewOrExistEventObjectData(f[0].GetUInt64());d.guid=f[0].GetUInt64();d.id=f[1].GetUInt32();d.mapid=f[2].GetUInt16();d.zoneId=f[3].GetUInt16();d.areaId=f[4].GetUInt16();d.Pos.Relocate(f[5].GetFloat(),f[6].GetFloat(),f[7].GetFloat(),f[8].GetFloat());d.spawnMask=f[9].GetUInt64();d.phaseMask=f[10].GetUInt32();Require(f[11].GetString().empty()&&d.mapid==1648,"Native Helheim trigger phase/map changed");sObjectMgr->AddEventObjectToGrid(d.guid,&d);++triggerCount;}while(triggers->NextRow());Require(triggerCount==2,"Native Helheim trigger count changed");
    LoadRandomEnchantmentsTable();sObjectMgr->LoadPersonalLootTemplate();auto& name=LootTemplates_Creature.*Access(LootTableNameTag{});auto original=name;name+=helya?" WHERE Entry IN(114323,114537)":" WHERE Entry=114323";Require((LootTemplates_Creature.*Access(LoadLootTableTag{}))()>0,"Native raid loot loader empty");name=original;
    auto rows=WorldDatabase.Query("SELECT guid,id,map,zoneId,areaId,spawnMask,phaseMask,PhaseId,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3,spawntimesecs,animprogress,AiID,state,isActive,personal_size FROM gameobject WHERE guid IN(545004,545027,545028)");
    Require(bool(rows),"Missing Trial DB doors");unsigned count=0;
    do
    {
        auto f=rows->Fetch();auto& d=sObjectMgr->NewGOData(f[0].GetUInt64());d.guid=f[0].GetUInt64();d.id=f[1].GetUInt32();d.mapid=f[2].GetUInt16();d.zoneId=f[3].GetUInt16();d.areaId=f[4].GetUInt16();d.spawnMask=f[5].GetUInt64();d.phaseMask=f[6].GetUInt32();Require(f[7].GetString().empty(),"Load new door phase IDs before testing");
        d.posX=f[8].GetFloat();d.posY=f[9].GetFloat();d.posZ=f[10].GetFloat();d.orientation=f[11].GetFloat();d.rotation=G3D::Quat(f[12].GetFloat(),f[13].GetFloat(),f[14].GetFloat(),f[15].GetFloat());d.spawntimesecs=f[16].GetInt32();d.animprogress=f[17].GetUInt8();d.AiID=f[18].GetUInt32();d.go_state=GOState(f[19].GetUInt8());d.isActive=f[20].GetBool();d.personalSize=f[21].GetFloat();Require(d.mapid==1648&&d.spawnMask==245760,"Trial door map/difficulties changed");++count;
    }while(rows->NextRow());Require(count==3,"Trial door spawn count changed");
    auto encounter=sDungeonEncounterStore.LookupEntry(1962);Require(encounter&&encounter->MapID==1648&&encounter->Bit==1,"Native Guarm encounter catalog changed");
}
static std::array<GameObject*,3> LoadDoors(InstanceMap& map)
{
    std::array<GameObject*,3> result{};for(unsigned i=0;i<3;++i){auto door=new GameObject();Require(door->LoadGameObjectFromDB(doorIds[i],&map,true),"Actual DB door initialization failed");result[i]=door;}return result;
}
static void Doors(std::array<GameObject*,3> const& doors,bool odyn,bool guarm,bool passage)
{
    std::array<bool,3> expected{{odyn,guarm,passage}};for(unsigned i=0;i<3;++i)Require(doors[i]->GetGoState()==(expected[i]?GO_STATE_ACTIVE:GO_STATE_READY),"Native Trial door state mismatch");
}
struct GarmPlayer:TrialPlayer
{
    using TrialPlayer::TrialPlayer;
    ~GarmPlayer(){for(auto diff:{DIFFICULTY_NORMAL_RAID,DIFFICULTY_HEROIC_RAID,DIFFICULTY_MYTHIC_RAID,DIFFICULTY_LFR_RAID})while(!GetBoundInstances(diff).empty()){auto b=GetBoundInstances(diff).begin();UnbindInstance(b->first,b->second.save->GetDifficultyID(),true);}}
};
static void Heads(InstanceMap& map,Creature& guarm)
{
    for(unsigned i=0;i<3;++i){auto entry=114341u+(i?i+1:0);auto actors=Actors(map,entry);Require(actors.size()==1&&guarm.GetVehicleKit()->GetPassenger(i)==actors[0]&&actors[0]->GetVehicleBase()==&guarm,"Guarm head/passenger missing or duplicated");}
}
static void ClearCombat(GarmPlayer& p,Creature& guarm)
{
    guarm.AI()->EnterEvadeMode();PumpInstance(*p.GetMap()->ToInstanceMap(),3000);p.CombatStop(true);
}
static void PlayerLookup(GarmPlayer& p,char const* stage)
{Require(ObjectAccessor::FindPlayer(p.GetGUID())==&p,(std::string("Fixture player missing from native lookup: ")+stage).c_str());}
static void AckTravel(GarmPlayer& p,float x,float y,float z)
{
    Require(p.IsBeingTeleportedNear()&&p.GetTeleportDest().GetExactDist(x,y,z)<0.02f,"Native travel destination mismatch");WorldPacket wire(CMSG_MOVE_TELEPORT_ACK);wire<<p.GetGUID()<<int32(0)<<int32(0);WorldPackets::Movement::MoveTeleportAck ack(std::move(wire));ack.Read();p.GetSession()->HandleMoveTeleportAck(ack);Require(!p.IsBeingTeleportedNear()&&p.GetExactDist(x,y,z)<0.02f,"Native travel ACK did not arrive");
}
static void Travel(GarmPlayer& p,Creature& odyn)
{
    auto map=p.GetMap();Tick(p,200);p.CombatStop(true);map->PlayerRelocation(&p,odyn.GetPositionX(),odyn.GetPositionY(),odyn.GetPositionZ(),odyn.GetOrientation());
    auto hello=[&](){WorldPacket wire(CMSG_TALK_TO_GOSSIP);wire<<odyn.GetGUID();WorldPackets::NPC::Hello packet(std::move(wire));packet.Read();p.GetSession()->HandleGossipHelloOpcode(packet);auto& menu=p.PlayerTalkClass->GetGossipMenu();Require(menu.GetMenuId()==20502&&menu.GetItem(0),"Native post-Odyn travel menu missing");};
    auto choose=[&](uint32 menu,uint32 index){WorldPacket wire(CMSG_GOSSIP_SELECT_OPTION);wire<<odyn.GetGUID()<<menu<<index;wire.WriteBits(0,8);wire.FlushBits();WorldPackets::NPC::GossipSelectOption packet(std::move(wire));packet.Read();p.GetSession()->HandleGossipSelectOption(packet);Tick(p,1000);};
    hello();
#ifdef TRIAL_GARM_BASELINE
    choose(20503,0);Require(p.IsBeingTeleportedNear(),"Pinned foreign Sky Jump menu acceptance changed");std::cout<<"BASELINE: foreign menu20503 starts Odyn Sky Jump20502"<<std::endl;
#else
    p.PlayerTalkClass->ClearMenus();choose(20502,0);Require(!p.IsBeingTeleportedNear(),"Sky Jump accepted an absent prepared menu");hello();choose(20503,0);Require(!p.IsBeingTeleportedNear(),"Sky Jump accepted a foreign menu");choose(20502,1);Require(!p.IsBeingTeleportedNear(),"Sky Jump accepted absent option");choose(20502,0);Require(p.PlayerTalkClass->GetGossipMenu().Empty(),"Sky Jump retained a consumed menu");
#endif
    auto target=sSpellMgr->GetSpellTargetPosition(231166);Require(target&&target->target_mapId==1648,"Native Sky Jump destination missing");AckTravel(p,target->target_X,target->target_Y,target->target_Z);
    auto trigger=sEventObjectDataStore->GetEventObjectData(410224);map->LoadGrid(trigger->Pos.GetPositionX(),trigger->Pos.GetPositionY());EventObject* eo=nullptr;for(uint64 guid=1;guid<sObjectMgr->GetGenerator<HighGuid::EventObject>()->GetNextAfterMaxUsed();++guid)if(auto candidate=HashMapHolder<EventObject>::FindLow(guid))if(candidate->IsInWorld()&&candidate->GetMap()==map&&candidate->GetDBTableGUIDLow()==410224)eo=candidate;Require(eo,"Native grid-loaded Helheim entry trigger missing");auto dest=sWorldSafeLocsStore.LookupEntry(5827);Require(dest&&dest->MapID==1648,"Native Helheim location missing");
    map->PlayerRelocation(&p,eo->GetPositionX(),eo->GetPositionY(),eo->GetPositionZ(),eo->GetOrientation());eo->Update(1);AckTravel(p,dest->Loc.X,dest->Loc.Y,dest->Loc.Z);
    std::cout<<"PASS: actual Odyn gossip/Sky Jump/near ACK -> DB Helheim trigger/native location5827 "<<dest->Loc.X<<","<<dest->Loc.Y<<","<<dest->Loc.Z<<std::endl;
}
struct GarmGroup
{
    Group group;
    GarmGroup(GarmPlayer& p,Difficulty diff){sWorld->AddCharacterInfo(p.GetGUID(),0,p.GetName(),GENDER_MALE,p.getRace(),p.getClass(),p.getLevel());Require(group.Create(&p,0,false),"Native Trial group creation failed");sGroupMgr->AddGroup(&group);group.SetRaidDifficultyID(diff);}
    ~GarmGroup(){for(auto diff:{DIFFICULTY_NORMAL_RAID,DIFFICULTY_HEROIC_RAID,DIFFICULTY_MYTHIC_RAID,DIFFICULTY_LFR_RAID})while(!group.GetBoundInstances(diff).empty()){auto b=group.GetBoundInstances(diff).begin();group.UnbindInstance(b->first,b->second.save->GetDifficultyID(),true);}while(auto ref=group.GetFirstMember())ref->getSource()->SetGroup(nullptr);sGroupMgr->RegisterGroupDbStoreId(group.GetDbStoreId(),nullptr);sGroupMgr->RemoveGroup(&group);}
};
struct GarmCheckpoint{std::string data;uint32 mask;};
static GarmCheckpoint Checkpoint(InstanceMap& map,Group& group,uint32 mask)
{
    auto script=map.GetInstanceScript();FlushWrites();auto row=CharacterDatabase.Query(Trinity::StringFormat("SELECT data,completedEncounters FROM group_instance WHERE guid=%u AND instance=%u",group.GetDbStoreId(),map.GetInstanceId()).c_str());
    std::cout<<"CHECKPOINT diff="<<uint32(map.GetDifficultyID())<<" live="<<script->GetSaveData()<<" liveMask="<<script->GetCompletedEncounterMask()<<" row="<<bool(row)<<" sql="<<(row?row->Fetch()[0].GetString():"missing")<<" sqlMask="<<(row?row->Fetch()[1].GetUInt32():0)<<" expected="<<mask<<std::endl;
    Require(row&&row->Fetch()[0].GetString()==script->GetSaveData()&&row->Fetch()[1].GetUInt32()==mask,"Native victory SQL checkpoint/mask mismatch");return {row->Fetch()[0].GetString(),row->Fetch()[1].GetUInt32()};
}
using HelyaProgressionCheck=void (*)(GarmPlayer&,InstanceMap&,Creature&,Group&);
static void GarmCase(Map& staging,MapInstanced& parent,Difficulty diff,uint8 race,bool petKill,HelyaProgressionCheck check=nullptr)
{
    GarmPlayer p(staging,race);PlayerLookup(p,"created");p.SaveToDB(true);FlushWrites();GarmGroup group(p,diff);PlayerLookup(p,"group");p.SetRaidDifficultyID(diff);auto map=sMapMgr->CreateMap(1648,&p)->ToInstanceMap();Require(map&&map->GetDifficultyID()==diff,"Fresh native Trial selection failed");
    staging.RemovePlayerFromMap(&p,false);p.ResetMap();p.SetMap(map);p.Relocate(pos);Require(map->AddPlayerToMap(&p,false),"Native Trial player admission failed");
    auto script=map->GetInstanceScript();auto doors=LoadDoors(*map);auto odyn=LoadBoss(*map,505062);auto guarm=LoadBoss(*map,505263);auto helya=LoadBoss(*map,505276);Doors(doors,true,true,false);
    PlayerLookup(p,"actors");PumpInstance(*map,2500);Heads(*map,*guarm);PlayerLookup(p,"heads");
    // Earlier defender combat is outside this transition probe. Earn Odyn's
    // DONE through his real surrender, never seed Guarm DONE or the passage.
    script->SetData(Data::DataMisc::OdynEventComplete1,DONE);script->SetData(Data::DataMisc::OdynEventComplete2,DONE);script->SetBossState(0,IN_PROGRESS);Doors(doors,false,true,false);
    odyn->AI()->DoAction(ACTION_1);odyn->AI()->UpdateAI(7000);odyn->CastStop();odyn->AI()->UpdateAI(2000);odyn->CastStop();odyn->SetHealth(odyn->CountPctFromMaxHealth(9));uint32 damage=1;odyn->AI()->DamageTakenAfterScaling(&p,damage,DIRECT_DAMAGE);
    Require(script->GetBossState(0)==DONE&&script->GetBossState(1)==NOT_STARTED,"Actual Odyn transition failed");Doors(doors,true,true,false);Checkpoint(*map,group.group,1);PlayerLookup(p,"odyn");
    Travel(p,*odyn);
    map->PlayerRelocation(&p,guarm->GetPositionX(),guarm->GetPositionY(),guarm->GetPositionZ(),guarm->GetOrientation());guarm->SetInCombatWith(&p);Require(script->GetBossState(1)==IN_PROGRESS,"Native Guarm combat entry failed");Doors(doors,true,false,false);
    ClearCombat(p,*guarm);PlayerLookup(p,"evade");Require(script->GetBossState(1)==NOT_STARTED,"Native Guarm evade failed");Doors(doors,true,true,false);Heads(*map,*guarm);
    Require(!guarm->HasUnitState(UNIT_STATE_EVADE),"Native Guarm home return did not finish");guarm->SetInCombatWith(&p);Require(script->GetBossState(1)==IN_PROGRESS,"Guarm retry cannot enter combat");Doors(doors,true,false,false);p.Begin(44868);
    // Tap/damage eligibility is setup; execute real native reward/death/mask.
    guarm->ResetPlayerDamageReq();guarm->LowerPlayerDamageReq(1);guarm->SetLootRecipient(&p);
    {LootPet pet(p,*map);Unit* killer=petKill?static_cast<Unit*>(&pet):static_cast<Unit*>(&p);Require(killer->GetCharmerOrOwnerPlayerOrPlayerItself()==&p&&guarm->GetLootRecipient()==&p&&guarm->IsDamageEnoughForLootingAndReward()&&guarm->IsDungeonBoss(),"Native player/pet owner/tap/reward prerequisites failed");killer->Kill(guarm,false);killer->Kill(guarm,false);}Tick(p,2000);
    Require(guarm->isDead(false)&&script->GetBossState(1)==DONE,"Native Guarm death failed");Doors(doors,true,true,true);auto saved=Checkpoint(*map,group.group,3);auto id=map->GetInstanceId();
    Require(!p.GetQuestObjectiveData(44868,55482)&&p.GetQuestStatus(44868)==QUEST_STATUS_INCOMPLETE,"Guarm completed Helya's quest");
    unsigned trash=0;for(auto c:Actors(*map))switch(c->GetEntry())
    {case Data::Creatures::DeepbrineMonstrosity:case Data::Creatures::KvaldirReefcaller:case Data::Creatures::KvaldirTideWitch:case Data::Creatures::DarkSeraph:case Data::Creatures::RotsoulGiant:case Data::Creatures::AncientBonethrall:case Data::Creatures::KvaldirCoralMaiden:p.Kill(c,false);++trash;if(trash<26)Require(helya->HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC|UNIT_FLAG_NOT_ATTACKABLE_1),"Helya unlocked before last entrance enemy");break;}
    Require(trash==26&&!helya->HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC|UNIT_FLAG_NOT_ATTACKABLE_1),"Actual Helya entrance trash did not unlock boss");
    p.CombatStop(true);map->PlayerRelocation(&p,helya->GetPositionX(),helya->GetPositionY(),helya->GetPositionZ(),helya->GetOrientation());helya->SetInCombatWith(&p);Require(script->GetBossState(2)==IN_PROGRESS,"Helya combat did not start after Guarm/trash");Doors(doors,true,true,false);
    if(check)
    {
        check(p,*map,*helya,group.group);
        map->RemovePlayerFromMap(&p,false);p.ResetMap();p.SetMap(&staging);p.Relocate(pos);Require(staging.AddPlayerToMap(&p,false),"Staging readmission failed");parent.UnloadAll();return;
    }
    ClearCombat(p,*helya);Require(script->GetBossState(2)==NOT_STARTED&&script->GetBossState(1)==DONE,"Helya evade lost Guarm progress");Doors(doors,true,true,true);saved=Checkpoint(*map,group.group,3);
    std::cout<<"PASS: actual Odyn surrender -> Guarm combat/evade/retry/death -> passage; diff="<<uint32(diff)<<" race="<<uint32(race)<<" pet="<<petKill<<std::endl;
    // Leave and cold-load the actually written save and corpse respawn data.
    map->RemovePlayerFromMap(&p,false);p.ResetMap();p.SetMap(&staging);p.Relocate(pos);Require(staging.AddPlayerToMap(&p,false),"Staging readmission failed");parent.UnloadAll();
    InstanceMap restored(1648,300,id,diff,&parent);InstanceSave save(1648,id,diff,saved.mask,saved.data,GameTime::GetGameTime()+WEEK,false);restored.CreateInstanceData(&save);restored.LoadRespawnTimes();
    auto loaded=restored.GetInstanceScript();auto restoredDoors=LoadDoors(restored);auto dead=LoadBoss(restored,505263);LoadBoss(restored,505062);auto restoredHelya=LoadBoss(restored,505276);
    PumpInstance(restored,2500);Require(dead->isDead(false)&&loaded->GetBossState(0)==DONE&&loaded->GetBossState(1)==DONE&&loaded->GetBossState(2)==NOT_STARTED&&loaded->GetCompletedEncounterMask()==3,"Saved Guarm initialization lost progress/dead state");Doors(restoredDoors,true,true,true);for(unsigned seat=0;seat<3;++seat)Require(!dead->GetVehicleKit()->GetPassenger(seat),"Saved dead Guarm recreated living heads");
    std::cout<<"PASS: native saved Guarm corpse/doors and Helya prerequisite restored; diff="<<uint32(diff)<<std::endl;restored.UnloadAll();
}
static int RunGarm(int argc,char** argv,HelyaProgressionCheck check=nullptr)
{
    InitializeNative(argc,argv);opcodeTable.Initialize();LoadGarmInputs(check!=nullptr);OpenScratch();sAchievementMgr->LoadRewards();sAchievementMgr->LoadCompletedAchievements();
    // Normal world startup sizes these before any player/pet is registered.
    // A socketless bootstrap must do the same for native owner/tap lookups.
    sObjectAccessor->SetGuidSize(HighGuid::Player,100000);sObjectAccessor->SetGuidSize(HighGuid::Pet,100000);sObjectAccessor->SetGuidSize(HighGuid::EventObject,100000);
    for(auto state:{WS_INSTANCE_DAILY_RESET_TIME,WS_INSTANCE_WEEKLY_RESET_TIME,WS_WEEKLY_RESET_TIME})sWorld->setWorldState(state,uint32(GameTime::GetGameTime()+WEEK));
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());AddSC_tov_boss_odyn();AddSC_boss_garm();AddSC_boss_tov_helya();AddSC_trial_of_valor();AddSC_instance_trial_of_valor();sScriptMgr->SwapScriptContext(true);
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1648,{}},{1220,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1648,{}},{1220,{}}});
    sMapMgr->InitInstanceIds();auto parent=static_cast<MapInstanced*>(sMapMgr->CreateBaseMap(1648));parent->SetMapStop();sMapMgr->_mapThreads[1648]->join();
    {Map staging(1220,300,0,DIFFICULTY_NONE);for(auto diff:{DIFFICULTY_LFR_RAID,DIFFICULTY_NORMAL_RAID,DIFFICULTY_HEROIC_RAID,DIFFICULTY_MYTHIC_RAID})for(auto race:{RACE_HUMAN,RACE_ORC})GarmCase(staging,*parent,diff,race,race==RACE_ORC,check);}
    FlushWrites();sInstanceSaveMgr->UnloadAll();CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();return 0;
}
#ifndef TRIAL_GARM_SUPPORT_ONLY
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{return RunGarm(argc,argv);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
#endif
