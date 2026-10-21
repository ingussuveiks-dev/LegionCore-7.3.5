#include "CharacterPackets.h"
struct SpiritSessionSerialTag{using Type=uint8 WorldSession::*;friend Type Access(SpiritSessionSerialTag);};
template struct FixtureAccess<SpiritSessionSerialTag,&WorldSession::m_loginCompletionSerial>;
struct SpiritSessionFloodTag{using Type=uint8 WorldSession::*;friend Type Access(SpiritSessionFloodTag);};
template struct FixtureAccess<SpiritSessionFloodTag,&WorldSession::playerLoginCounter>;
static void SpiritLoadingScreen(WorldSession* session,uint32 map,bool showing)
{
    WorldPacket wire(CMSG_LOADING_SCREEN_NOTIFY);wire<<int32(map);wire.WriteBit(showing);wire.FlushBits();
    WorldPackets::Character::LoadingScreenNotify packet(std::move(wire));packet.Read();session->HandleLoadScreenOpcode(packet);
}
static void SpiritSessionLogin(ObjectGuid guid,WorldLocation const& expected,uint64 itemGuid,uint32 durability,WorldSession* oldSession,uint32 difficulty,uint8 race,unsigned direction,unsigned sourceMode)
{
    for(unsigned mode=0;mode<5;++mode)
    {
        auto holder=std::make_shared<LoginQueryHolder>(0,guid);Require(holder->Initialize(),"Session login query holder initialization failed");
        auto query=CharacterDatabase.DelayQueryHolder(holder);Require(query.m_future.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Session login query holder timeout");query.m_future.get();
        auto session=Session();session->*Access(ControlLoadingTag{})=guid;
        Require(session->*Access(SpiritSessionSerialTag{})==0,"Session completion serial not initialized");
        static bool abi=false;if(!abi){abi=true;std::cout<<"SPIRIT_SESSION_ABI serial="<<(reinterpret_cast<char*>(&(session->*Access(SpiritSessionSerialTag{})))-reinterpret_cast<char*>(session))<<" flood="<<(reinterpret_cast<char*>(&(session->*Access(SpiritSessionFloodTag{})))-reinterpret_cast<char*>(session))<<std::endl;}
        SpiritLoopback realm,instanceSocket;std::unique_ptr<SpiritLoopback> replacement;auto shared=WorldSessionPtr(session,[](WorldSession*){});
        realm.socket->SetWorldSession(shared);instanceSocket.socket->SetWorldSession(shared);
        auto& sockets=session->*Access(SpiritSocketsTag{});sockets[CONNECTION_TYPE_REALM]=realm.socket;sockets[CONNECTION_TYPE_INSTANCE]=instanceSocket.socket;session->ResetTimeOutTime();
        auto& sessions=const_cast<SessionMap&>(sWorld->GetAllSessions());auto found=sessions.find(0);WorldSessionPtr previous;
        if(found!=sessions.end()){previous=found->second;Require(previous.get()==oldSession,"Session login would replace unrelated account");}
        sessions[0]=shared;
        session->HandlePlayerLogin(*holder);auto player=session->GetPlayer();
        Require(session->*Access(SpiritSessionSerialTag{})==1,"Successful login did not establish completion serial");
        Require(player&&player->GetGUID()==guid&&player->IsAlive()&&player->GetHealth()==321&&player->IsChangeMap()&&!player->IsInWorld(),"Session login did not stage its actual loaded Player");
        Require(player->HasAura(15007)&&player->GetAura(15007)->GetDuration()==598000,"Session login did not retain active resurrection sickness");
        auto map=player->GetMap();SpiritFarFreeze(player->GetMapId());
        Require(session->GetMap()==map&&player->GetMapId()==expected.GetMapId()&&player->GetExactDist(expected)<0.01f&&!ObjectAccessor::FindPlayer(guid),"Session login staged wrong map/position/lookup");
        session->HandleWorldPortAck();oldSession->HandleWorldPortAck();
        Require(session->GetPlayer()==player&&player->IsChangeMap()&&!player->IsInWorld(),"Pre-completion worldport ACK admitted or replaced login Player");
        if(mode==1){session->*Access(SpiritSessionFloodTag{})=1;SpiritLoadingScreen(session,expected.GetMapId(),true);SpiritLoadingScreen(session,expected.GetMapId(),false);}
        if(mode==2)realm.PeerClose();
        if(mode==4){
            map->UpdateSessions(50);Require(player->IsChangeMap()&&!player->IsInWorld(),"First login completed before replacement setup");
            session->SetMap(nullptr);session->LogoutPlayer(true);FlushWrites();
            Require(!session->GetPlayer()&&!ObjectAccessor::FindPlayer(guid),"Replacement login did not delete previous Player");
            replacement=std::make_unique<SpiritLoopback>();replacement->socket->SetWorldSession(shared);session->AddInstanceConnection(replacement->socket);
            session->*Access(ControlLoadingTag{})=guid;
            auto next=std::make_shared<LoginQueryHolder>(0,guid);Require(next->Initialize(),"Replacement query initialization failed");auto pending=CharacterDatabase.DelayQueryHolder(next);Require(pending.m_future.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Replacement query timed out");pending.m_future.get();
            session->HandlePlayerLogin(*next);player=session->GetPlayer();Require(player&&player->IsChangeMap()&&!player->IsInWorld()&&session->*Access(SpiritSessionSerialTag{})==2,"Replacement login not staged with a new completion serial");map=player->GetMap();SpiritFarFreeze(player->GetMapId());
            map->UpdateSessions(50);
            Require(session->GetPlayer()==player&&player->IsChangeMap()&&!player->IsInWorld()&&!ObjectAccessor::FindPlayer(guid),"Old login callback admitted the replacement Player at the old deadline");
        }
        if(mode==3){session->SetMap(nullptr);session->LogoutPlayer(true);Require(!session->GetPlayer()&&!ObjectAccessor::FindPlayer(guid),"Pre-callback session logout retained Player");session->m_Functions.Update(100);}
        else{
            map->UpdateSessions(mode==4?49:99);Require(session->GetPlayer()==player&&player->IsChangeMap()&&!player->IsInWorld(),"Login callback ran before its100ms boundary");
            map->UpdateSessions(1);SpiritFarFreeze(expected.GetMapId());
            Require(session->GetPlayer()==player&&player->IsInWorld()&&!player->IsChangeMap()&&ObjectAccessor::FindPlayer(guid)==player&&map->FindSession(0).get()==session,"Native delayed login callback did not complete map/session/lookup admission");
            Require(player->GetHealth()==321&&!player->GetCorpse()&&player->GetUnitBeingMoved()==player&&!player->IsBeingTeleported()&&player->HasAura(15007)&&player->GetAura(15007)->GetDuration()==598000,"Session login completion lost saved health/corpse/mover/sickness state");
            if(mode==2){map->UpdateSessions(1);Require(!session->GetPlayer()&&!session->GetMap(),"Transport loss during login did not clean up after the delayed callback");}
            else{
                SpiritLoadingScreen(session,expected.GetMapId(),false);SpiritLoadingScreen(session,expected.GetMapId(),false);session->HandleWorldPortAck();oldSession->HandleWorldPortAck();session->SendSpiritResurrect();
                auto item=player->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGuid));
                Require(!session->PlayerLoading()&&session->GetPlayer()==player&&player->IsAlive()&&player->GetHealth()==321&&player->GetMapId()==expected.GetMapId()&&player->GetExactDist(expected)<0.01f&&item&&item->GetUInt32Value(ITEM_FIELD_DURABILITY)==durability,"Repeated loading/worldport/resurrection notifications changed completed login");
                player->UpdateStatsByMask();map->UpdateSessions(1);
                Require(session->GetPlayer()==player&&player->IsInWorld()&&player->GetHealth()==321&&player->GetUnitBeingMoved()==player&&!player->IsBeingTeleported(),"First completed-login session/stat tick changed recovery");
                session->SetMap(nullptr);session->LogoutPlayer(true);
            }
        }
        Require(!session->GetPlayer()&&!ObjectAccessor::FindPlayer(guid),"Session login cleanup retained Player");session->SetMap(nullptr);session->m_Functions.Update(1000);
        SpiritLoadingScreen(session,expected.GetMapId(),false);session->HandleWorldPortAck();Require(!session->GetPlayer()&&!session->PlayerLoading(),"Stale post-logout callbacks recreated login Player");FlushWrites();
        if(previous)sessions[0]=previous;else sessions.erase(0);
        auto row=CharacterDatabase.Query(Trinity::StringFormat("SELECT map,position_x,position_y,position_z,health,playerFlags,online FROM characters WHERE guid=%u",guid.GetCounter()).c_str());Require(bool(row),"Session login save missing");auto f=row->Fetch();
        Require(f[0].GetUInt32()==expected.GetMapId()&&std::abs(f[1].GetFloat()-expected.GetPositionX())<0.01f&&std::abs(f[2].GetFloat()-expected.GetPositionY())<0.01f&&std::abs(f[3].GetFloat()-expected.GetPositionZ())<0.01f&&f[4].GetUInt64()==321&&!(f[5].GetUInt32()&PLAYER_FLAGS_GHOST)&&!f[6].GetBool(),"Session login cleanup lost saved recovery or offline state");
        auto sickness=CharacterDatabase.Query(Trinity::StringFormat("SELECT COUNT(*) FROM character_aura WHERE guid=%u AND spell=15007 AND remaintime=598000",guid.GetCounter()).c_str());Require(sickness&&sickness->Fetch()[0].GetUInt32()==1,"Session login changed saved sickness");
        std::cout<<"PASS: spirit session login diff="<<difficulty<<" race="<<uint32(race)<<" direction="<<direction<<" source="<<sourceMode<<" mode="<<mode<<" map="<<expected.GetMapId()<<std::endl;
    }
}
