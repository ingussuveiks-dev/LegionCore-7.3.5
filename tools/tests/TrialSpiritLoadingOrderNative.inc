struct SpiritOrderFunctionsTag{using Type=FunctionProcessor Unit::*;friend Type Access(SpiritOrderFunctionsTag);};
template struct FixtureAccess<SpiritOrderFunctionsTag,&Unit::m_Functions>;
struct SpiritOrderChangeTag{using Type=bool Player::*;friend Type Access(SpiritOrderChangeTag);};
template struct FixtureAccess<SpiritOrderChangeTag,&Player::m_change_map>;
struct SpiritOrderWorldTag{using Type=std::atomic<uint64> Object::*;friend Type Access(SpiritOrderWorldTag);};
template struct FixtureAccess<SpiritOrderWorldTag,&Object::m_inWorld>;
struct SpiritInitializationPackets
{
    unsigned total=0;
    std::vector<uint32> difficulties;
    std::vector<std::array<uint32,3>> zones;
};
static SpiritInitializationPackets SpiritReadInitialization(WorldSession* session)
{
    SpiritInitializationPackets result;
    for(auto const& socket:session->*Access(SpiritSocketsTag{}))if(socket){
        EncryptablePacket* queued=nullptr;
        while((socket.get()->*Access(SpiritPacketQueueTag{})).Dequeue(queued)){
            ++result.total;queued->rpos(0);
            if(queued->GetOpcode()==SMSG_WORLD_SERVER_INFO){uint32 difficulty;*queued>>difficulty;result.difficulties.push_back(difficulty);}
            if(queued->GetOpcode()==SMSG_INIT_WORLD_STATES){std::array<uint32,3> zone;*queued>>zone[0]>>zone[1]>>zone[2];result.zones.push_back(zone);}
            delete queued;
        }
    }
    return result;
}
static void SpiritInitializationContext(SpiritInitializationPackets const& packets,Player& player,unsigned expected)
{
    Require(packets.difficulties.size()==expected,"Map initialization packet count is not exactly once per admitted arrival");
    for(auto difficulty:packets.difficulties)Require(difficulty==uint32(player.GetMap()->GetDifficultyID()),"WorldServerInfo contains source-map difficulty");
    uint32 expectedZone,expectedArea;player.GetZoneAndAreaId(expectedZone,expectedArea);
    for(auto const& zone:packets.zones)Require(zone[0]==player.GetMapId()&&zone[1]==expectedZone&&zone[2]==expectedArea,"Initialization contains source-map/zone/area world states");
}
static void SpiritLoadingDuplicate(WorldSession* session,uint32 map)
{
    auto player=session->GetPlayer();auto loaded=player->*Access(SpiritArrivalLoadedTag{});auto settle=player->*Access(SpiritArrivalSettleTag{});
    SpiritReadInitialization(session);SpiritLoadingScreen(session,map,false);SpiritLoadingScreen(session,map,true);SpiritLoadingScreen(session,map,false);
    Require(player->*Access(SpiritArrivalLoadedTag{})==loaded&&player->*Access(SpiritArrivalSettleTag{})==settle,"Duplicate screen changed acknowledgement or arrival timer");
    auto packets=SpiritReadInitialization(session);Require(!packets.total,"Repeated completed screen dispatched initialization packets again");
}
static void SpiritLoadingOrderLogin(ObjectGuid guid,WorldLocation const& expected,uint64 itemGuid,uint32 durability,WorldSession* oldSession,GarmPlayer& leader,InstanceMap& raid,unsigned direction,unsigned sourceMode)
{
    auto addresses=realm.Addresses;realm.Addresses={boost::asio::ip::address_v4::loopback(),boost::asio::ip::address_v4::loopback()};
    // Far-first makes the baseline report the actual source/destination
    // difficulty mismatch, rather than just the early-login duplicate.
    for(unsigned mode:{1u,0u,2u,3u,4u}){
        SpiritLoopback realmSocket,instanceSocket;auto session=new WorldSession(0,"LoadingOrder",realmSocket.socket,SEC_PLAYER,6,0,"Win",LOCALE_enUS,0,false,AT_AUTH_FLAG_NONE,{});
        auto shared=WorldSessionPtr(session,[](WorldSession*){});realmSocket.socket->SetWorldSession(shared);
        auto& sessions=const_cast<SessionMap&>(sWorld->GetAllSessions());auto found=sessions.find(0);WorldSessionPtr previous;
        if(found!=sessions.end()){previous=found->second;Require(previous.get()==oldSession,"Loading-order fixture would replace unrelated session");}sessions[0]=shared;
        SpiritRequestLogin(session,guid);SpiritLinkSocket(instanceSocket,session->GetConnectToInstanceKey());auto response=SpiritTakeLinkQuery(session);SpiritDeliverQuery(session,std::move(response));auto player=session->GetPlayer();Require(player,"Loading-order SQL did not construct Player");auto final=expected;
        static bool abi=false;if(!abi){abi=true;std::cout<<"SPIRIT_LOADING_ORDER_ABI state="<<(reinterpret_cast<char*>(&(session->*Access(SpiritLoadingStateTag{})))-reinterpret_cast<char*>(session))<<" changing="<<(reinterpret_cast<char*>(&(player->*Access(SpiritOrderChangeTag{})))-reinterpret_cast<char*>(player))<<" world="<<(reinterpret_cast<char*>(&(player->*Access(SpiritOrderWorldTag{})))-reinterpret_cast<char*>(player))<<std::endl;}
        if(mode==0){
            SpiritReadInitialization(session);SpiritLoadingScreen(session,expected.GetMapId(),false);auto early=SpiritReadInitialization(session);Require(!early.total,"Early login completion dispatched initialization before map admission");
            auto map=player->GetMap();SpiritFarFreeze(player->GetMapId());map->UpdateSessions(99);Require(SpiritReadInitialization(session).difficulties.empty(),"Login initialization ran before100ms");map->UpdateSessions(1);SpiritFarFreeze(expected.GetMapId());(player->*Access(SpiritOrderFunctionsTag{})).Update(300);SpiritInitializationContext(SpiritReadInitialization(session),*player,1);
        }else SpiritCancelComplete(session,guid,expected,itemGuid,durability);
        (player->*Access(SpiritOrderFunctionsTag{})).Update(300);SpiritReadInitialization(session);
        Group* group=nullptr;
        if(mode==1||mode==2){
            group=new Group();Require(group->Create(player),"Loading-order raid group creation failed");sGroupMgr->AddGroup(group);group->ConvertToRaid();group->SetRaidDifficultyID(raid.GetDifficultyID());player->SetRaidDifficultyID(raid.GetDifficultyID());
            auto entrance=sWorldSafeLocsStore.LookupEntry(5775);Require(entrance&&entrance->MapID==1648,"Loading-order raid anchor missing");final=WorldLocation(1648,entrance->Loc.X,entrance->Loc.Y,entrance->Loc.Z,0);
            Require(player->TeleportTo(final)&&player->IsBeingTeleportedFar()&&player->GetTeleportMap(),"Loading-order actual raid transfer failed");auto destinationRaid=player->GetTeleportMap()->ToInstanceMap();Require(destinationRaid&&destinationRaid->GetId()==1648&&destinationRaid->GetDifficultyID()==raid.GetDifficultyID(),"Loading-order selected raid difficulty differs");SpiritFarFreeze(1648);SpiritReadInitialization(session);
            if(mode==1){
                SpiritLoadingScreen(session,1648,false);auto early=SpiritReadInitialization(session);
                std::cout<<"LOADING_ORDER_EARLY sourceMap="<<player->GetMapId()<<" destinationMap=1648 sourceDifficulty="<<uint32(player->GetMap()->GetDifficultyID())<<" destinationDifficulty="<<uint32(raid.GetDifficultyID())<<" initialPackets="<<early.difficulties.size();if(!early.difficulties.empty())std::cout<<" packetDifficulty="<<early.difficulties.front();std::cout<<std::endl;
                if(early.total){group->UnbindInstance(1648,raid.GetDifficultyID());player->SetGroup(nullptr);group->Disband();group=nullptr;session->SetMap(nullptr);session->LogoutPlayer(true);FlushWrites();if(previous)sessions[0]=previous;else sessions.erase(0);Require(false,"Early destination finish dispatched source-map initialization before WorldPortAck");}SpiritLoadingScreen(session,1648,true);Require(player->*Access(SpiritArrivalLoadedTag{})==1648,"Late start erased retained early destination finish");SpiritLoadingScreen(session,1648,false);Require(!SpiritReadInitialization(session).total,"Repeated early finish/start dispatched packets");
            }
            session->HandleWorldPortAck();SpiritFarFreeze(1648);Require(player->GetMap()==destinationRaid&&player->IsInWorld()&&!player->IsBeingTeleported()&&!player->IsChangeMap(),"Loading-order raid ACK failed");
            (player->*Access(SpiritOrderFunctionsTag{})).Update(300);auto admission=SpiritReadInitialization(session);SpiritInitializationContext(admission,*player,mode==1?1:0);if(mode==1)Require(!admission.zones.empty(),"Raid admission did not exercise native delayed world-state serialization");
            if(mode==2){SpiritLoadingScreen(session,1648,false);(player->*Access(SpiritOrderFunctionsTag{})).Update(300);SpiritInitializationContext(SpiritReadInitialization(session),*player,1);}
            SpiritLoadingDuplicate(session,1648);
            // Return through a second real far transfer: initialization must
            // reset for this arrival and use the outdoor difficulty again.
            final=expected;Require(player->TeleportTo(final)&&player->IsBeingTeleportedFar(),"Loading-order return transfer failed");SpiritFarFreeze(final.GetMapId());SpiritReadInitialization(session);
            SpiritLoadingScreen(session,final.GetMapId(),false);Require(!SpiritReadInitialization(session).total,"Return screen initialized before ACK");session->HandleWorldPortAck();SpiritFarFreeze(final.GetMapId());player->UpdateZone(player->GetCurrentZoneID(),player->GetCurrentAreaID());SpiritFarFreeze(final.GetMapId());if(player->IsBeingTeleportedFar()&&player->IsChangeMap())session->m_Functions.Update(500);
            (player->*Access(SpiritOrderFunctionsTag{})).Update(300);SpiritInitializationContext(SpiritReadInitialization(session),*player,1);group->UnbindInstance(1648,raid.GetDifficultyID());player->UnbindInstance(1648,raid.GetDifficultyID());group->Disband();group=nullptr;
        }
        if(mode==3){
            for(unsigned pass=0;pass<2;++pass){final.Relocate(final.GetPositionX()+40,final.GetPositionY(),final.GetPositionZ(),final.GetOrientation());Require(player->TeleportTo(final)&&player->IsBeingTeleportedNear(),"Loading-order near transfer failed");SpiritReadInitialization(session);SpiritLoadingScreen(session,final.GetMapId(),false);Require(!SpiritReadInitialization(session).total,"Near screen initialized before movement ACK");WorldPackets::Movement::MoveTeleportAck ack{WorldPacket(CMSG_MOVE_TELEPORT_ACK)};ack.MoverGUID=guid;session->HandleMoveTeleportAck(ack);(player->*Access(SpiritOrderFunctionsTag{})).Update(300);SpiritInitializationContext(SpiritReadInitialization(session),*player,1);SpiritLoadingDuplicate(session,final.GetMapId());}
        }
        if(mode==4){
            auto destination=expected.GetMapId()==1220?sObjectMgr->GetDefaultGraveYard(player->GetTeam()):sWorldSafeLocsStore.LookupEntry(5779);Require(destination,"Loading-order replacement destination missing");WorldLocation abandoned(destination->MapID,destination->Loc.X,destination->Loc.Y,destination->Loc.Z,0);
            Require(player->TeleportTo(abandoned)&&player->IsBeingTeleportedFar(),"Loading-order first pending transfer failed");SpiritFarFreeze(abandoned.GetMapId());SpiritReadInitialization(session);SpiritLoadingScreen(session,abandoned.GetMapId(),false);Require(!SpiritReadInitialization(session).total,"Superseded destination initialized before admission");
            Require(player->TeleportTo(expected)&&player->IsBeingTeleportedFar(),"Loading-order replacement transfer failed");SpiritFarFreeze(expected.GetMapId());SpiritReadInitialization(session);SpiritWrongMap(session,abandoned.GetMapId(),false);session->HandleWorldPortAck();SpiritFarFreeze(expected.GetMapId());player->UpdateZone(player->GetCurrentZoneID(),player->GetCurrentAreaID());SpiritFarFreeze(expected.GetMapId());if(player->IsBeingTeleportedFar()&&player->IsChangeMap())session->m_Functions.Update(500);SpiritInitializationContext(SpiritReadInitialization(session),*player,0);SpiritLoadingScreen(session,expected.GetMapId(),false);(player->*Access(SpiritOrderFunctionsTag{})).Update(300);SpiritInitializationContext(SpiritReadInitialization(session),*player,1);
        }
        Require(player->IsInWorld()&&!player->IsBeingTeleported()&&!player->IsChangeMap(),"Loading-order final arrival incomplete");SpiritLoadingDuplicate(session,final.GetMapId());auto item=player->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(itemGuid));Require(player->GetHealth()==321&&player->HasAura(15007)&&player->GetAura(15007)->GetDuration()==598000&&item&&item->GetUInt32Value(ITEM_FIELD_DURABILITY)==durability&&player->GetUnitBeingMoved()==player&&!player->GetCorpse(),"Loading-order changed health/aura/item/mover");
        session->SetMap(nullptr);session->LogoutPlayer(true);FlushWrites();session->m_Functions.Update(1000);Require(!session->GetPlayer()&&!session->PlayerLoading()&&!ObjectAccessor::FindPlayer(guid)&&!session->IsforceExit()&&realmSocket.socket->IsOpen(),"Loading-order cleanup retained login or kicked realm");if(previous)sessions[0]=previous;else sessions.erase(0);
        auto saved=CharacterDatabase.Query(Trinity::StringFormat("SELECT health,online,map,position_x,position_y,position_z FROM characters WHERE guid=%u",guid.GetCounter()).c_str());Require(bool(saved),"Loading-order save missing");auto f=saved->Fetch();Require(f[0].GetUInt64()==321&&!f[1].GetBool()&&f[2].GetUInt32()==final.GetMapId()&&std::abs(f[3].GetFloat()-final.GetPositionX())<0.01f&&std::abs(f[4].GetFloat()-final.GetPositionY())<0.01f&&std::abs(f[5].GetFloat()-final.GetPositionZ())<0.01f,"Loading-order saved state changed");
        CharacterDatabase.DirectExecute(Trinity::StringFormat("UPDATE characters SET map=%u,position_x=%.9g,position_y=%.9g,position_z=%.9g,orientation=%.9g,instance_id=0 WHERE guid=%u",expected.GetMapId(),expected.GetPositionX(),expected.GetPositionY(),expected.GetPositionZ(),expected.GetOrientation(),guid.GetCounter()).c_str());
        std::cout<<"PASS: spirit loading order diff="<<uint32(raid.GetDifficultyID())<<" race="<<uint32(leader.getRace())<<" direction="<<direction<<" source="<<sourceMode<<" mode="<<mode<<" map="<<expected.GetMapId()<<std::endl;
    }
    realm.Addresses=std::move(addresses);
}
