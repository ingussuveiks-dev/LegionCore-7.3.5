// Compile the actual final instance dispatcher; only its pre-existing actors
// and cinematic traversal are outside this deterministic lifecycle fixture.
#include "MiscPackets.h"
// The two original content headers use Data as an enum and a namespace.
// Rename only this declaration to compile both unmodified dispatchers together.
#define Data BrokenShoreFixtureData
#ifdef SCENARIO_COMPLETION_BASELINE
#include "NativeBrokenShoreBaseline.inc"
#else
#include "../../src/server/scripts/Scenario/BrokenIslands/instance_broken_islands.cpp"
#endif
#undef Data
#include "NativeBrokenShoreReturn.inc"
static void ScenarioCompletionInputs()
{
    Require(sObjectMgr->LoadTrinityStrings(),"Native city-channel strings missing");
    auto rows=WorldDatabase.Query(
#include "NativeBrokenShoreQuest.inc"
    );Require(bool(rows),"Broken Shore quest templates missing");
    do {auto q=new Quest(rows->Fetch());const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[q->Id]=q;(sQuestDataStore->*Access(QuestVectorTag{}))[q->Id]=q;}while(rows->NextRow());
    auto objectives=WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID IN(42740,40518)");Require(bool(objectives),"Broken Shore objectives missing");
    do {auto f=objectives->Fetch();const_cast<Quest*>(sQuestDataStore->GetQuestTemplate(f[1].GetUInt32()))->LoadQuestObjective(f);}while(objectives->NextRow());
    for(auto id:{42740u,40518u}){auto qinfo=const_cast<Quest*>(sQuestDataStore->GetQuestTemplate(id));for(auto const& obj:qinfo->GetObjectives()){Require(obj.Type==QUEST_OBJECTIVE_MONSTER,"Load new objective postprocessing before testing");switch(obj.Type){
#include "NativeBrokenShoreMonster.inc"
    }}}
    auto& ends=sQuestDataStore->*Access(EndersTag{});auto& index=sQuestDataStore->*Access(StarterIndexTag{});
    (sQuestDataStore->*Access(RelationsLoaderTag{}))(ends,index,"(SELECT id,quest FROM creature_questender WHERE quest IN(42740,40518))",false,false);
}
static void ScenarioFinishExit(WorldSession* session)
{
    if(session->GetPlayer()->isWatchingMovie()){WorldPackets::Misc::CompleteMovie movie{WorldPacket(CMSG_COMPLETE_MOVIE)};session->HandleCompleteMovie(movie);session->GetPlayer()->Update(0);}
    auto p=session->GetPlayer();Require(p->IsBeingTeleportedFar(),"Native finale return transfer missing");auto destination=p->GetTeleportMap();Require(destination,"Finale return map missing");auto id=destination->GetId();SpiritFarFreeze(id);session->HandleWorldPortAck();SpiritFarFreeze(id);p->UpdateZone(p->GetCurrentZoneID(),p->GetCurrentAreaID());SpiritFarFreeze(id);if(p->IsBeingTeleportedFar()&&p->IsChangeMap())session->m_Functions.Update(500);Require(p->IsInWorld()&&!p->IsBeingTeleported(),"Finale return ACK did not admit Player");
}
static uint32 ScenarioClaim(WorldSession* session,Quest const* quest)
{
    auto p=session->GetPlayer();Actor giver(p->GetMap(),p->GetTeam()==ALLIANCE?100395:100453,++nextGuid,p->GetPosition());Require(p->GetMap()->AddToMap(static_cast<Creature*>(&giver)),"Native quest ender admission failed");
    QuestPackageItemEntry const* choice=nullptr;if(auto items=sDB2Manager.GetQuestPackageItems(quest->PackageID))for(auto item:*items)if(p->CanSelectQuestPackageItem(item)){choice=item;break;}if(!choice)if(auto items=sDB2Manager.GetQuestPackageItemsFallback(quest->PackageID))if(!items->empty())choice=items->front();Require(choice&&choice->ItemID&&choice->ItemQuantity,"Native Broken Shore reward package missing eligible choice");
    auto count=p->GetItemCount(choice->ItemID);auto already=p->GetQuestRewardStatus(quest->Id);
    auto request=[&](uint32 item){WorldPacket wire(CMSG_QUEST_GIVER_CHOOSE_REWARD);wire<<giver.GetGUID()<<int32(quest->Id)<<int32(item);WorldPackets::Quest::QuestGiverChooseReward packet(std::move(wire));packet.Read();session->HandleQuestGiverChooseReward(packet);};
    request(0);Require(p->GetQuestRewardStatus(quest->Id)==already&&p->GetItemCount(choice->ItemID)==count,"Missing-choice request consumed the quest or granted a reward");request(choice->ItemID);Require(p->GetQuestRewardStatus(quest->Id)&&p->GetItemCount(choice->ItemID)==count+(already?0:choice->ItemQuantity),"Native quest giver did not award exactly one package choice");auto money=p->GetMoney();auto xp=p->GetUInt32Value(PLAYER_FIELD_XP);request(choice->ItemID);Require(p->GetItemCount(choice->ItemID)==count+(already?0:choice->ItemQuantity)&&p->GetMoney()==money&&p->GetUInt32Value(PLAYER_FIELD_XP)==xp&&!p->CanRewardQuest(quest,false),"Repeated reward request duplicated item/money/XP or remained claimable");
    std::cout<<"SCENARIO_COMPLETION_CLAIM quest="<<quest->Id<<" package="<<quest->PackageID<<" item="<<choice->ItemID<<" quantity="<<choice->ItemQuantity<<" already="<<already<<std::endl;
    return choice->ItemID;
}
static Player* TrialScenarioCompletion(WorldSession*& session,WorldSessionPtr& owner,ScenarioLoginCleanup& cleanup,SpiritLoopback& oldRealm,SpiritLoopback& oldInstance,SpiritLoopback& nextRealm,SpiritLoopback& nextInstance,ObjectGuid guid,unsigned boundary,bool instanceEOF)
{
    auto player=session->GetPlayer();auto map=player->GetMap();auto scenario=sScenarioMgr->GetScenario(player->GetInstanceId());auto id=scenario->GetScenarioId();auto quest=sQuestDataStore->GetQuestTemplate(id==786?42740:40518);
    player->RemoveRewardedQuest(quest->Id);player->AddQuest(quest,nullptr);player->KilledMonsterCredit(id==786?108920:113118);Require(player->GetQuestStatus(quest->Id)==QUEST_STATUS_INCOMPLETE&&player->GetQuestObjectiveData(quest,int8(1))==1&&!player->GetQuestObjectiveData(quest,int8(0))&&!player->ExitCompletedBrokenShoreLogin(),"Completion input quest must retain native embark credit and unfinished finale");
    auto emit=[&](uint32 event,uint32 count=1){scenario->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,event,count,0,player,player);};
    if(!scenario->GetCurrentStep())emit(id==786?44060:54140);
    emit(id==786?44095:54116,33);emit(id==786?44077:54117,3);emit(id==786?52643:54114,3);Require(scenario->GetCurrentStep()==2,"Completion traversal stage2 missing");
    emit(id==786?45131:54109);emit(id==786?45228:54123);emit(id==786?45288:54141,4);Require(scenario->GetCurrentStep()==5,"Completion traversal stage5 missing");emit(44384,300);Require(scenario->GetCurrentStep()==6,"Native weighted progress did not reach300");emit(50027);emit(44669);Require(scenario->GetCurrentStep()==8&&!scenario->IsCompleted(false),"Final criterion input is not native stage8");ReadScenarioPackets(session);
    Require(!map->ToInstanceMap()->GetInstanceScript(),"Completion fixture would replace a live instance script");auto script=new instance_broken_islands::instance_broken_islands_InstanceMapScript(map->ToInstanceMap());script->team=player->GetTeam();map->ToInstanceMap()->*Access(TrialInstanceTag{})=script;
