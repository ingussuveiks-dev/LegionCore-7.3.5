// Deserialized gossip packets, linked victory AI, native far teleport/ACK,
// and prepared quest/inventory reload against disposable character schemas.
#define TRIAL_RAID_INSTANCE_SUPPORT_ONLY
#include "TrialRaidInstanceNative.cpp"
#include "GossipData.h"
#include "NPCPackets.h"
#include "MapManager.h"

static void Hello(TrialPlayer& p,Creature& giver)
{
    WorldPacket wire(CMSG_TALK_TO_GOSSIP);wire<<giver.GetGUID();
    WorldPackets::NPC::Hello packet(std::move(wire));packet.Read();p.GetSession()->HandleGossipHelloOpcode(packet);
    auto& menu=p.PlayerTalkClass->GetGossipMenu();
    Require(menu.GetMenuId()==20849&&menu.GetMenuItemCount()==1&&menu.GetItem(1),"Actual native exit menu/option not admitted");
}
static void Select(TrialPlayer& p,Creature& giver,uint32 menu,uint32 option)
{
    WorldPacket wire(CMSG_GOSSIP_SELECT_OPTION);wire<<giver.GetGUID()<<menu<<option;wire.WriteBits(0,8);wire.FlushBits();
    WorldPackets::NPC::GossipSelectOption packet(std::move(wire));packet.Read();p.GetSession()->HandleGossipSelectOption(packet);
}
static void CheckExit(InstanceMap& map,Creature& giver)
{
    auto destination=sWorldSafeLocsStore.LookupEntry(5779);
    Require(destination&&destination->MapID==1220,"Native exit WorldSafeLocs changed");
    std::cout<<"EXIT_DEST map="<<destination->MapID<<" x="<<destination->Loc.X<<" y="<<destination->Loc.Y<<" z="<<destination->Loc.Z<<" degrees="<<destination->Loc.O<<std::endl;
    for(auto race:{RACE_HUMAN,RACE_ORC})for(unsigned state=0;state<3;++state)
    {
        auto guid=++nextGuid;auto saved=map.GetInstanceScript()->GetSaveData();
        {
            TrialPlayer p(map,race,guid);p.Relocate(giver.GetPosition());
            // Precreate the real destination zone, then stop its socket worker.
            // This deterministic probe drives packets/ACK synchronously; a
            // socketless fixture session would otherwise be logged out.
            auto targetBase=static_cast<MapInstanced*>(sMapMgr->FindBaseMap(1220));
            auto preparedTarget=sMapMgr->CreateMap(1220,&p);preparedTarget->SetMapStop();
            for(auto& thread:targetBase->_zoneThreads)if(thread.second->joinable())thread.second->join();
            // Register the already attached fixture session in the real lookup
            // used by SafeTeleport. No socket/login or map worker is simulated.
            WorldSessionPtr session(p.GetSession(),[](WorldSession*){});
            auto& sessions=const_cast<SessionMap&>(sWorld->GetAllSessions());sessions[0]=session;session->SetMap(&map);
            p.SaveToDB(true);FlushWrites();
            if(state)
            {
                const_cast<RewardedQuestSet&>(p.getRewardedQuests()).insert(44729);(p.*Access(RewardSaveTag{}))[44729]=QUEST_DEFAULT_SAVE_TYPE;
                p.Begin(44868);Emit(map,2008);Tick(p,2000);
                Require(p.GetQuestStatus(44868)==QUEST_STATUS_COMPLETE,"Exit pending quest setup failed");
                if(state==2)
                {
                    p.SaveToDB();FlushWrites();
                    WorldPacket wire(CMSG_QUEST_GIVER_CHOOSE_REWARD);wire<<giver.GetGUID()<<int32(44868)<<int32(0);
                    WorldPackets::Quest::QuestGiverChooseReward packet(std::move(wire));packet.Read();session->HandleQuestGiverChooseReward(packet);Tick(p,200);
                    Require(p.GetQuestRewardStatus(44868)&&p.GetItemCount(142534)==1,"Exit rewarded quest setup failed");
                }
            }
            auto status=p.GetQuestStatus(44868);auto reward=p.GetQuestRewardStatus(44868);auto items=p.GetItemCount(142534);
            p.SaveToDB();FlushWrites();
#ifndef TRIAL_EXIT_BASELINE
            p.PlayerTalkClass->ClearMenus();Select(p,giver,20849,1);
            Require(!p.IsBeingTeleportedFar(),"Exit accepted an option without an opened menu");
            Hello(p,giver);Select(p,giver,20850,1);
            Require(!p.IsBeingTeleportedFar(),"Exit accepted a foreign menu ID");
            Select(p,giver,20849,0);Select(p,giver,20849,99);
            Require(!p.IsBeingTeleportedFar(),"Exit accepted an absent option");
            p.Relocate(giver.GetPositionX()+50,giver.GetPositionY(),giver.GetPositionZ());Select(p,giver,20849,1);
            Require(!p.IsBeingTeleportedFar(),"Exit bypassed native interaction distance");p.Relocate(giver.GetPosition());
            Hello(p,giver);Select(p,giver,20849,1);
#else
            Hello(p,giver);Select(p,giver,20850,1);
            std::cout<<"BASELINE: foreign menu20850 accepted for exit20849"<<std::endl;
#endif
            Require(p.IsBeingTeleportedFar()&&!p.IsInWorld(),"Native gossip did not start far teleport/remove from raid");
#ifndef TRIAL_EXIT_BASELINE
            Require(p.PlayerTalkClass->GetGossipMenu().Empty(),"Exit retained a consumed menu");
#endif
            auto loc=p.GetTeleportDest();
            Require(loc.GetMapId()==destination->MapID&&loc.GetExactDist(destination->Loc.X,destination->Loc.Y,destination->Loc.Z)<0.01f&&std::abs(loc.GetOrientation()-destination->Loc.O*float(M_PI)/180.0f)<0.001f,"Native exit destination/orientation differs from DB2");
            auto target=p.GetTeleportMap();Require(target&&session->GetMap()==target,"Native teleport target/session map missing");
            session->HandleWorldPortAck();
            Require(p.IsInWorld()&&!p.IsBeingTeleportedFar()&&!p.IsChangeMap()&&p.GetMap()==target&&p.GetMapId()==1220,"Native worldport ACK failed destination admission");
            Require(p.GetExactDist(destination->Loc.X,destination->Loc.Y,destination->Loc.Z)<0.01f,"Native ACK lost DB2 coordinates");
            // Replayed selection/ACK after departure must not restart transfer.
            Select(p,giver,20849,1);session->HandleWorldPortAck();
            Require(!p.IsBeingTeleportedFar()&&p.GetMap()==target,"Repeated exit packet restarted transfer");
            Require(p.GetQuestStatus(44868)==status&&p.GetQuestRewardStatus(44868)==reward&&p.GetItemCount(142534)==items&&map.GetInstanceScript()->GetSaveData()==saved,"Exit modified quest/reward/raid state");
            p.SaveToDB();FlushWrites();
            auto row=CharacterDatabase.Query(Trinity::StringFormat("SELECT map,position_x,position_y,position_z FROM characters WHERE guid=%u",guid).c_str());
            Require(row&&row->Fetch()[0].GetUInt32()==1220&&std::abs(row->Fetch()[1].GetFloat()-destination->Loc.X)<0.01f&&std::abs(row->Fetch()[2].GetFloat()-destination->Loc.Y)<0.01f&&std::abs(row->Fetch()[3].GetFloat()-destination->Loc.Z)<0.01f,"Native exit save lost destination");
            target->RemovePlayerFromMap(&p,false);sessions.erase(0);session->SetMap(nullptr);
        }
        {
            TrialPlayer restored(map,race,guid);Reload(restored);
            Require(restored.GetQuestStatus(44868)==(state==0?QUEST_STATUS_NONE:state==1?QUEST_STATUS_COMPLETE:QUEST_STATUS_REWARDED)&&restored.GetQuestRewardStatus(44868)==(state==2)&&restored.GetItemCount(142534)==uint32(state==2),"Prepared reload lost exit quest/reward state");
        }
        std::cout<<"PASS: native exit gossip/DB2 teleport/worldport ACK/save/reload; diff="<<uint32(map.GetDifficultyID())<<" race="<<uint32(race)<<" questState="<<state<<std::endl;
    }
}
static int RunExit(int argc,char** argv)
{
    InitializeNative(argc,argv);opcodeTable.Initialize();LoadInstanceInputs();OpenScratch();sAchievementMgr->LoadRewards();sAchievementMgr->LoadCompletedAchievements();
    sObjectMgr->LoadNPCText();sQuestDataStore->LoadPointsOfInterest();sGossipDataStore->LoadGossipMenu();sGossipDataStore->LoadGossipMenuItems();
    auto conditions=WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(14,15) AND SourceGroup=20849");
    Require(conditions&&!conditions->Fetch()[0].GetUInt32(),"Load new scoped native gossip conditions before testing");
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());AddSC_tov_boss_odyn();AddSC_boss_tov_helya();AddSC_trial_of_valor();AddSC_instance_trial_of_valor();sScriptMgr->SwapScriptContext(true);
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1648,{}},{1220,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1648,{}},{1220,{}}});
    auto destinationBase=sMapMgr->CreateBaseMap(1220);destinationBase->SetMapStop();sMapMgr->_mapThreads[1220]->join();
    {MapInstanced parent(1648,300);for(auto diff:{DIFFICULTY_LFR_RAID,DIFFICULTY_NORMAL_RAID,DIFFICULTY_HEROIC_RAID,DIFFICULTY_MYTHIC_RAID}){auto completed=WinCase(parent,diff,CheckExit);for(bool first:{false,true})SavedCase(parent,diff,completed,first,CheckExit);}}
    FlushWrites();CharacterDatabase.Close();LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();return 0;
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);
    try{return RunExit(argc,argv);}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
