#include "AuthenticationPackets.h"
#include "Realm.h"
struct SpiritAllowedTag{using Type=std::set<ObjectGuid::LowType> WorldSession::*;friend Type Access(SpiritAllowedTag);};
template struct FixtureAccess<SpiritAllowedTag,&WorldSession::_allowedCharsToLogin>;
struct SpiritQueryProcessorTag{using Type=AsyncCallbackProcessor<SQLQueryHolderCallback> WorldSession::*;friend Type Access(SpiritQueryProcessorTag);};
template struct FixtureAccess<SpiritQueryProcessorTag,&WorldSession::_queryHolderProcessor>;
struct SpiritQueryListTag{using Type=std::vector<SQLQueryHolderCallback> AsyncCallbackProcessor<SQLQueryHolderCallback>::*;friend Type Access(SpiritQueryListTag);};
template struct FixtureAccess<SpiritQueryListTag,&AsyncCallbackProcessor<SQLQueryHolderCallback>::_callbacks>;
struct SpiritQueryPumpTag{using Type=void(WorldSession::*)();friend Type Access(SpiritQueryPumpTag);};
template struct FixtureAccess<SpiritQueryPumpTag,&WorldSession::ProcessQueryCallbacks>;
struct SpiritQueryPlayerTag{using Type=Player* WorldSession::*;friend Type Access(SpiritQueryPlayerTag);};
template struct FixtureAccess<SpiritQueryPlayerTag,&WorldSession::_player>;
static void SpiritRequestLogin(WorldSession* session,ObjectGuid guid)
{
    (session->*Access(SpiritAllowedTag{})).insert(guid.GetCounter());
    WorldPackets::Character::PlayerLogin packet{WorldPacket(CMSG_PLAYER_LOGIN)};packet.Guid=guid;
    session->HandlePlayerLoginOpcode(packet);
    Require(session->PlayerLoading()&&!session->GetPlayer()&&session->*Access(ControlLoadingTag{})==guid,"Native login opcode did not accept fixture character");
}
static SQLQueryHolderCallback SpiritQueryResponse(WorldSession* session)
{
    session->HandleContinuePlayerLogin();
    auto& pending=(session->*Access(SpiritQueryProcessorTag{})).*Access(SpiritQueryListTag{});
    Require(pending.size()==1,"Native continue-login did not queue one query callback");
    auto result=std::move(pending.back());pending.clear();
    Require(result.m_future.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Native continue-login query timed out");
    return result; // Defer delivery, retaining production's query results and callback.
}
static void SpiritDeliverQuery(WorldSession* session,SQLQueryHolderCallback&& response)
{
    session->AddQueryHolderCallback(std::move(response));(session->*Access(SpiritQueryPumpTag{}))();
    Require(((session->*Access(SpiritQueryProcessorTag{})).*Access(SpiritQueryListTag{})).empty(),"Ready native query callback was not consumed");
}
#include "TrialSpiritInstanceLinkNative.inc"
#include "TrialSpiritLinkedCancelNative.inc"
#include "TrialSpiritEarlyLoadingNative.inc"
#include "TrialSpiritMapLoadingNative.inc"
#include "TrialSpiritLoadingOrderNative.inc"
static void SpiritQueryLogin(ObjectGuid guid,WorldLocation const& expected,uint64 itemGuid,uint32 durability,WorldSession* oldSession,uint32 difficulty,uint8 race,unsigned direction,unsigned sourceMode)
{
    static bool rsa=false;if(!rsa){Require(WorldPackets::Auth::ConnectTo::InitializeEncryption(),"Native ConnectTo RSA bootstrap failed");rsa=true;}
    auto addresses=realm.Addresses;realm.Addresses={boost::asio::ip::address_v4::loopback(),boost::asio::ip::address_v4::loopback()};
    SpiritMapLoadingLogin(guid,expected,itemGuid,durability,oldSession,difficulty,race,direction,sourceMode);
    SpiritEarlyLoadingLogin(guid,expected,itemGuid,durability,oldSession,difficulty,race,direction,sourceMode);
    SpiritLinkedCancelLogin(guid,expected,itemGuid,durability,oldSession,difficulty,race,direction,sourceMode);
    SpiritInstanceLinkLogin(guid,expected,itemGuid,durability,oldSession,difficulty,race,direction,sourceMode);
    for(unsigned mode=0;mode<7;++mode)
    {
        SpiritLoopback realmSocket,instanceSocket;std::unique_ptr<SpiritLoopback> replacement;
        auto session=new WorldSession(0,"Query",realmSocket.socket,SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
        static bool abi=false;if(!abi){abi=true;std::cout<<"SPIRIT_QUERY_ABI loading="<<(reinterpret_cast<char*>(&(session->*Access(ControlLoadingTag{})))-reinterpret_cast<char*>(session))<<" player="<<(reinterpret_cast<char*>(&(session->*Access(SpiritQueryPlayerTag{})))-reinterpret_cast<char*>(session))<<std::endl;}
        auto shared=WorldSessionPtr(session,[](WorldSession*){});realmSocket.socket->SetWorldSession(shared);instanceSocket.socket->SetWorldSession(shared);session->AddInstanceConnection(instanceSocket.socket);
        auto& sessions=const_cast<SessionMap&>(sWorld->GetAllSessions());auto found=sessions.find(0);WorldSessionPtr previous;
        if(found!=sessions.end()){previous=found->second;Require(previous.get()==oldSession,"Query login would replace unrelated account");}sessions[0]=shared;
        SpiritRequestLogin(session,guid);auto response=SpiritQueryResponse(session);
        auto initialSerial=session->*Access(SpiritSessionSerialTag{});
        auto other=ObjectGuid::Create<HighGuid::Player>(guid.GetCounter()+100000000);
        if(mode==0||mode==2||mode==3||mode==5)session->AbortLogin(WorldPackets::Character::LoginFailureReason::NoWorld);
        if(mode==2||mode==3||mode==5)SpiritRequestLogin(session,mode==3?other:guid);
        if(mode==2||mode==5){replacement=std::make_unique<SpiritLoopback>();replacement->socket->SetWorldSession(shared);replacement->socket.get()->*Access(SpiritLinkTypeTag{})=CONNECTION_TYPE_INSTANCE;session->AddInstanceConnection(replacement->socket);}
        auto requestSerial=session->*Access(SpiritSessionSerialTag{});
        std::unique_ptr<SQLQueryHolderCallback> second;
        if(mode==4||mode==5)second=std::make_unique<SQLQueryHolderCallback>(SpiritQueryResponse(session));
        if(mode==5)SpiritDeliverQuery(session,std::move(*second));
        if(mode==6){SpiritLoadingScreen(session,expected.GetMapId(),false);Require(session->PlayerLoading()&&!session->GetPlayer()&&session->HasInstanceConnection(),"Early loading notification consumed the pending SQL request");}
        auto current=session->GetPlayer();auto serial=session->*Access(SpiritSessionSerialTag{});
        SpiritDeliverQuery(session,std::move(response));
        Require(realmSocket.socket->IsOpen()&&!session->IsforceExit(),"Stale query result kicked the healthy realm session");
        if(mode==0||mode==1||mode==3){
            auto before=session->GetPlayer();auto beforeSerial=session->*Access(SpiritSessionSerialTag{});
            LoginQueryHolder rejected(0,guid);session->HandlePlayerLogin(rejected);
            Require(session->GetPlayer()==before&&session->*Access(SpiritSessionSerialTag{})==beforeSerial,"Direct cancelled/duplicate/different-GUID handler entered LoadFromDB");
        }
        if(mode==0){Require(!session->GetPlayer()&&!session->PlayerLoading()&&!session->GetMap()&&!ObjectAccessor::FindPlayer(guid)&&session->*Access(SpiritSessionSerialTag{})==serial,"Cancelled query result created a Player or changed session state");}
        if(mode==2||mode==3){Require(!session->GetPlayer()&&session->PlayerLoading()&&session->*Access(ControlLoadingTag{})==(mode==3?other:guid)&&!session->GetMap()&&!ObjectAccessor::FindPlayer(guid)&&session->*Access(SpiritSessionSerialTag{})==requestSerial,"Old query result consumed or replaced a newer pending login");}
        if(mode==2){auto fresh=SpiritQueryResponse(session);SpiritDeliverQuery(session,std::move(fresh));}
        if(mode==3)session->AbortLogin(WorldPackets::Character::LoginFailureReason::NoWorld);
        if(mode==4){current=session->GetPlayer();serial=session->*Access(SpiritSessionSerialTag{});SpiritDeliverQuery(session,std::move(*second));Require(session->GetPlayer()==current&&session->*Access(SpiritSessionSerialTag{})==serial,"Duplicate query callback replaced the staged Player");}
        if(mode==5)Require(session->GetPlayer()==current&&session->*Access(SpiritSessionSerialTag{})==serial,"Old query callback replaced a newer same-GUID Player");
        if(mode==1||mode==2||mode==4||mode==5||mode==6){
            auto player=session->GetPlayer();Require(player&&player->GetGUID()==guid&&player->IsChangeMap()&&!player->IsInWorld(),"Current query result did not stage its native Player");
            Require(session->*Access(SpiritSessionSerialTag{})!=initialSerial,"Constructed Player did not invalidate other query completions");
            auto map=player->GetMap();SpiritFarFreeze(player->GetMapId());map->UpdateSessions(99);Require(player->IsChangeMap()&&!player->IsInWorld(),"Query login completion ran early");map->UpdateSessions(1);SpiritFarFreeze(expected.GetMapId());
            auto item=player->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGuid));
            Require(player->IsInWorld()&&!player->IsChangeMap()&&ObjectAccessor::FindPlayer(guid)==player&&player->GetHealth()==321&&player->GetMapId()==expected.GetMapId()&&player->GetExactDist(expected)<0.01f&&player->HasAura(15007)&&player->GetAura(15007)->GetDuration()==598000&&item&&item->GetUInt32Value(ITEM_FIELD_DURABILITY)==durability,"Query login did not complete unchanged saved recovery");
            SpiritLoadingScreen(session,expected.GetMapId(),false);session->SetMap(nullptr);session->LogoutPlayer(true);
        }
        session->m_Functions.Update(1000);Require(!session->GetPlayer()&&!session->PlayerLoading()&&!ObjectAccessor::FindPlayer(guid),"Query login cleanup retained Player/loading state");session->SetMap(nullptr);FlushWrites();
        Require(realmSocket.socket->IsOpen(),"Query login cleanup closed the realm connection");
        if(previous)sessions[0]=previous;else sessions.erase(0);
        auto saved=CharacterDatabase.Query(Trinity::StringFormat("SELECT health,online FROM characters WHERE guid=%u",guid.GetCounter()).c_str());Require(saved&&saved->Fetch()[0].GetUInt64()==321&&!saved->Fetch()[1].GetBool(),"Query login cleanup changed saved health/offline state");
        std::cout<<"PASS: spirit query login diff="<<difficulty<<" race="<<uint32(race)<<" direction="<<direction<<" source="<<sourceMode<<" mode="<<mode<<" map="<<expected.GetMapId()<<std::endl;
    }
    realm.Addresses=std::move(addresses);
}
