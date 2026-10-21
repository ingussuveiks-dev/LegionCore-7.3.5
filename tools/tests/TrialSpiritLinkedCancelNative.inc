struct SpiritCancelAuthedTag{using Type=bool WorldSocket::*;friend Type Access(SpiritCancelAuthedTag);};
template struct FixtureAccess<SpiritCancelAuthedTag,&WorldSocket::_authed>;
struct SpiritCancelAccountTag{using Type=uint32 WorldSocket::*;friend Type Access(SpiritCancelAccountTag);};
template struct FixtureAccess<SpiritCancelAccountTag,&WorldSocket::_accountId>;
struct SpiritCancelLockTag{using Type=std::mutex WorldSocket::*;friend Type Access(SpiritCancelLockTag);};
template struct FixtureAccess<SpiritCancelLockTag,&WorldSocket::_worldSessionLock>;
static void SpiritCancelOwnerRelease()
{
    static bool checked=false;if(checked)return;checked=true;
    for(unsigned kind=0;kind<2;++kind){
        SpiritLoopback transport;auto raw=new WorldSession(123456789,"Owner",{},SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});bool outside=false;
        auto owner=WorldSessionPtr(raw,[socket=transport.socket,&outside](WorldSession*){
            std::thread contender([&]{auto& mutex=socket.get()->*Access(SpiritCancelLockTag{});outside=mutex.try_lock();if(outside)mutex.unlock();});contender.join();
        });
        transport.socket->SetWorldSession(owner);owner.reset();
        Require(transport.socket.get()->*Access(SpiritCancelAccountTag{})==123456789&&transport.socket.get()->*Access(SpiritCancelAuthedTag{}),"Native nonzero socket account association failed");
        if(kind)transport.socket->CloseSocket();else transport.socket->SetWorldSession(nullptr);
        Require(outside&&!(transport.socket.get()->*Access(SpiritSocketSessionTag{})),"Socket released its last session owner under the association mutex");
        if(!kind)Require(!(transport.socket.get()->*Access(SpiritCancelAuthedTag{}))&&!(transport.socket.get()->*Access(SpiritCancelAccountTag{})),"Native socket detach retained authentication/account state");
        Require(raw->IsforceExit()==bool(kind),"Intentional detach and real close lost their distinct teardown behavior");
        std::cout<<"PASS: native socket owner release kind="<<kind<<std::endl;
    }
}
static void SpiritRetiredInstance(SpiritLoopback& retired,WorldSession* session,bool cancelled)
{
    Require(!(session->*Access(SpiritSocketsTag{}))[CONNECTION_TYPE_INSTANCE]&&!session->HasInstanceConnection()&&!retired.socket->IsOpen(),"Cancelled/retired login retained its instance socket");
    Require(!(retired.socket.get()->*Access(SpiritSocketSessionTag{}))&&!(retired.socket.get()->*Access(SpiritCancelAuthedTag{})),"Retired instance retained authenticated session ownership");
    if(!cancelled){boost::system::error_code error;retired.peer.shutdown(boost::asio::ip::tcp::socket::shutdown_both,error);retired.peer.close(error);}
    retired.io.run_for(std::chrono::milliseconds(250));
    Require(retired.io.stopped(),"Cancelled instance left its asynchronous read alive");
    Require(!session->GetPlayer()&&!session->PlayerLoading()&&!session->IsforceExit()&&(session->*Access(SpiritSocketsTag{}))[CONNECTION_TYPE_REALM]->IsOpen(),"Intentional instance retirement kicked the healthy realm or retained login state");
}
static void SpiritCancelComplete(WorldSession* session,ObjectGuid guid,WorldLocation const& expected,uint64 itemGuid,uint32 durability)
{
    auto player=session->GetPlayer();Require(player&&player->GetGUID()==guid&&player->IsChangeMap()&&!player->IsInWorld(),"Retry SQL did not stage its native Player");
    auto map=player->GetMap();SpiritFarFreeze(player->GetMapId());map->UpdateSessions(99);Require(player->IsChangeMap()&&!player->IsInWorld(),"Retry completion ran before100ms");map->UpdateSessions(1);SpiritFarFreeze(expected.GetMapId());
    auto item=player->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGuid));
    Require(player->IsInWorld()&&ObjectAccessor::FindPlayer(guid)==player&&!player->IsChangeMap()&&player->GetHealth()==321&&player->GetMapId()==expected.GetMapId()&&player->GetExactDist(expected)<0.01f&&player->HasAura(15007)&&player->GetAura(15007)->GetDuration()==598000&&item&&item->GetUInt32Value(ITEM_FIELD_DURABILITY)==durability&&player->GetUnitBeingMoved()==player&&!player->GetCorpse(),"Retry completion changed saved recovery/mover/corpse/inventory state");
    SpiritLoadingScreen(session,expected.GetMapId(),false);
}
static void SpiritLinkedCancelLogin(ObjectGuid guid,WorldLocation const& expected,uint64 itemGuid,uint32 durability,WorldSession* oldSession,uint32 difficulty,uint8 race,unsigned direction,unsigned sourceMode)
{
    SpiritCancelOwnerRelease();
    for(unsigned mode=0;mode<7;++mode)
    {
        SpiritLoopback realmSocket,original,retry,third,stale;
        auto session=new WorldSession(0,"Cancel",realmSocket.socket,SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
        static bool abi=false;if(!abi){abi=true;std::cout<<"SPIRIT_CANCEL_ABI instance="<<(reinterpret_cast<char*>(&(session->*Access(SpiritSocketsTag{}))[CONNECTION_TYPE_INSTANCE])-reinterpret_cast<char*>(session))<<" authed="<<(reinterpret_cast<char*>(&(original.socket.get()->*Access(SpiritCancelAuthedTag{})))-reinterpret_cast<char*>(original.socket.get()))<<" account="<<(reinterpret_cast<char*>(&(original.socket.get()->*Access(SpiritCancelAccountTag{})))-reinterpret_cast<char*>(original.socket.get()))<<std::endl;}
        auto shared=WorldSessionPtr(session,[](WorldSession*){});realmSocket.socket->SetWorldSession(shared);
        auto& sessions=const_cast<SessionMap&>(sWorld->GetAllSessions());auto found=sessions.find(0);WorldSessionPtr previous;
        if(found!=sessions.end()){previous=found->second;Require(previous.get()==oldSession,"Linked-cancel fixture would replace unrelated session");}sessions[0]=shared;
        SpiritRequestLogin(session,guid);auto oldKey=session->GetConnectToInstanceKey();SpiritLinkSocket(original,oldKey);auto oldResponse=SpiritTakeLinkQuery(session);
        if(mode>=5){
            if(mode==5)original.PeerClose();else realmSocket.PeerClose();
            Require(session->IsforceExit(),"Native EOF did not mark genuine teardown before cancellation");
            session->AbortLogin(WorldPackets::Character::LoginFailureReason::NoWorld);SpiritDeliverQuery(session,std::move(oldResponse));
            Require(session->IsforceExit()&&!session->GetPlayer()&&!session->PlayerLoading()&&!realmSocket.socket->IsOpen()&&!original.socket->IsOpen()&&!(session->*Access(SpiritSocketsTag{}))[CONNECTION_TYPE_INSTANCE],"Cancellation cleared real teardown or left its realm/instance open");
            sWorld->UpdateSessions(1);Require(!sessions.count(0)&&!ObjectAccessor::FindPlayer(guid)&&SpiritLinkQueries(session).empty(),"Cancelled teardown retained world session/Player/query");
            if(previous)sessions[0]=previous;else sessions.erase(0);
            std::cout<<"PASS: spirit linked cancel diff="<<difficulty<<" race="<<uint32(race)<<" direction="<<direction<<" source="<<sourceMode<<" mode="<<mode<<" map="<<expected.GetMapId()<<std::endl;continue;
        }
        original.socket->AsyncRead();
        if(mode==4){
            SpiritDeliverQuery(session,std::move(oldResponse));SpiritCancelComplete(session,guid,expected,itemGuid,durability);session->SetMap(nullptr);session->LogoutPlayer(true);FlushWrites();
        }else session->AbortLogin(WorldPackets::Character::LoginFailureReason::NoWorld);
        SpiritRetiredInstance(original,session,mode!=4);
        if(mode==0){SpiritDeliverQuery(session,std::move(oldResponse));Require(!session->GetPlayer()&&!session->PlayerLoading(),"Aborted linked SQL recreated a Player before retry");}
        SpiritRequestLogin(session,guid);SpiritLinkSocket(stale,oldKey);SpiritRejectedLink(stale,session);
        Require(!session->HasInstanceConnection()&&SpiritLinkQueries(session).empty()&&session->PlayerLoading(),"Old cancelled connection key consumed retry");
        SpiritLinkSocket(retry,session->GetConnectToInstanceKey());auto response=SpiritTakeLinkQuery(session);SpiritLoopback* active=&retry;
        if(mode==3){
            retry.socket->AsyncRead();session->AbortLogin(WorldPackets::Character::LoginFailureReason::NoWorld);SpiritRetiredInstance(retry,session,true);
            SpiritRequestLogin(session,guid);SpiritLinkSocket(third,session->GetConnectToInstanceKey());auto finalResponse=SpiritTakeLinkQuery(session);active=&third;
            SpiritDeliverQuery(session,std::move(oldResponse));SpiritDeliverQuery(session,std::move(response));response=std::move(finalResponse);
            Require(!session->GetPlayer()&&session->PlayerLoading()&&session->HasInstanceConnection(),"Two cancelled SQL responses consumed the third login");
        }
        if(mode==1){SpiritDeliverQuery(session,std::move(oldResponse));Require(!session->GetPlayer()&&session->PlayerLoading(),"Cancelled linked SQL consumed a new same-GUID request");}
        session->m_Functions.Update(12000);Require(session->HasInstanceConnection()&&!session->IsforceExit()&&session->PlayerLoading(),"Old deadline/retired socket aborted linked retry");
        original.socket->CloseSocket();Require(active->socket->IsOpen()&&!session->IsforceExit(),"Old socket close kicked the replacement instance");
        SpiritDeliverQuery(session,std::move(response));auto current=session->GetPlayer();auto serial=session->*Access(SpiritSessionSerialTag{});
        if(mode==2){SpiritDeliverQuery(session,std::move(oldResponse));Require(session->GetPlayer()==current&&session->*Access(SpiritSessionSerialTag{})==serial,"Cancelled linked SQL replaced the new same-GUID Player");}
        SpiritCancelComplete(session,guid,expected,itemGuid,durability);session->SetMap(nullptr);session->LogoutPlayer(true);
        session->m_Functions.Update(1000);Require(!session->GetPlayer()&&!ObjectAccessor::FindPlayer(guid)&&SpiritLinkQueries(session).empty()&&!session->IsforceExit()&&realmSocket.socket->IsOpen(),"Linked-cancel cleanup retained Player/query or kicked realm");session->SetMap(nullptr);FlushWrites();
        if(previous)sessions[0]=previous;else sessions.erase(0);
        auto saved=CharacterDatabase.Query(Trinity::StringFormat("SELECT health,online FROM characters WHERE guid=%u",guid.GetCounter()).c_str());Require(saved&&saved->Fetch()[0].GetUInt64()==321&&!saved->Fetch()[1].GetBool(),"Linked-cancel cleanup changed saved health/offline state");
        std::cout<<"PASS: spirit linked cancel diff="<<difficulty<<" race="<<uint32(race)<<" direction="<<direction<<" source="<<sourceMode<<" mode="<<mode<<" map="<<expected.GetMapId()<<std::endl;
    }
}
