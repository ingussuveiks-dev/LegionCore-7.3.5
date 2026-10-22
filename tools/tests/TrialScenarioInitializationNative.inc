#include "ScenarioMgr.h"
#include "AchievementPackets.h"
struct ScenarioWireProgress{uint32 id=0,date=0,start=0,create=0;uint64 quantity=0;ObjectGuid player;};
struct ScenarioWireState{uint32 id=0,step=0;std::vector<uint32> active;std::vector<ScenarioWireProgress> progress;bool complete=false;};
struct ScenarioWirePackets{std::vector<ScenarioWireState> states;std::vector<ScenarioWireProgress> updates;unsigned initial=0;};
static ScenarioWireProgress ReadScenarioProgress(ByteBuffer& packet)
{
    ScenarioWireProgress p;packet>>p.id>>p.quantity>>p.player>>p.date>>p.start>>p.create;packet.ReadBits(4);packet.ResetBitPos();return p;
}
static ScenarioWirePackets ReadScenarioPackets(WorldSession* session)
{
    ScenarioWirePackets result;
    for(auto const& socket:session->*Access(SpiritSocketsTag{}))if(socket){EncryptablePacket* packet=nullptr;
        while((socket.get()->*Access(SpiritPacketQueueTag{})).Dequeue(packet)){
            packet->rpos(0);
            if(packet->GetOpcode()==SMSG_WORLD_SERVER_INFO)++result.initial;
            if(packet->GetOpcode()==SMSG_SCENARIO_PROGRESS_UPDATE)result.updates.push_back(ReadScenarioProgress(*packet));
            if(packet->GetOpcode()==SMSG_SCENARIO_STATE){
                ScenarioWireState state;uint32 difficulty,wave,maximum,timer,progress,bonus,active,spells;
                *packet>>state.id>>state.step>>difficulty>>wave>>maximum>>timer>>progress>>bonus>>active>>spells;
                Require(progress<100&&bonus<100&&active<100&&spells<100,"Unexpected scenario packet vector size");
                for(uint32 i=0;i<active;++i){uint32 id;*packet>>id;state.active.push_back(id);}state.complete=packet->ReadBit();packet->ResetBitPos();
                for(uint32 i=0;i<progress;++i)state.progress.push_back(ReadScenarioProgress(*packet));
                for(uint32 i=0;i<bonus+spells;++i){uint32 id;*packet>>id;packet->ReadBit();packet->ResetBitPos();}
                Require(packet->rpos()==packet->size(),"ScenarioState decoder left unread bytes");result.states.push_back(std::move(state));
            }
            delete packet;
        }
    }
    return result;
}
static void CheckScenarioProgress(ScenarioWireProgress const& wire,Scenario& scenario)
{
    auto progress=scenario.GetAchievementMgr().GetCriteriaProgressMap();
    auto found=std::find_if(progress->begin(),progress->end(),[&](auto const& p){return p.second.criteria&&p.second.criteria->ID==wire.id;});Require(found!=progress->end()&&found->second.Counter==wire.quantity,"Scenario packet changed criterion ID/quantity");
    ByteBuffer packed;packed.AppendPackedTime(found->second.date);auto expectedDate=packed.read<uint32>();
    std::cout<<"SCENARIO_PROGRESS_DATE scenario="<<scenario.GetScenarioId()<<" criterion="<<wire.id<<" quantity="<<wire.quantity<<" packed="<<wire.date<<" expected="<<expectedDate<<std::endl;
    Require(wire.date==expectedDate,"Scenario packet replaced the criterion update timestamp with elapsed age");
}
static void CheckScenarioState(ScenarioWireState const& state,Scenario& scenario,unsigned stage,bool full)
{
    auto steps=sScenarioMgr->GetScenarioSteps(scenario.GetScenarioId());Require(steps&&steps->size()>stage,"Native scenario steps missing");
    Require(state.id==scenario.GetScenarioId()&&state.step==(*steps)[stage]->ID&&!state.complete,"Scenario packet has wrong identity/current step/completion");
    Require(state.active.size()==stage+1,"Scenario packet has wrong ActiveSteps count");for(unsigned i=0;i<=stage;++i)Require(state.active[i]==(*steps)[i]->ID,"Scenario packet lost a native active step");
    auto progress=scenario.GetAchievementMgr().GetCriteriaProgressMap();Require(state.progress.size()==(full?progress->size():0),"Scenario full snapshot lost criteria progress");
    for(auto const& wire:state.progress)CheckScenarioProgress(wire,scenario);
}
struct ScenarioLoginCleanup
{
    WorldSession* session;WorldSessionPtr previous;
    ~ScenarioLoginCleanup(){if(session&&session->GetPlayer()){session->SetMap(nullptr);session->LogoutPlayer(true);FlushWrites();}auto& sessions=const_cast<SessionMap&>(sWorld->GetAllSessions());if(previous)sessions[0]=previous;else sessions.erase(0);}
};
#include "TrialScenarioRelogNative.inc"
#include "TrialScenarioReconnectNative.inc"
static void TrialScenarioInitialization(ObjectGuid guid,WorldLocation const& expected,uint64 itemGuid,uint32 durability,WorldSession* oldSession,uint32 difficulty,uint8 race,unsigned direction)
{
    static bool loaded=false;if(!loaded){sObjectMgr->LoadScenarioData();sObjectMgr->LoadScenarioSpellData();loaded=true;
        CriteriaProgressMap::value_type node;WorldPackets::Achievement::CriteriaTreeProgress wire;
        std::cout<<"SCENARIO_INITIALIZATION_ABI node_date="<<(reinterpret_cast<char*>(&node.second.date)-reinterpret_cast<char*>(&node))<<" packet_date="<<(reinterpret_cast<char*>(&wire.Date)-reinterpret_cast<char*>(&wire))<<std::endl;
    }
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1460,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1460,{}}});
    auto addresses=realm.Addresses;realm.Addresses={boost::asio::ip::address_v4::loopback(),boost::asio::ip::address_v4::loopback()};
    for(unsigned mode=0;mode<3;++mode){
        SpiritLoopback realmSocket,instanceSocket,relogSocket,restartSocket,reconnectRealm,reconnectInstance,unloadRealm,unloadInstance;auto session=new WorldSession(0,"ScenarioInit",realmSocket.socket,SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});auto shared=WorldSessionPtr(session,[](WorldSession*){});realmSocket.socket->SetWorldSession(shared);
        auto& sessions=const_cast<SessionMap&>(sWorld->GetAllSessions());auto found=sessions.find(0);WorldSessionPtr previous;if(found!=sessions.end()){previous=found->second;Require(previous.get()==oldSession,"Scenario fixture would replace unrelated session");}sessions[0]=shared;ScenarioLoginCleanup cleanup{session,previous};
        SpiritRequestLogin(session,guid);SpiritLinkSocket(instanceSocket,session->GetConnectToInstanceKey());auto response=SpiritTakeLinkQuery(session);SpiritDeliverQuery(session,std::move(response));SpiritCancelComplete(session,guid,expected,itemGuid,durability);auto player=session->GetPlayer();
        uint32 scenarioId=player->GetTeam()==ALLIANCE?786:1189;auto data=sLFGMgr->GetLFGDungeon(uint16(786),uint16(1460));Require(data&&(data->x||data->y||data->z),"Native Broken Shore LFG entrance missing");auto route=sObjectMgr->GetScenarioOnMap(1460,data->difficulty,player->GetTeam(),player->getClass(),data->id);Require(route&&route->ScenarioID==scenarioId,"Native faction scenario route differs");
        auto steps=sScenarioMgr->GetScenarioSteps(scenarioId);Require(steps&&steps->size()==9&&(*steps)[0]->ID==(scenarioId==786?1504:2895)&&(*steps)[1]->ID==(scenarioId==786?1522:2896),"Native Broken Shore step catalog changed");
        WorldLocation entrance(1460,data->x,data->y,data->z,data->o);player->SetScenarioId(uint16(scenarioId));Require(player->TeleportTo(entrance)&&player->IsBeingTeleportedFar(),"Actual scenario teleport did not start");SpiritFarFreeze(1460);auto map=player->GetTeleportMap();Require(map&&map->IsScenario(),"Native scenario map factory failed");ReadScenarioPackets(session);
        if(mode!=1){SpiritLoadingScreen(session,1460,false);Require(ReadScenarioPackets(session).states.empty(),"Scenario state sent before destination admission");}
        session->HandleWorldPortAck();SpiritFarFreeze(1460);auto scenario=sScenarioMgr->GetScenario(player->GetInstanceId());Require(player->IsInWorld()&&!player->IsBeingTeleported()&&scenario&&scenario->GetScenarioId()==scenarioId&&scenario->GetMap()==map&&player->GetScenarioId()==0,"Native scenario construction/selector consumption failed");
        auto initial=ReadScenarioPackets(session);if(mode==1){Require(!initial.initial&&initial.states.empty(),"Late-screen scenario initialized before its finish");SpiritLoadingScreen(session,1460,false);initial=ReadScenarioPackets(session);}Require(initial.initial==1&&initial.states.size()==1,"Scenario arrival did not send one initialization/snapshot");CheckScenarioState(initial.states[0],*scenario,0,true);
        scenario->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,scenarioId==786?44060:54140,1,0,player,player);Require(scenario->GetCurrentStep()==1,"Native first criterion did not advance scenario");scenario->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,scenarioId==786?44095:54116,7,0,player,player);Require(scenario->GetCurrentStep()==1,"Native partial criterion advanced scenario unexpectedly");
        auto updates=ReadScenarioPackets(session);Require(updates.updates.size()==2&&updates.updates[0].quantity==1&&updates.updates[1].quantity==7,"Native incremental scenario progress missing");for(auto const& wire:updates.updates)CheckScenarioProgress(wire,*scenario);for(auto const& state:updates.states)CheckScenarioState(state,*scenario,1,false);
        ReadScenarioPackets(session);scenario->SendStepUpdate(player,true);auto full=ReadScenarioPackets(session);Require(full.states.size()==1,"Native full scenario snapshot missing");CheckScenarioState(full.states[0],*scenario,1,true);
        if(mode<2){player=TrialScenarioRelog(session,relogSocket,guid,*scenario,itemGuid,durability,mode==0);player=TrialScenarioRelog(session,restartSocket,guid,*scenario,itemGuid,durability,mode==1,true);scenario=sScenarioMgr->GetScenario(player->GetInstanceId());}
        if(mode==2){
            player=TrialScenarioReconnect(session,shared,cleanup,realmSocket,instanceSocket,reconnectRealm,reconnectInstance,guid,itemGuid,durability,false,direction!=0);player=TrialScenarioReconnect(session,shared,cleanup,reconnectRealm,reconnectInstance,unloadRealm,unloadInstance,guid,itemGuid,durability,true,direction!=0);scenario=sScenarioMgr->GetScenario(player->GetInstanceId());
            auto instance=player->GetInstanceId();Require(player->TeleportTo(expected)&&player->IsBeingTeleportedFar(),"Scenario exit failed");SpiritFarFreeze(expected.GetMapId());session->HandleWorldPortAck();SpiritFarFreeze(expected.GetMapId());player->UpdateZone(player->GetCurrentZoneID(),player->GetCurrentAreaID());SpiritFarFreeze(expected.GetMapId());if(player->IsBeingTeleportedFar()&&player->IsChangeMap())session->m_Functions.Update(500);SpiritLoadingScreen(session,expected.GetMapId(),false);ReadScenarioPackets(session);
            player->SetScenarioId(uint16(scenarioId));Require(player->TeleportTo(entrance)&&player->IsBeingTeleportedFar(),"Scenario re-entry failed");SpiritFarFreeze(1460);ReadScenarioPackets(session);SpiritLoadingScreen(session,1460,false);Require(ReadScenarioPackets(session).states.empty(),"Fresh scenario snapshot sent before re-entry ACK");session->HandleWorldPortAck();SpiritFarFreeze(1460);auto fresh=sScenarioMgr->GetScenario(player->GetInstanceId());Require(player->GetInstanceId()!=instance&&fresh&&fresh!=scenario&&fresh->GetScenarioId()==scenarioId&&fresh->GetCurrentStep()==0,"Unbound solo re-entry did not construct a fresh native scenario");scenario=fresh;auto rejoined=ReadScenarioPackets(session);Require(rejoined.initial==1&&rejoined.states.size()==1,"Fresh re-entry snapshot missing/duplicated");CheckScenarioState(rejoined.states[0],*scenario,0,true);
        }
        SpiritLoadingScreen(session,1460,false);SpiritLoadingScreen(session,1460,true);SpiritLoadingScreen(session,1460,false);auto duplicate=ReadScenarioPackets(session);Require(!duplicate.initial&&duplicate.states.empty()&&duplicate.updates.empty(),"Duplicate screen resent scenario initialization/progress");
        Require(player->TeleportTo(expected)&&player->IsBeingTeleportedFar(),"Scenario final exit failed");SpiritFarFreeze(expected.GetMapId());session->HandleWorldPortAck();SpiritFarFreeze(expected.GetMapId());player->UpdateZone(player->GetCurrentZoneID(),player->GetCurrentAreaID());SpiritFarFreeze(expected.GetMapId());if(player->IsBeingTeleportedFar()&&player->IsChangeMap())session->m_Functions.Update(500);SpiritLoadingScreen(session,expected.GetMapId(),false);player->UnbindInstance(1460,Difficulty(data->difficulty));session->SetMap(nullptr);session->LogoutPlayer(true);FlushWrites();Require(!session->GetPlayer()&&!session->PlayerLoading()&&!ObjectAccessor::FindPlayer(guid),"Scenario fixture retained Player/loading state");
        std::cout<<"PASS: native scenario initialization diff="<<difficulty<<" race="<<uint32(race)<<" direction="<<direction<<" mode="<<mode<<" scenario="<<scenarioId<<std::endl;
    }
    realm.Addresses=std::move(addresses);
}
