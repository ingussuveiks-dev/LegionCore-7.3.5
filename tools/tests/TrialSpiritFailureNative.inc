// Real transfer refusal and transport-loss cleanup, without client authentication.
#include "WorldSocket.h"
#include "OpenSSLCrypto.h"
#include <boost/asio/io_context.hpp>
struct SpiritExpansionTag{using Type=uint8 WorldSession::*;friend Type Access(SpiritExpansionTag);};
template struct FixtureAccess<SpiritExpansionTag,&WorldSession::m_expansion>;
struct SpiritSocketsTag{using Type=std::shared_ptr<WorldSocket> (WorldSession::*)[MAX_CONNECTION_TYPES];friend Type Access(SpiritSocketsTag);};
template struct FixtureAccess<SpiritSocketsTag,&WorldSession::m_Socket>;
struct SpiritExpireTag{using Type=uint32 WorldSession::*;friend Type Access(SpiritExpireTag);};
template struct FixtureAccess<SpiritExpireTag,&WorldSession::expireTime>;
struct SpiritSocketSessionTag{using Type=WorldSessionPtr WorldSocket::*;friend Type Access(SpiritSocketSessionTag);};
template struct FixtureAccess<SpiritSocketSessionTag,&WorldSocket::_worldSession>;

struct SpiritLoopback
{
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket peer{io};
    std::shared_ptr<WorldSocket> socket;
    SpiritLoopback()
    {
        using boost::asio::ip::tcp;
        tcp::acceptor listener(io,tcp::endpoint(boost::asio::ip::address_v4::loopback(),0));
        peer.connect(listener.local_endpoint());tcp::socket accepted(io);listener.accept(accepted);
        socket=std::make_shared<WorldSocket>(std::move(accepted));
        Require(socket->IsOpen(),"Spirit loopback transport not open");
    }
    void Attach(WorldSession* session){socket.get()->*Access(SpiritSocketSessionTag{})=WorldSessionPtr(session,[](WorldSession*){});}
    void PeerClose()
    {
        socket->AsyncRead();boost::system::error_code error;
        peer.shutdown(boost::asio::ip::tcp::socket::shutdown_both,error);peer.close(error);
        io.run();Require(!socket->IsOpen(),"Native socket read did not detect peer EOF");
    }
    ~SpiritLoopback(){socket->CloseSocket();}
};
#include "TrialSpiritSessionNative.inc"