#ifndef SCENARIO_COMPLETION_BASELINE
    static bool abi=false;if(!abi){abi=true;std::cout<<"SCENARIO_COMPLETION_ABI finale="<<(reinterpret_cast<char*>(&script->finaleStarted)-reinterpret_cast<char*>(script))<<std::endl;}
#endif
    script->SetData(SCENARION_STEP_END,0);Require(scenario->IsCompleted(false)&&scenario->GetCurrentStep()==9,"Actual final instance action did not complete scenario");
    auto packets=ReadScenarioPackets(session);Require(packets.completed==1&&packets.states.size()==1&&packets.states[0].complete&&packets.states[0].step==uint32(-1)&&packets.states[0].active.size()==9,"Native final state/completion packet invalid");
    std::cout<<"SCENARIO_COMPLETION_BOUNDARY scenario="<<id<<" boundary="<<boundary<<" quest_status="<<uint32(player->GetQuestStatus(quest->Id))<<" final_credit="<<player->GetQuestObjectiveData(quest,int8(0))<<std::endl;
    Require(player->GetQuestStatus(quest->Id)==QUEST_STATUS_COMPLETE&&player->GetQuestObjectiveData(quest,int8(0))==1&&!player->GetQuestRewardStatus(quest->Id),"Final criterion loses quest completion before delayed cinematics");
    scenario->SendStepUpdate(player,true);auto full=ReadScenarioPackets(session);Require(full.states.size()==1&&full.states[0].complete&&full.states[0].step==uint32(-1)&&full.states[0].progress.size()==scenario->GetAchievementMgr().GetCriteriaProgressMap()->size(),"Completed full snapshot lost native progress");for(auto const& progress:full.states[0].progress)CheckScenarioProgress(progress,*scenario);
    script->SetData(SCENARION_STEP_END,0);auto repeated=ReadScenarioPackets(session);Require(!repeated.completed&&player->GetQuestObjectiveData(quest,int8(0))==1,"Repeated finale duplicated completion/quest credit");
    auto target=sSpellMgr->GetSpellTargetPosition(id==786?208514:208517);Require(target,"Native finale return destination missing");
    uint32 claimedItem=0;
    if(boundary){(player->*Access(SpiritOrderFunctionsTag{})).Update(100);if(boundary==3)Require(player->isWatchingMovie()&&player->IsBeingTeleportedFar()&&!player->GetTeleportMap(),"Movie disconnect did not retain the native pending transfer");else {ScenarioFinishExit(session);Require(player->GetMapId()==target->target_mapId,"Delayed finale used a different return map");if(boundary==2)claimedItem=ScenarioClaim(session,quest);}}
    auto claimedCount=claimedItem?player->GetItemCount(claimedItem):0;
    auto source=player->GetMap();auto beforeMoney=player->GetMoney();auto beforeXP=player->GetUInt32Value(PLAYER_FIELD_XP);ReadScenarioPackets(session);
    (instanceEOF?oldInstance:oldRealm).PeerClose();Require(session->IsforceExit(),"Completion TCP EOF did not request logout");source->UpdateSessions(1);sWorld->UpdateSessions(1);FlushWrites();
    auto& sessions=const_cast<SessionMap&>(sWorld->GetAllSessions());Require(!session->GetPlayer()&&!ObjectAccessor::FindPlayer(guid)&&!sessions.count(0)&&!source->FindSession(0)&&!oldRealm.socket->IsOpen()&&!oldInstance.socket->IsOpen(),"Completion EOF retained Player/session/socket registration");
    auto saved=CharacterDatabase.Query(Trinity::StringFormat("SELECT scenarioResume,(SELECT COUNT(*) FROM character_queststatus_rewarded WHERE guid=%u AND quest=%u) FROM characters WHERE guid=%u",guid.GetCounter(),quest->Id,guid.GetCounter()).c_str());Require(saved&&saved->Fetch()[0].GetString().empty()&&saved->Fetch()[1].GetUInt32()==uint32(boundary==2),"Completion save retained restart checkpoint or wrong reward count");
    if(!boundary&&instanceEOF){auto parent=static_cast<MapInstanced*>(sMapMgr->FindBaseMap(1460));auto old=map->GetInstanceId();auto iterator=parent->m_InstancedMaps.find(old);Require(iterator!=parent->m_InstancedMaps.end()&&parent->DestroyInstance(iterator)&&!sScenarioMgr->GetScenario(old),"Completed empty instance unload failed");}
    auto retired=session;cleanup.session=nullptr;session=nullptr;owner.reset();delete retired;FlushWrites();session=new WorldSession(0,"ScenarioComplete",nextRealm.socket,SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});cleanup.session=session;owner=WorldSessionPtr(session,[](WorldSession*){});nextRealm.socket->SetWorldSession(owner);sessions[0]=owner;
    SpiritRequestLogin(session,guid);SpiritLinkSocket(nextInstance,session->GetConnectToInstanceKey());auto response=SpiritTakeLinkQuery(session);SpiritDeliverQuery(session,std::move(response));player=session->GetPlayer();Require(player,"Completed SQL login failed");SpiritFarFreeze(player->GetMapId());map=player->GetMap();map->UpdateSessions(99);map->UpdateSessions(1);SpiritFarFreeze(player->GetMapId());
    if(player->IsBeingTeleportedFar())ScenarioFinishExit(session);std::cout<<"SCENARIO_COMPLETION_LOGIN scenario="<<id<<" boundary="<<boundary<<" map="<<player->GetMapId()<<" target="<<target->target_mapId<<" distance="<<player->GetExactDist(target->target_X,target->target_Y,target->target_Z)<<" selector="<<player->GetScenarioId()<<" quest="<<uint32(player->GetQuestStatus(quest->Id))<<" money="<<player->GetMoney()<<" expected_money="<<beforeMoney<<" xp="<<player->GetUInt32Value(PLAYER_FIELD_XP)<<" expected_xp="<<beforeXP<<std::endl;Require(player->IsInWorld()&&player->GetMapId()==target->target_mapId&&player->GetExactDist(target->target_X,target->target_Y,target->target_Z)<0.01f&&!player->GetScenarioId(),"Completed login did not return to native finale destination");
    Require(player->GetQuestStatus(quest->Id)==(boundary==2?QUEST_STATUS_REWARDED:QUEST_STATUS_COMPLETE)&&player->GetMoney()==beforeMoney&&player->GetUInt32Value(PLAYER_FIELD_XP)==beforeXP&&(!claimedItem||player->GetItemCount(claimedItem)==claimedCount)&&!CanReturnToBrokenShore(player),"SQL reconnect lost completed/rewarded quest/item or reopened unfinished recovery");SpiritLoadingScreen(session,player->GetMapId(),false);auto reconnect=ReadScenarioPackets(session);Require(reconnect.states.empty()&&!reconnect.completed,"Completed login replayed a scenario state/reward");
    auto item=ScenarioClaim(session,quest);Require(!CanReturnToBrokenShore(player),"Rewarded quest reopened scenario recovery");player->SaveToDB();FlushWrites();auto claimed=CharacterDatabase.Query(Trinity::StringFormat("SELECT (SELECT COUNT(*) FROM character_queststatus_rewarded WHERE guid=%u AND quest=%u),(SELECT COALESCE(SUM(ii.count),0) FROM character_inventory ci JOIN item_instance ii ON ci.item=ii.guid WHERE ci.guid=%u AND ii.itemEntry=%u)",guid.GetCounter(),quest->Id,guid.GetCounter(),item).c_str());Require(claimed&&claimed->Fetch()[0].GetUInt32()==1&&claimed->Fetch()[1].GetUInt32()==player->GetItemCount(item),"Completed reconnect lost the single reward record or saved package inventory");
    // Restore only this disposable character's intro quest for the next case.
    player->RemoveRewardedQuest(quest->Id);player->RemoveActiveQuest(quest->Id);player->SaveToDB();FlushWrites();
    std::cout<<"PASS: native scenario completion scenario="<<id<<" boundary="<<boundary<<" instance_eof="<<instanceEOF<<" map="<<player->GetMapId()<<std::endl;
    return player;
}
