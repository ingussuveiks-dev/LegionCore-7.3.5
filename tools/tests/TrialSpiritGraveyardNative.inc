// Spirit healer transfer, actual near ACK and actual session-owned logout.
struct SpiritNearTag{using Type=bool Player::*;friend Type Access(SpiritNearTag);};
template struct FixtureAccess<SpiritNearTag,&Player::mSemaphoreTeleport_Near>;
static Map* SpiritGravePlace(GarmPlayer& player,WorldSafeLocsEntry const& loc)
{
    player.GetMap()->RemovePlayerFromMap(&player,false);
    player.Relocate(loc.Loc.X,loc.Loc.Y,loc.Loc.Z,1.25f);
    auto base=sMapMgr->CreateBaseMap(loc.MapID);base->SetMapStop();
    auto& worker=sMapMgr->_mapThreads[loc.MapID];if(worker&&worker->joinable())worker->join();
    auto selected=sMapMgr->CreateMap(loc.MapID,&player);
    if(base->Instanceable()||base->CanCreatedZone())StopGraveyardFixtureZones(*static_cast<MapInstanced*>(base));
    player.ResetMap();player.SetMap(selected);Require(selected->AddPlayerToMap(&player,false),"Spirit graveyard map attachment failed");
    uint32 zone,area;player.GetZoneAndAreaId(zone,area);player.UpdateZone(zone,area);
    return selected;
}
static void SpiritGraveRequest(WorldSession* session,ObjectGuid healer)
{
    WorldPacket wire(CMSG_SPIRIT_HEALER_ACTIVATE);wire<<healer;
    WorldPackets::NPC::SpiritHealerActivate request(std::move(wire));request.Read();
    session->HandleSpiritHealerActivate(request);
}
static void SpiritGraveAck(GarmPlayer& player,WorldSafeLocsEntry const& destination)
{
    auto session=player.GetSession();AckTravel(player,destination.Loc.X,destination.Loc.Y,destination.Loc.Z);
    // ZoneTeleport uses a native500ms session callback, not WorldPortAck.
    // Freeze fixture-owned workers and dispatch that real callback explicitly.
    StopGraveyardFixtureZones(*static_cast<MapInstanced*>(sMapMgr->FindBaseMap(1220)));
    if(player.IsBeingTeleportedFar()&&player.IsChangeMap())session->m_Functions.Update(500);
    WorldPacket wire(CMSG_MOVE_TELEPORT_ACK);wire<<player.GetGUID()<<int32(0)<<int32(0);WorldPackets::Movement::MoveTeleportAck ack(std::move(wire));ack.Read();session->HandleMoveTeleportAck(ack);
    Require(player.IsInWorld()&&player.GetHealth()==321&&!player.IsBeingTeleported()&&player.GetExactDist(destination.Loc.X,destination.Loc.Y,destination.Loc.Z)<0.01f,"Native zone handoff or repeated near ACK changed spirit arrival");
}
static void TrialSpiritGraveyardCases(GarmPlayer& leader,InstanceMap& raid)
{
    sObjectMgr->LoadGraveyardZones();
    auto body=sWorldSafeLocsStore.LookupEntry(5779),ghost=sWorldSafeLocsStore.LookupEntry(4949);
    Require(body&&ghost&&body->MapID==1220&&ghost->MapID==1220,"Native Stormheim/Azsuna graveyard anchors missing");
    auto destination=sObjectMgr->GetClosestGraveYard(body->Loc.X,body->Loc.Y,body->Loc.Z,body->MapID,leader.GetTeam());
    auto source=sObjectMgr->GetClosestGraveYard(ghost->Loc.X,ghost->Loc.Y,ghost->Loc.Z,ghost->MapID,leader.GetTeam());
    Require(destination&&source&&source!=destination&&destination->MapID==1220,"Different native graveyard selection missing");
    auto zone=sMapMgr->GetZoneId(destination->MapID,destination->Loc.X,destination->Loc.Y,destination->Loc.Z);
    auto sourceZone=sMapMgr->GetZoneId(source->MapID,source->Loc.X,source->Loc.Y,source->Loc.Z);
    std::cout<<"SPIRIT_GRAVE_SELECT corpse="<<destination->ID<<" ghost="<<source->ID<<" destinationZone="<<zone<<" sourceZone="<<sourceZone<<std::endl;
    Require(zone&&sourceZone&&zone!=sourceZone,"Cross-zone spirit graveyard inputs missing");
    auto mask=raid.GetInstanceScript()->GetCompletedEncounterMask();
    for(unsigned mode=0;mode<3;++mode)
    {
        auto owned=std::make_unique<GarmPlayer>(*leader.GetMap(),leader.getRace());auto target=owned.get();auto guid=target->GetGUID();auto session=target->GetSession();
        std::cout<<"SPIRIT_GRAVE_ABI near="<<(reinterpret_cast<char*>(&(target->*Access(SpiritNearTag{})))-reinterpret_cast<char*>(target))<<std::endl;
        target->*Access(ControlSocialTag{})=sSocialMgr->LoadFromDB({},guid);
        target->SetHomebind(WorldLocation(1220,body->Loc.X,body->Loc.Y,body->Loc.Z,0),zone);
        SpiritGravePlace(*target,*body);
        sWorld->AddCharacterInfo(guid,0,target->GetName(),GENDER_MALE,target->getRace(),target->getClass(),target->getLevel());target->SaveToDB(true);FlushWrites();
        target->SetMaxHealth(1000);target->KillPlayer();target->BuildPlayerRepop();Require(target->GetCorpse(),"Spirit transfer corpse input missing");
        SpiritGravePlace(*target,*source);GraveyardSession registered(*target,raid);session->SetMap(target->GetMap());
        Actor healer(target->GetMap(),6491,19000+guid.GetCounter(),target->GetPosition());Require(target->GetMap()->AddToMap(static_cast<Creature*>(&healer)),"Spirit transfer healer spawn failed");
        Require(target->GetNPCIfCanInteractWith(healer.GetGUID(),UNIT_NPC_FLAG_SPIRITHEALER)==&healer,"Spirit transfer healer interaction missing");
        target->SaveToDB();FlushWrites();SpiritGraveRequest(session,healer.GetGUID());
        Require(target->IsAlive()&&target->GetHealth()==500&&!target->GetCorpse()&&!target->HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&target->HasAura(15007),"Spirit transfer resurrection/sickness failed");
        Require(target->IsBeingTeleportedNear()&&!target->IsBeingTeleportedFar()&&target->GetTeleportDest().GetExactDist(destination->Loc.X,destination->Loc.Y,destination->Loc.Z)<0.01f,"Spirit healer did not select corpse graveyard near transfer");
        auto expected=target->GetTeleportDest();target->SetHealth(321);auto sickness=target->GetAura(15007);sickness->SetDuration(598000);
        SpiritGraveRequest(session,healer.GetGUID());Require(target->GetHealth()==321&&sickness->GetDuration()==598000&&target->GetTeleportDest().GetExactDist(expected)<0.01f&&target->IsBeingTeleportedNear(),"Repeated spirit request changed pending transfer or penalties");
        if(mode==1)SpiritGraveAck(*target,*destination);
        uint32 expectedInstance=target->IsBeingTeleported()?0:target->GetInstanceId();
        if(mode==2){target->SaveToDB();FlushWrites();}
        else{session->SetMap(nullptr);owned.release();session->SetCanLogout();session->Update(1);Require(!session->GetPlayer()&&!ObjectAccessor::FindPlayer(guid),"Spirit transfer logout did not delete Player");FlushWrites();}
        auto saved=CharacterDatabase.Query(Trinity::StringFormat("SELECT map,position_x,position_y,position_z,orientation,zone,health,playerFlags,instance_id,online FROM characters WHERE guid=%u",guid.GetCounter()).c_str());Require(bool(saved),"Spirit transfer character save missing");auto f=saved->Fetch();
        std::cout<<"SPIRIT_GRAVE_SAVE mode="<<mode<<" map="<<f[0].GetUInt32()<<" zone="<<f[5].GetUInt32()<<" expectedZone="<<zone<<" instance="<<f[8].GetUInt32()<<" health="<<f[6].GetUInt64()<<std::endl;
        Require(f[0].GetUInt32()==expected.GetMapId()&&std::abs(f[1].GetFloat()-expected.GetPositionX())<0.01f&&std::abs(f[2].GetFloat()-expected.GetPositionY())<0.01f&&std::abs(f[3].GetFloat()-expected.GetPositionZ())<0.01f&&std::abs(f[4].GetFloat()-expected.GetOrientation())<0.001f&&f[5].GetUInt32()==zone&&f[6].GetUInt64()==321&&!(f[7].GetUInt32()&PLAYER_FLAGS_GHOST)&&f[8].GetUInt32()==expectedInstance&&(mode==2||!f[9].GetBool()),"Spirit graveyard save lost destination/zone/alive/health/instance/offline state");
        auto corpses=CharacterDatabase.Query(Trinity::StringFormat("SELECT COUNT(*) FROM corpse WHERE guid=%u AND corpseType<>0",guid.GetCounter()).c_str());Require(corpses&&!corpses->Fetch()[0].GetUInt32(),"Spirit transfer retained resurrectable SQL corpse");
        if(mode<2){
            GarmPlayer fresh(*leader.GetMap(),leader.getRace(),guid.GetCounter());fresh.SetMaxHealth(1000);fresh.SetHealth(f[6].GetUInt64());fresh.SetUInt32Value(PLAYER_FIELD_PLAYER_FLAGS,f[7].GetUInt32());
            auto stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURAS);stmt->setUInt64(0,guid.GetCounter());auto auras=ReadPrepared(stmt);
            stmt=CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURAS_EFFECTS);stmt->setUInt64(0,guid.GetCounter());(fresh.*Access(ControlAurasLoadTag{}))(auras,ReadPrepared(stmt),0);
            fresh.LoadCorpse();Require(fresh.IsAlive()&&fresh.GetHealth()==321&&!fresh.GetCorpse()&&!fresh.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&fresh.HasAura(15007),"Spirit saved-field/aura/LoadCorpse relog component lost recovery");
            SpiritGravePlace(fresh,*destination);SpiritGraveRequest(session,healer.GetGUID());
            Require(!session->GetPlayer()&&fresh.IsAlive()&&fresh.GetHealth()==321&&!fresh.GetCorpse()&&fresh.GetExactDist(destination->Loc.X,destination->Loc.Y,destination->Loc.Z)<0.01f,"Old-session spirit packet changed reloaded Player");
            fresh.GetMap()->RemovePlayerFromMap(&fresh,false);
            std::cout<<"PASS: spirit graveyard logout saved-field/prepared-aura relog and old-session request diff="<<uint32(raid.GetDifficultyID())<<" race="<<uint32(leader.getRace())<<" mode="<<mode<<std::endl;
        }
        if(owned){SpiritGraveAck(*target,*destination);target->GetMap()->RemovePlayerFromMap(target,false);}
        Require(raid.GetInstanceScript()->GetCompletedEncounterMask()==mask,"Spirit transfer changed raid progress");
        std::cout<<"PASS: spirit graveyard transfer/save/logout diff="<<uint32(raid.GetDifficultyID())<<" race="<<uint32(leader.getRace())<<" mode="<<mode<<std::endl;
    }
}