static void TrialSpiritFailureCases(GarmPlayer& leader,InstanceMap& raid)
{
    SpiritLoginInputs();
    // Match worldserver Main's provider bootstrap before constructing native
    // WorldSocket packet crypto. The runner's working directory is Release.
    OpenSSLCrypto::threadsSetup(boost::filesystem::path("."));
    std::shared_ptr<void> crypto(nullptr,[](void*){OpenSSLCrypto::threadsCleanup();});
    auto rc4=EVP_CIPHER_fetch(nullptr,"RC4",nullptr);Require(rc4,"Native socket fixture needs the canonical legacy crypto provider");EVP_CIPHER_free(rc4);
    auto islands=sWorldSafeLocsStore.LookupEntry(5779),oldWorld=sObjectMgr->GetDefaultGraveYard(leader.GetTeam());
    Require(islands&&oldWorld,"Spirit failure graveyard anchors missing");
    auto mask=raid.GetInstanceScript()->GetCompletedEncounterMask();
    auto timeout=sWorld->getIntConfig(CONFIG_SOCKET_TIMEOUTTIME);
    sWorld->setIntConfig(CONFIG_SOCKET_TIMEOUTTIME,1000);
    // Mode0 denies expansion access before departure. Modes1/2/3 close realm,
    // expire an idle realm, or close instance transport while far ACK is absent.
    for(unsigned direction=0;direction<2;++direction)for(unsigned mode=0;mode<4;++mode)
    {
        if(mode==0&&direction)continue; // Classic continents have no expansion gate.
        auto body=direction?oldWorld:islands,ghost=direction?islands:oldWorld;
        auto destination=sObjectMgr->GetClosestGraveYard(body->Loc.X,body->Loc.Y,body->Loc.Z,body->MapID,leader.GetTeam());
        auto source=sObjectMgr->GetClosestGraveYard(ghost->Loc.X,ghost->Loc.Y,ghost->Loc.Z,ghost->MapID,leader.GetTeam());
        Require(destination&&source&&destination->MapID!=source->MapID,"Spirit failure destination selection failed");
        auto owned=std::make_unique<GarmPlayer>(*leader.GetMap(),leader.getRace());auto player=owned.get();
        auto guid=player->GetGUID();auto session=player->GetSession();
        std::string loginName="Spirit";auto nameNumber=guid.GetCounter();for(unsigned i=0;i<4;++i){loginName+=char('a'+nameNumber%26);nameNumber/=26;}player->SetName(loginName);
        player->SetPrimarySpecialization(71);player->SetActiveTalentGroup(0);
        player->*Access(ControlSocialTag{})=sSocialMgr->LoadFromDB({},guid);
        SpiritGravePlace(*player,*body);
        sWorld->AddCharacterInfo(guid,0,player->GetName(),GENDER_MALE,player->getRace(),player->getClass(),player->getLevel());
        player->SaveToDB(true);FlushWrites();player->SetMaxHealth(1000);player->KillPlayer();player->BuildPlayerRepop();
        Require(player->GetCorpse(),"Spirit failure corpse input missing");SpiritGravePlace(*player,*source);
        SpiritFarSession registered(*player);
        Actor healer(player->GetMap(),6491,40000+guid.GetCounter(),player->GetPosition());
        Require(player->GetMap()->AddToMap(static_cast<Creature*>(&healer)),"Spirit failure healer spawn failed");
        player->SetInventorySlotCount(16);ItemPosCountVec slots;
        Require(player->CanStoreNewItem(NULL_BAG,NULL_SLOT,slots,25,1)==EQUIP_ERR_OK,"Spirit failure durable item admission failed");
        auto item=player->StoreNewItem(slots,25,true);Require(item&&item->GetUInt32Value(ITEM_FIELD_DURABILITY)>4,"Spirit failure durable item missing");
        auto originalDurability=item->GetUInt32Value(ITEM_FIELD_DURABILITY);auto itemGuid=item->GetGUID().GetCounter();
        if(mode==0)session->*Access(SpiritExpansionTag{})=0;
        SpiritGraveRequest(session,healer.GetGUID());
        Require(player->IsAlive()&&player->GetHealth()==500&&!player->GetCorpse()&&!player->HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&player->HasAura(15007),"Spirit failure resurrection state wrong");
        auto durability=item->GetUInt32Value(ITEM_FIELD_DURABILITY);Require(durability<originalDurability,"Spirit failure did not apply native durability penalty");
        if(mode==0){
            Require(player->IsInWorld()&&!player->IsBeingTeleported()&&player->GetMapId()==source->MapID&&player->GetExactDist(source->Loc.X,source->Loc.Y,source->Loc.Z)<0.01f,"Rejected spirit transfer stranded resurrected player");
        }else{
            SpiritFarFreeze(destination->MapID);
            Require(!player->IsInWorld()&&player->IsBeingTeleportedFar()&&!player->IsChangeMap(),"Spirit failure did not begin pending far transfer");
        }
        auto expected=mode?player->GetTeleportDest():WorldLocation(player->GetMapId(),player->GetPosition());
        auto zone=sMapMgr->GetZoneId(expected.GetMapId(),expected.GetPositionX(),expected.GetPositionY(),expected.GetPositionZ());
        auto instance=mode?0:player->GetInstanceId();
        player->SetHealth(321);player->GetAura(15007)->SetDuration(598000);
        // Restore admission and retry the healer. Already-alive requests must
        // neither teleport nor charge another durability/sickness penalty.
        session->*Access(SpiritExpansionTag{})=6;
        SpiritGraveRequest(session,healer.GetGUID());session->SendSpiritResurrect();
        Require(player->GetHealth()==321&&player->GetAura(15007)->GetDuration()==598000&&item->GetUInt32Value(ITEM_FIELD_DURABILITY)==durability&&player->IsBeingTeleportedFar()==bool(mode),"Spirit retry repeated penalties or changed rejected/pending transfer");
        if(mode==0){
            session->SetMap(nullptr);owned.release();session->SetCanLogout();session->Update(1);
        }else{
            SpiritLoopback realm,instanceSocket;realm.Attach(session);instanceSocket.Attach(session);
            auto& sockets=session->*Access(SpiritSocketsTag{});
            sockets[CONNECTION_TYPE_REALM]=realm.socket;sockets[CONNECTION_TYPE_INSTANCE]=instanceSocket.socket;
            session->ResetTimeOutTime();session->*Access(SpiritExpireTag{})=100;
            auto pendingMap=session->GetMap();Require(pendingMap&&pendingMap->GetId()==destination->MapID,"Spirit pending session map missing");
            owned.release(); // Actual Map::UpdateSessions -> LogoutPlayer owns deletion.
            if(mode==1)realm.PeerClose();
            if(mode==2)session->UpdateTimeOutTime(1000);
            if(mode==3)instanceSocket.PeerClose();
            if(mode!=2)Require(session->IsforceExit(),"Native WorldSocket::OnClose did not mark its attached session");
            pendingMap->UpdateSessions(40);
            Require(session->IsforceExit()&&!realm.socket->IsOpen()&&!instanceSocket.socket->IsOpen()&&session->*Access(SpiritExpireTag{})==60,"Native attached-socket close/idle timeout did not bypass the remaining grace");
            Require(!session->GetPlayer()&&!sockets[CONNECTION_TYPE_REALM]&&!sockets[CONNECTION_TYPE_INSTANCE]&&!session->GetMap(),"Spirit disconnect expiry did not log out/delete player and clear transports");
            // Global session cleanup must also tolerate the map-owned logout.
            sWorld->UpdateSessions(1);
        }
        Require(!session->GetPlayer()&&!ObjectAccessor::FindPlayer(guid),"Spirit failure logout retained player");FlushWrites();
        auto saved=CharacterDatabase.Query(Trinity::StringFormat("SELECT map,position_x,position_y,position_z,orientation,zone,health,playerFlags,instance_id,online FROM characters WHERE guid=%u",guid.GetCounter()).c_str());Require(bool(saved),"Spirit failure save missing");auto f=saved->Fetch();
        Require(f[0].GetUInt32()==expected.GetMapId()&&std::abs(f[1].GetFloat()-expected.GetPositionX())<0.01f&&std::abs(f[2].GetFloat()-expected.GetPositionY())<0.01f&&std::abs(f[3].GetFloat()-expected.GetPositionZ())<0.01f&&std::abs(f[4].GetFloat()-expected.GetOrientation())<0.001f&&f[5].GetUInt32()==zone&&f[6].GetUInt64()==321&&!(f[7].GetUInt32()&PLAYER_FLAGS_GHOST)&&f[8].GetUInt32()==instance&&!f[9].GetBool(),"Spirit failure save lost recovery position/zone/health/alive/offline state");
        auto row=CharacterDatabase.Query(Trinity::StringFormat("SELECT durability FROM item_instance WHERE guid=%llu",static_cast<unsigned long long>(itemGuid)).c_str());Require(row&&row->Fetch()[0].GetUInt32()==durability,"Spirit failure saved durability changed");
        auto corpses=CharacterDatabase.Query(Trinity::StringFormat("SELECT COUNT(*) FROM corpse WHERE guid=%u AND corpseType<>0",guid.GetCounter()).c_str());Require(corpses&&!corpses->Fetch()[0].GetUInt32(),"Spirit failure retained resurrectable corpse");
        session->HandleWorldPortAck();SpiritGraveRequest(session,healer.GetGUID());Require(!session->GetPlayer(),"Spirit failure stale requests recreated player");
        {GarmPlayer fresh(*leader.GetMap(),leader.getRace(),guid.GetCounter());fresh.SetMaxHealth(1000);fresh.SetHealth(f[6].GetUInt64());fresh.SetUInt32Value(PLAYER_FIELD_PLAYER_FLAGS,f[7].GetUInt32());
        auto stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURAS);stmt->setUInt64(0,guid.GetCounter());auto auras=ReadPrepared(stmt);stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURAS_EFFECTS);stmt->setUInt64(0,guid.GetCounter());(fresh.*Access(ControlAurasLoadTag{}))(auras,ReadPrepared(stmt),0);
        fresh.LoadCorpse();SpiritGravePlace(fresh,mode?*destination:*source);session->HandleWorldPortAck();session->SendSpiritResurrect();
        Require(fresh.IsAlive()&&fresh.GetHealth()==321&&fresh.HasAura(15007)&&!fresh.GetCorpse()&&!fresh.IsBeingTeleported()&&fresh.GetMapId()==expected.GetMapId()&&fresh.GetExactDist(expected)<0.01f,"Spirit failure fresh-player component recovery failed");
        fresh.GetMap()->RemovePlayerFromMap(&fresh,false);}
        SpiritSessionLogin(guid,expected,itemGuid,durability,session,uint32(raid.GetDifficultyID()),leader.getRace(),direction,mode);
        SpiritLoadingOrderLogin(guid,expected,itemGuid,durability,session,leader,raid,direction,mode);
        if(mode==1)TrialScenarioInitialization(guid,expected,itemGuid,durability,session,uint32(raid.GetDifficultyID()),leader.getRace(),direction);
        SpiritFullLogin(guid,expected,itemGuid,durability,session,uint32(raid.GetDifficultyID()),leader.getRace(),direction,mode);
        Require(raid.GetInstanceScript()->GetCompletedEncounterMask()==mask,"Spirit failure changed raid progress");
        std::cout<<"PASS: spirit failure recovery diff="<<uint32(raid.GetDifficultyID())<<" race="<<uint32(leader.getRace())<<" direction="<<direction<<" mode="<<mode<<" map="<<expected.GetMapId()<<" zone="<<zone<<" durability="<<durability<<std::endl;
    }
    sWorld->setIntConfig(CONFIG_SOCKET_TIMEOUTTIME,timeout);
}
