// Real loopback transports at the post-authentication World link boundary.
using SpiritLinkInfo=std::pair<std::weak_ptr<WorldSocket>,uint64>;
struct SpiritLinkQueueTag{using Type=LockedQueue<SpiritLinkInfo> World::*;friend Type Access(SpiritLinkQueueTag);};
template struct FixtureAccess<SpiritLinkQueueTag,&World::_linkSocketQueue>;
struct SpiritLinkDispatchTag{using Type=void(World::*)(SpiritLinkInfo);friend Type Access(SpiritLinkDispatchTag);};
template struct FixtureAccess<SpiritLinkDispatchTag,&World::ProcessLinkInstanceSocket>;
struct SpiritLinkTypeTag{using Type=ConnectionType WorldSocket::*;friend Type Access(SpiritLinkTypeTag);};
template struct FixtureAccess<SpiritLinkTypeTag,&WorldSocket::_type>;
struct SpiritLinkExitTag{using Type=bool WorldSession::*;friend Type Access(SpiritLinkExitTag);};
template struct FixtureAccess<SpiritLinkExitTag,&WorldSession::forceExit>;
static auto& SpiritLinkQueries(WorldSession* session)
{return (session->*Access(SpiritQueryProcessorTag{})).*Access(SpiritQueryListTag{});}
static void SpiritLinkSocket(SpiritLoopback& incoming,uint64 key)
{
    incoming.socket.get()->*Access(SpiritLinkTypeTag{})=CONNECTION_TYPE_INSTANCE;
    sWorld->AddInstanceSocket(incoming.socket,key);SpiritLinkInfo pending;
    auto& queue=sWorld->*Access(SpiritLinkQueueTag{});
    Require(queue.next(pending),"Native instance-link enqueue did not publish socket");
    (sWorld->*Access(SpiritLinkDispatchTag{}))(std::move(pending));
    Require(!queue.next(pending),"Instance-link fixture left another queued socket");
}
static void SpiritRejectedLink(SpiritLoopback& incoming,WorldSession* session)
{
    Require(!incoming.socket->IsOpen()&&!(incoming.socket.get()->*Access(SpiritSocketSessionTag{})),"Rejected instance socket remained open or associated");
    incoming.socket->CloseSocket();
    Require(!session->IsforceExit(),"Rejecting/closing an old instance socket kicked the current realm");
}
static SQLQueryHolderCallback SpiritTakeLinkQuery(WorldSession* session)
{
    auto& queries=SpiritLinkQueries(session);Require(queries.size()==1,"Native instance link did not schedule exactly one login query");
    auto response=std::move(queries.back());queries.clear();
    Require(response.m_future.wait_for(std::chrono::seconds(5))==std::future_status::ready,"Instance-link login query timed out");return response;
}
static void SpiritInstanceLinkLogin(ObjectGuid guid,WorldLocation const& expected,uint64 itemGuid,uint32 durability,WorldSession* oldSession,uint32 difficulty,uint8 race,unsigned direction,unsigned sourceMode)
{
    for(unsigned mode=0;mode<7;++mode)
    {
        SpiritLoopback realmSocket,incoming,stale,duplicate;
        auto session=new WorldSession(0,"Link",realmSocket.socket,SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
        static bool abi=false;if(!abi){abi=true;std::cout<<"SPIRIT_LINK_ABI exit="<<(reinterpret_cast<char*>(&(session->*Access(SpiritLinkExitTag{})))-reinterpret_cast<char*>(session))<<std::endl;}
        auto shared=WorldSessionPtr(session,[](WorldSession*){});realmSocket.socket->SetWorldSession(shared);
        auto& sessions=const_cast<SessionMap&>(sWorld->GetAllSessions());auto found=sessions.find(0);WorldSessionPtr previous;
        if(found!=sessions.end()){previous=found->second;Require(previous.get()==oldSession,"Instance-link fixture would replace unrelated session");}sessions[0]=shared;
        SpiritRequestLogin(session,guid);auto key=session->GetConnectToInstanceKey();
        if(mode==1){session->m_Functions.Update(11999);Require(session->GetConnectToInstanceKey()==key,"Native instance deadline retried before12 seconds");session->m_Functions.Update(1);Require(session->GetConnectToInstanceKey()!=key,"Native instance deadline did not rotate connection key");}
        if(mode==2||mode==3)session->AbortLogin(WorldPackets::Character::LoginFailureReason::NoWorld);
        if(mode==3)SpiritRequestLogin(session,guid);
        if(mode==1||mode==3){SpiritLinkSocket(stale,key);SpiritRejectedLink(stale,session);Require(!session->HasInstanceConnection()&&SpiritLinkQueries(session).empty()&&session->PlayerLoading(),"Stale key consumed the current login or scheduled SQL");key=session->GetConnectToInstanceKey();}
        if(mode==4){stale.socket->CloseSocket();SpiritLinkSocket(stale,key);Require(SpiritLinkQueries(session).empty()&&!session->HasInstanceConnection(),"Closed incoming socket scheduled login");}
        if(mode==5){realmSocket.PeerClose();Require(session->IsforceExit(),"Native realm EOF did not mark teardown");}
        SpiritLinkSocket(incoming,key);
        if(mode==2||mode==5){
            Require(SpiritLinkQueries(session).empty()&&!session->HasInstanceConnection()&&!(incoming.socket.get()->*Access(SpiritSocketSessionTag{}))&&!incoming.socket->IsOpen(),mode==5?"Instance socket was admitted after realm EOF and queued login SQL":"Cancelled login admitted its instance socket");
            if(mode==2)SpiritRejectedLink(incoming,session);
        }else{
            Require(session->HasInstanceConnection()&&(session->*Access(SpiritSocketsTag{}))[CONNECTION_TYPE_INSTANCE]==incoming.socket&&(incoming.socket.get()->*Access(SpiritSocketSessionTag{})).get()==session,"Native current instance socket was not attached");
            auto response=SpiritTakeLinkQuery(session);
            if(mode==6){
                incoming.PeerClose();Require(session->IsforceExit()&&!session->HasInstanceConnection(),"Native instance EOF did not mark teardown");
                SpiritLinkSocket(duplicate,key);
                Require(SpiritLinkQueries(session).empty()&&!(duplicate.socket.get()->*Access(SpiritSocketSessionTag{}))&&!duplicate.socket->IsOpen(),"Replacement instance socket was admitted during teardown");
                // The already-completed SQL response is discarded with this
                // closing fixture; this case isolates admission, not delivery.
            }else{
                SpiritLinkSocket(duplicate,key);SpiritRejectedLink(duplicate,session);
                Require(SpiritLinkQueries(session).empty()&&(session->*Access(SpiritSocketsTag{}))[CONNECTION_TYPE_INSTANCE]==incoming.socket,"Duplicate instance link scheduled SQL or replaced the valid socket");
                session->m_Functions.Update(12000);Require(session->HasInstanceConnection()&&session->PlayerLoading()&&!session->IsforceExit(),"Old native connection deadlines aborted a linked login");
                SpiritDeliverQuery(session,std::move(response));auto player=session->GetPlayer();Require(player&&player->GetGUID()==guid&&player->IsChangeMap()&&!player->IsInWorld(),"Instance-link SQL did not stage its native Player");
                auto map=player->GetMap();SpiritFarFreeze(player->GetMapId());map->UpdateSessions(99);Require(player->IsChangeMap()&&!player->IsInWorld(),"Instance-link completion ran early");map->UpdateSessions(1);SpiritFarFreeze(expected.GetMapId());
                auto item=player->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGuid));
                Require(player->IsInWorld()&&ObjectAccessor::FindPlayer(guid)==player&&!player->IsChangeMap()&&player->GetHealth()==321&&player->GetMapId()==expected.GetMapId()&&player->GetExactDist(expected)<0.01f&&player->HasAura(15007)&&player->GetAura(15007)->GetDuration()==598000&&item&&item->GetUInt32Value(ITEM_FIELD_DURABILITY)==durability,"Instance-link completion changed saved recovery");
                SpiritLoadingScreen(session,expected.GetMapId(),false);session->SetMap(nullptr);session->LogoutPlayer(true);
            }
        }
        if(mode==5||mode==6){sWorld->UpdateSessions(1);Require(!sessions.count(0)&&!session->GetPlayer()&&!realmSocket.socket->IsOpen(),"Native teardown retained world session or realm socket");}
        session->m_Functions.Update(1000);Require(!session->GetPlayer()&&!ObjectAccessor::FindPlayer(guid)&&SpiritLinkQueries(session).empty(),"Instance-link cleanup retained a Player/lookup/query");session->SetMap(nullptr);FlushWrites();
        if(previous)sessions[0]=previous;else sessions.erase(0);
        auto saved=CharacterDatabase.Query(Trinity::StringFormat("SELECT health,online FROM characters WHERE guid=%u",guid.GetCounter()).c_str());Require(saved&&saved->Fetch()[0].GetUInt64()==321&&!saved->Fetch()[1].GetBool(),"Instance-link cleanup changed saved health/offline state");
        std::cout<<"PASS: spirit instance link diff="<<difficulty<<" race="<<uint32(race)<<" direction="<<direction<<" source="<<sourceMode<<" mode="<<mode<<" map="<<expected.GetMapId()<<std::endl;
    }
}
