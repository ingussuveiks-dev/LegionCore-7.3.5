struct SpiritLoadingStateTag{using Type=uint8 WorldSession::*;friend Type Access(SpiritLoadingStateTag);};
template struct FixtureAccess<SpiritLoadingStateTag,&WorldSession::m_mapLoadingState>;
#include "MovementPackets.h"
struct SpiritArrivalMapTag{using Type=int32 Player::*;friend Type Access(SpiritArrivalMapTag);};
template struct FixtureAccess<SpiritArrivalMapTag,&Player::_mapObjectArrivalMap>;
struct SpiritArrivalLoadedTag{using Type=int32 Player::*;friend Type Access(SpiritArrivalLoadedTag);};
template struct FixtureAccess<SpiritArrivalLoadedTag,&Player::_mapObjectArrivalLoadedMap>;
struct SpiritArrivalWaitTag{using Type=uint32 Player::*;friend Type Access(SpiritArrivalWaitTag);};
template struct FixtureAccess<SpiritArrivalWaitTag,&Player::_mapObjectArrivalWait>;
struct SpiritArrivalSettleTag{using Type=uint32 Player::*;friend Type Access(SpiritArrivalSettleTag);};
template struct FixtureAccess<SpiritArrivalSettleTag,&Player::_mapObjectArrivalSettle>;
struct SpiritMapFarTag{using Type=bool Player::*;friend Type Access(SpiritMapFarTag);};
template struct FixtureAccess<SpiritMapFarTag,&Player::mSemaphoreTeleport_Far>;
struct SpiritMapDestTag{using Type=WorldLocation Player::*;friend Type Access(SpiritMapDestTag);};
template struct FixtureAccess<SpiritMapDestTag,&Player::m_teleport_dest>;
struct SpiritPacketQueueTag{using Type=MPSCQueue<EncryptablePacket> WorldSocket::*;friend Type Access(SpiritPacketQueueTag);};
template struct FixtureAccess<SpiritPacketQueueTag,&WorldSocket::_bufferQueue>;
static unsigned SpiritDrainPackets(WorldSession* session,unsigned* initial=nullptr)
{
    unsigned count=0;
    for(auto const& socket:session->*Access(SpiritSocketsTag{}))if(socket){
        EncryptablePacket* packet=nullptr;
        while((socket.get()->*Access(SpiritPacketQueueTag{})).Dequeue(packet)){
            ++count;if(initial&&packet->GetOpcode()==SMSG_WORLD_SERVER_INFO)++*initial;delete packet;
        }
    }
    return count;
}
static void SpiritWrongMap(WorldSession* session,uint32 map,bool showing)
{
    auto player=session->GetPlayer();Require(player,"Wrong-map fixture lacks Player");
    auto loading=session->*Access(ControlLoadingTag{});auto serial=session->*Access(SpiritSessionSerialTag{});
    auto currentMap=player->GetMap();auto sessionMap=session->GetMap();auto position=player->GetPosition();auto health=player->GetHealth();auto mover=player->GetUnitBeingMoved();
    auto arrival=player->*Access(SpiritArrivalMapTag{}),loaded=player->*Access(SpiritArrivalLoadedTag{});
    auto wait=player->*Access(SpiritArrivalWaitTag{}),settle=player->*Access(SpiritArrivalSettleTag{});
    auto pendingFar=player->IsBeingTeleportedFar(),pendingNear=player->IsBeingTeleportedNear(),changing=player->IsChangeMap(),world=player->IsInWorld();
    auto root=player->HasUnitState(UNIT_STATE_ROOT),gravity=player->HasUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY);
    auto instance=(session->*Access(SpiritSocketsTag{}))[CONNECTION_TYPE_INSTANCE];SpiritDrainPackets(session);
    SpiritLoadingScreen(session,map,showing);auto packets=SpiritDrainPackets(session);
    Require(!packets,"Wrong-map loading notification dispatched native outgoing packets");
    Require(session->GetPlayer()==player&&session->*Access(ControlLoadingTag{})==loading&&session->*Access(SpiritSessionSerialTag{})==serial&&session->GetMap()==sessionMap&&player->GetMap()==currentMap&&player->GetExactDist(position)<0.01f&&player->GetHealth()==health&&player->GetUnitBeingMoved()==mover&&player->IsBeingTeleportedFar()==pendingFar&&player->IsBeingTeleportedNear()==pendingNear&&player->IsChangeMap()==changing&&player->IsInWorld()==world,"Wrong-map loading notification changed login/map/mover state");
    Require(player->*Access(SpiritArrivalMapTag{})==arrival&&player->*Access(SpiritArrivalLoadedTag{})==loaded&&player->*Access(SpiritArrivalWaitTag{})==wait&&player->*Access(SpiritArrivalSettleTag{})==settle&&player->HasUnitState(UNIT_STATE_ROOT)==root&&player->HasUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY)==gravity,"Wrong-map loading notification changed arrival acknowledgement or protection");
    Require((session->*Access(SpiritSocketsTag{}))[CONNECTION_TYPE_INSTANCE]==instance&&instance&&instance->IsOpen()&&!session->IsforceExit(),"Wrong-map notification detached the instance connection");
}
static void SpiritValidMap(WorldSession* session,uint32 map)
{
    auto player=session->GetPlayer();bool dispatch=session->*Access(SpiritLoadingStateTag{})!=2&&player->IsInWorld()&&!player->IsChangeMap()&&!player->IsBeingTeleported();
    SpiritDrainPackets(session);SpiritLoadingScreen(session,map,false);unsigned initial=0;SpiritDrainPackets(session,&initial);
    Require(initial==unsigned(dispatch)&&!session->PlayerLoading()&&session->GetPlayer()->*Access(SpiritArrivalLoadedTag{})==int32(map),"Valid loading completion lost its native initial packets/acknowledgement");
}
static void SpiritMapLoadingLogin(ObjectGuid guid,WorldLocation const& expected,uint64 itemGuid,uint32 durability,WorldSession* oldSession,uint32 difficulty,uint8 race,unsigned direction,unsigned sourceMode)
{
    for(unsigned mode=0;mode<7;++mode){
        SpiritLoopback realmSocket,instanceSocket;auto session=new WorldSession(0,"MapLoading",realmSocket.socket,SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
        auto shared=WorldSessionPtr(session,[](WorldSession*){});realmSocket.socket->SetWorldSession(shared);
        auto& sessions=const_cast<SessionMap&>(sWorld->GetAllSessions());auto found=sessions.find(0);WorldSessionPtr previous;
        if(found!=sessions.end()){previous=found->second;Require(previous.get()==oldSession,"Map-loading fixture would replace unrelated session");}sessions[0]=shared;
        SpiritRequestLogin(session,guid);SpiritLinkSocket(instanceSocket,session->GetConnectToInstanceKey());auto response=SpiritTakeLinkQuery(session);SpiritDeliverQuery(session,std::move(response));auto player=session->GetPlayer();Require(player,"Map-loading SQL did not construct Player");
        static bool abi=false;if(!abi){abi=true;std::cout<<"SPIRIT_MAP_LOADING_ABI far="<<(reinterpret_cast<char*>(&(player->*Access(SpiritMapFarTag{})))-reinterpret_cast<char*>(player))<<" destination="<<(reinterpret_cast<char*>(&(player->*Access(SpiritMapDestTag{})).m_mapId)-reinterpret_cast<char*>(player))<<" current="<<(reinterpret_cast<char*>(&player->m_mapId)-reinterpret_cast<char*>(player))<<std::endl;}
        auto wrong=expected.GetMapId()==0?1u:0u;auto final=expected;
        if(mode==0)SpiritWrongMap(session,wrong,false);
        if(mode==1){SpiritValidMap(session,expected.GetMapId());SpiritWrongMap(session,wrong,true);SpiritWrongMap(session,wrong,false);}
        SpiritCancelComplete(session,guid,expected,itemGuid,durability);
        if(mode==2){
            player->EndMapObjectArrival();player->NotifyMapObjectArrivalLoading(int32(expected.GetMapId()));player->BeginMapObjectArrival();Require(player->*Access(SpiritArrivalSettleTag{})==0,"Arrival control did not wait for loading finish");player->NotifyMapObjectArrivalLoaded(int32(expected.GetMapId()));player->UpdateMapObjectArrival(1000);
            Require(player->IsMapObjectArrivalProtected()&&player->*Access(SpiritArrivalSettleTag{})==7000,"Native arrival grace control missing");
            for(unsigned i=0;i<3;++i){SpiritWrongMap(session,wrong,true);SpiritWrongMap(session,wrong,false);SpiritWrongMap(session,uint32(-1),false);}
            player->UpdateMapObjectArrival(6999);Require(player->IsMapObjectArrivalProtected(),"Arrival grace released early");player->UpdateMapObjectArrival(1);Require(!player->IsMapObjectArrivalProtected(),"Arrival grace did not release");
        }
        if(mode>=3&&mode<=5){
            auto destination=player->GetMapId()==1220?sObjectMgr->GetDefaultGraveYard(player->GetTeam()):sWorldSafeLocsStore.LookupEntry(5779);Require(destination&&destination->MapID!=player->GetMapId(),"Far map-loading destination missing");
            auto source=player->GetMapId();final=WorldLocation(destination->MapID,destination->Loc.X,destination->Loc.Y,destination->Loc.Z,1.25f);
            Require(player->TeleportTo(final),"Native map-loading far teleport failed");SpiritFarFreeze(final.GetMapId());
            Require(player->IsBeingTeleportedFar()&&player->GetMapId()==source&&player->GetTeleportDest().GetMapId()==final.GetMapId(),"Far map-loading fixture did not retain old current map");
            if(mode==3){SpiritWrongMap(session,source,false);SpiritWrongMap(session,uint32(-1),true);}
            if(mode!=5){SpiritValidMap(session,final.GetMapId());SpiritWrongMap(session,source,true);SpiritWrongMap(session,source,false);}
            session->HandleWorldPortAck();SpiritFarFreeze(final.GetMapId());player->UpdateZone(player->GetCurrentZoneID(),player->GetCurrentAreaID());SpiritFarFreeze(final.GetMapId());
            if(player->IsBeingTeleportedFar()&&player->IsChangeMap())session->m_Functions.Update(500);
            Require(player->IsInWorld()&&!player->IsBeingTeleported()&&player->GetMapId()==final.GetMapId()&&player->GetExactDist(final)<0.01f,"Map-loading far ACK failed");
            if(mode!=5)Require(player->*Access(SpiritArrivalLoadedTag{})==int32(final.GetMapId()),"Far ACK lost early destination loading acknowledgement");
            SpiritWrongMap(session,source,true);SpiritWrongMap(session,source,false);SpiritValidMap(session,final.GetMapId());
        }
        if(mode==6){
            player->EndMapObjectArrival();final.Relocate(final.GetPositionX()+40,final.GetPositionY(),final.GetPositionZ(),final.GetOrientation());Require(player->TeleportTo(final),"Native map-loading near teleport failed");Require(player->IsBeingTeleportedNear(),"Map-loading near teleport not pending");
            SpiritWrongMap(session,wrong,true);SpiritWrongMap(session,wrong,false);SpiritLoadingScreen(session,final.GetMapId(),true);SpiritValidMap(session,final.GetMapId());
            WorldPackets::Movement::MoveTeleportAck ack{WorldPacket(CMSG_MOVE_TELEPORT_ACK)};ack.MoverGUID=guid;session->HandleMoveTeleportAck(ack);
            Require(!player->IsBeingTeleportedNear()&&player->GetExactDist(final)<0.01f,"Map-loading near ACK failed");SpiritWrongMap(session,uint32(-1),false);
        }
        SpiritValidMap(session,final.GetMapId());auto item=player->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGuid));
        Require(player->GetHealth()==321&&player->HasAura(15007)&&player->GetAura(15007)->GetDuration()==598000&&item&&item->GetUInt32Value(ITEM_FIELD_DURABILITY)==durability&&player->GetUnitBeingMoved()==player&&!player->GetCorpse(),"Map-loading changed health/aura/item/mover state");
        session->SetMap(nullptr);session->LogoutPlayer(true);FlushWrites();session->m_Functions.Update(1000);
        Require(!session->GetPlayer()&&!session->PlayerLoading()&&!ObjectAccessor::FindPlayer(guid)&&SpiritLinkQueries(session).empty()&&!session->IsforceExit()&&realmSocket.socket->IsOpen(),"Map-loading cleanup retained login/query or kicked realm");
        if(previous)sessions[0]=previous;else sessions.erase(0);
        auto saved=CharacterDatabase.Query(Trinity::StringFormat("SELECT health,online,map,position_x,position_y,position_z FROM characters WHERE guid=%u",guid.GetCounter()).c_str());Require(bool(saved),"Map-loading saved character missing");auto f=saved->Fetch();
        Require(f[0].GetUInt64()==321&&!f[1].GetBool()&&f[2].GetUInt32()==final.GetMapId()&&std::abs(f[3].GetFloat()-final.GetPositionX())<0.01f&&std::abs(f[4].GetFloat()-final.GetPositionY())<0.01f&&std::abs(f[5].GetFloat()-final.GetPositionZ())<0.01f,"Map-loading saved state changed");
        // Later cases load the same fixture row at its original recovery anchor.
        CharacterDatabase.DirectExecute(Trinity::StringFormat("UPDATE characters SET map=%u,position_x=%.9g,position_y=%.9g,position_z=%.9g,orientation=%.9g,instance_id=0 WHERE guid=%u",expected.GetMapId(),expected.GetPositionX(),expected.GetPositionY(),expected.GetPositionZ(),expected.GetOrientation(),guid.GetCounter()).c_str());
        std::cout<<"PASS: spirit map loading diff="<<difficulty<<" race="<<uint32(race)<<" direction="<<direction<<" source="<<sourceMode<<" mode="<<mode<<" map="<<expected.GetMapId()<<std::endl;
    }
}
