// Loading notifications do not identify a login request. Before a Player
// exists they must not consume the accepted request or its instance socket.
static void SpiritEarlyNotification(WorldSession* session,uint32 map,bool repeated)
{
    auto loading=session->*Access(ControlLoadingTag{});auto serial=session->*Access(SpiritSessionSerialTag{});auto key=session->GetConnectToInstanceKey();
    auto instance=(session->*Access(SpiritSocketsTag{}))[CONNECTION_TYPE_INSTANCE];auto queries=SpiritLinkQueries(session).size();
    Require(!session->GetPlayer(),"Early notification control already has a Player");
    if(repeated){SpiritLoadingScreen(session,map==0?1:0,true);SpiritLoadingScreen(session,map==0?1:0,false);}
    SpiritLoadingScreen(session,map,false);if(repeated)SpiritLoadingScreen(session,map,false);
    Require(session->*Access(ControlLoadingTag{})==loading&&session->PlayerLoading()==!loading.IsEmpty()&&session->*Access(SpiritSessionSerialTag{})==serial&&session->GetConnectToInstanceKey()==key&&!session->GetPlayer()&&!session->GetMap()&&(session->*Access(SpiritSocketsTag{}))[CONNECTION_TYPE_INSTANCE]==instance&&SpiritLinkQueries(session).size()==queries&&!session->IsforceExit(),"Premature loading-finished notification consumed the pending login or changed its connection");
    if(instance)Require(instance->IsOpen()&&(instance.get()->*Access(SpiritSocketSessionTag{})).get()==session,"Premature loading notification detached the accepted instance socket");
}
static void SpiritEarlyLoadingLogin(ObjectGuid guid,WorldLocation const& expected,uint64 itemGuid,uint32 durability,WorldSession* oldSession,uint32 difficulty,uint8 race,unsigned direction,unsigned sourceMode)
{
    for(unsigned mode=0;mode<7;++mode)
    {
        SpiritLoopback realmSocket,original,retry,stale;
        auto session=new WorldSession(0,"Early",realmSocket.socket,SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
        auto shared=WorldSessionPtr(session,[](WorldSession*){});realmSocket.socket->SetWorldSession(shared);
        auto& sessions=const_cast<SessionMap&>(sWorld->GetAllSessions());auto found=sessions.find(0);WorldSessionPtr previous;
        if(found!=sessions.end()){previous=found->second;Require(previous.get()==oldSession,"Early-loading fixture would replace unrelated session");}sessions[0]=shared;
        if(mode==5)SpiritEarlyNotification(session,expected.GetMapId(),true);
        SpiritRequestLogin(session,guid);auto key=session->GetConnectToInstanceKey();
        if(mode==1||mode==2)SpiritEarlyNotification(session,expected.GetMapId(),mode==2);
        SpiritLinkSocket(original,key);auto response=SpiritTakeLinkQuery(session);
        if(mode!=4)SpiritEarlyNotification(session,expected.GetMapId(),mode==2);
        SpiritLoopback* active=&original;
        if(mode==3||mode==4){
            original.socket->AsyncRead();session->AbortLogin(WorldPackets::Character::LoginFailureReason::NoWorld);SpiritRetiredInstance(original,session,true);
            auto oldResponse=std::move(response);SpiritRequestLogin(session,guid);
            SpiritEarlyNotification(session,expected.GetMapId(),true);
            SpiritLinkSocket(stale,key);SpiritRejectedLink(stale,session);
            SpiritLinkSocket(retry,session->GetConnectToInstanceKey());response=SpiritTakeLinkQuery(session);active=&retry;
            SpiritEarlyNotification(session,expected.GetMapId(),true);SpiritDeliverQuery(session,std::move(oldResponse));
            Require(!session->GetPlayer()&&session->PlayerLoading()&&session->HasInstanceConnection(),"Old loading/SQL callbacks consumed replacement login");
        }
        session->m_Functions.Update(12000);Require(session->PlayerLoading()&&session->HasInstanceConnection()&&active->socket->IsOpen()&&!session->IsforceExit(),"Early loading notification stranded login before SQL delivery");
        SpiritDeliverQuery(session,std::move(response));
        if(mode==6){
            auto player=session->GetPlayer();SpiritLoadingScreen(session,expected.GetMapId(),true);Require(session->PlayerLoading(),"Loading-start acknowledgement consumed the constructed Player's pending login");SpiritLoadingScreen(session,expected.GetMapId(),false);
            Require(player&&session->GetPlayer()==player&&!session->PlayerLoading()&&player->IsChangeMap()&&!player->IsInWorld(),"Constructed Player no longer admits early loading acknowledgement");
        }
        SpiritCancelComplete(session,guid,expected,itemGuid,durability);session->SetMap(nullptr);session->LogoutPlayer(true);FlushWrites();
        if(mode==5){
            SpiritEarlyNotification(session,expected.GetMapId(),true);SpiritRequestLogin(session,guid);SpiritLinkSocket(retry,session->GetConnectToInstanceKey());auto next=SpiritTakeLinkQuery(session);
            SpiritEarlyNotification(session,expected.GetMapId(),true);SpiritDeliverQuery(session,std::move(next));SpiritCancelComplete(session,guid,expected,itemGuid,durability);session->SetMap(nullptr);session->LogoutPlayer(true);FlushWrites();
        }
        session->m_Functions.Update(1000);Require(!session->GetPlayer()&&!session->PlayerLoading()&&!ObjectAccessor::FindPlayer(guid)&&SpiritLinkQueries(session).empty()&&!session->IsforceExit()&&realmSocket.socket->IsOpen(),"Early-loading cleanup retained a Player/query or kicked the realm");session->SetMap(nullptr);
        if(previous)sessions[0]=previous;else sessions.erase(0);
        auto saved=CharacterDatabase.Query(Trinity::StringFormat("SELECT health,online FROM characters WHERE guid=%u",guid.GetCounter()).c_str());Require(saved&&saved->Fetch()[0].GetUInt64()==321&&!saved->Fetch()[1].GetBool(),"Early-loading cleanup changed saved health/offline state");
        std::cout<<"PASS: spirit early loading diff="<<difficulty<<" race="<<uint32(race)<<" direction="<<direction<<" source="<<sourceMode<<" mode="<<mode<<" map="<<expected.GetMapId()<<std::endl;
    }
}
